#include <sys/types.h>   /* for type definitions */
#include <sys/stat.h>
#include <sys/socket.h>  /* for socket API function calls */
#include <netinet/in.h>  /* for address structs */
#include <arpa/inet.h>   /* for sockaddr_in */
#include <stdio.h>       /* for printf() */
#include <stdlib.h>      /* for atoi() */
#include <syslog.h>      /* for log info */
#include <string.h>      /* for strlen() */
#include <unistd.h>      /* for close() */
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sqlite3.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>

#define MAJOR_VER 1
#define MINOR_VER 0
#define POINT_VER 1

#define MAX_LEN  1024    /* maximum string size to send */
#define MIN_PORT 1024    /* minimum port allowed */
#define MAX_PORT 65535   /* maximum port allowed */

#define CLIENT_RETRY_CONNECTION_TIMER 10

#define LOG_FILE_PATH_LENGTH        128
#define MAX_LOG_FILE_NAME_LENGTH    (LOG_FILE_PATH_LENGTH + 128)

#define MAX_INTERFACE_NAME_LENGTH 32
#define DISK_FIELDS 6
#define MAX_BOARDS 10
#define BOARD_FIELDS 9
#define MAX_BROADCAST 4
#define NUM_CONFIG_OPTIONS 12

// =====================================================================
// Daemon Defines
// =====================================================================

#define MAX_LINE_LENGTH             255
#define STATE_LENGTH                32 
#define STATUS_FILE_PATH_LENGTH     128
#define CONFIG_FILE_PATH_LENGTH     128
#define CONFIG_FILE_BASENAME_LENGTH 64 

#define LOG_FILE_PATH_LENGTH        128
                                    
#define MAX_STATUS_FILE_NAME_LENGTH (STATUS_FILE_PATH_LENGTH + 128)
#define MAX_CONFIG_FILE_NAME_LENGTH (CONFIG_FILE_PATH_LENGTH + CONFIG_FILE_BASENAME_LENGTH) 
#define MAX_DATABASE_FILE_NAME_LENGTH (CONFIG_FILE_PATH_LENGTH + CONFIG_FILE_BASENAME_LENGTH)
#define MAX_LOG_FILE_NAME_LENGTH    (LOG_FILE_PATH_LENGTH + 128)
                                    
#define CHECK_INTERVAL              10 // seconds
#define MAX_ERROR_DURATION          (5*60) // seconds
                                    
#define DEFAULT_NUM_LOG_FILES       10
#define DEFAULT_LOG_SIZE            (10*1024*1024)

#define DEFAULT_STATUS_FILE_PATH    "/var/www"
#define DEFAULT_CONFIG_FILE_PATH    "/etc/lanManager"
#define DEFAULT_CONFIG_FILE_NAME    "/etc/lanManager/labAuditConfig.ini"
#define DEFAULT_TEXT_FILE_NAME      "/etc/lanManager/lanScan.txt"
#define DEFAULT_TEXT_FILE_NAME2     "/etc/lanManager/lanScan2.txt"
#define DEFAULT_TEXT_FILE_NAME3     "/etc/lanManager/lanScan3.txt"
#define DEFAULT_TEXT_FILE_NAME4     "/etc/lanManager/lanScan4.txt"
#define DEFAULT_DATABASE_FILE_NAME  "/etc/lanManager/packetTest.db"
#define DEFAULT_LOG_FILE_PATH       "/var/log/lanManager"
#define DEFAULT_STATE_BASENAME      "lanManager.state"


// =========================================================================
// volatile data / global data
// =========================================================================

volatile int updateMark = 0;
extern int errno;
int MAX_INTERFACES = 0;
int MAX_DISKS = 0;
int NUM_BROADCASTS = 0;

// =====================================================================
// enums
// =====================================================================

enum {
    APP_STATE_UNKNOWN   =-1
   ,APP_STATE_IDLE      =0
   ,APP_STATE_ACTIVE    =1
   ,APP_STATE_STARTING  =2
   ,APP_STATE_STOPPING  =3
   ,APP_STATE_ERROR     =4
};

// =========================================================================
// types
// =========================================================================

typedef struct {
    int max_log_files;
    int max_log_size;
    int max_error_duration;
    int use_daemon;
    int server;
    int client;
    int reset;
    char status_file_path[STATUS_FILE_PATH_LENGTH+1];
    char config_file_path[CONFIG_FILE_PATH_LENGTH+1];
    char config_file_name[MAX_CONFIG_FILE_NAME_LENGTH+1];
    char database_file_name[MAX_CONFIG_FILE_NAME_LENGTH+1];
    char text_file_name[MAX_BROADCAST][MAX_CONFIG_FILE_NAME_LENGTH+1];
    char manager_state_name[MAX_LOG_FILE_NAME_LENGTH+1];
    char log_file_path[LOG_FILE_PATH_LENGTH+1];
} app_config_t;

typedef struct {
    app_config_t cfg;
    int log_fd;
    int log_file_cnt;
    int log_file_idx;
} app_info_t;

// =========================================================================
// static data
// =========================================================================

static app_info_t app_info;

// =========================================================================
// static functions
// =========================================================================

static const char *state_name(int state)
{
    switch (state) {
        case APP_STATE_IDLE: return "IDLE";
        case APP_STATE_ACTIVE: return "ACTIVE";
        case APP_STATE_STARTING: return "STARTING";
        case APP_STATE_STOPPING: return "STOPPING";
        case APP_STATE_ERROR: return "ERROR";
        //default: return "UNKNOWN";
    }

    return "UNKNOWN";
}


static void usage()
{
    printf("lanManager [OPTIONS]\n\n");
    printf("OPTIONS:\n\n");
    printf("  -h|--help      : this message\n");
    printf("  -d|--daemon    : run as a daemon\n");
    printf("  -s|--server    : run in server mode (otherwise the program will run in client mode)\n");
    printf("  -t|--client    : force run in client mode (overrides config setting, and auto runs as client)\n");
    printf("  -r|--reset     : reset database contents\n");
    printf("  -c|--config-file /path/to/file : specify alternate config file.\n");

    exit(0);
}

int validate_config_filename(app_info_t *app, char *filename)
{
    FILE *fp;

    if ((fp = fopen(filename, "r")) != NULL) {
        syslog(LOG_ERR, "config file: %s ... valid\n", filename);
        fprintf(stderr, "config file: %s ... valid\n", filename);
        return 1;
    } else {
        syslog(LOG_ERR, "config file: %s ... invalid!!\n", filename);
        fprintf(stderr, "config file: %s ... invalid!!\n", filename);
        return 0;
    }

    fclose(fp);
}

int save_config_filename(app_info_t *app, char *filename)
{
    app_config_t *cfg = &app->cfg;
    int len = strlen(filename);
    int rc;

    if (len > MAX_CONFIG_FILE_NAME_LENGTH)
        len = MAX_CONFIG_FILE_NAME_LENGTH;

    if (filename[0] == '/') {
        memcpy(cfg->config_file_name, filename, len);
    } else {
        int free_space;
        char *ret_dir;
        ret_dir = getcwd(cfg->config_file_name, MAX_CONFIG_FILE_NAME_LENGTH);
        if (ret_dir == NULL) {
            int errsave = errno;
            syslog(LOG_ERR, "error getting cwd error=%s\n", strerror(errno));

            fprintf(stderr, "error getting cwd error=%s\n", strerror(errno));
            goto error;
        }


        free_space = MAX_CONFIG_FILE_NAME_LENGTH - strlen(cfg->config_file_name);
        if (free_space <= 1) {
            syslog(LOG_ERR, "error config filename too long\n");
            fprintf(stderr, "error config filename too long\n");
            goto error;
        }
        strncat(cfg->config_file_name, "/", free_space);

        free_space = MAX_CONFIG_FILE_NAME_LENGTH - strlen(cfg->config_file_name);

        if (filename[0] == '.' && filename[1] == '/') {
            if (free_space <= (strlen(filename)-2)) {
                syslog(LOG_ERR, "error config filename too long\n");
                fprintf(stderr, "error config filename too long\n");
                goto error;
            }
            strncat(cfg->config_file_name, &filename[2], len);
        } else {
            if (free_space <= strlen(filename)) {
                syslog(LOG_ERR, "error config filename too long\n");
                fprintf(stderr, "error config filename too long\n");
                goto error;
            }
            strncat(cfg->config_file_name, filename, free_space);
        }
    }

    syslog(LOG_INFO, "config file name=%s\n", cfg->config_file_name);
    fprintf(stderr, "config file name=%s\n", cfg->config_file_name);

    return 0;

error:
    return -1;
}

static void show_cmd_line(int argc, char *argv[])
{
    int i; 
    for (i=0;i<argc;i++) { 
        fprintf(stderr, "argv[%d] = %s\n", i, argv[i]); 
    } 
}

static struct option long_options[] = {
    {"config-file", required_argument, 0, 'c'}, 
    {"daemon",      no_argument,       0, 'd'}, 
    {"version",     no_argument,       0, 'v'}, 
    {"server",      no_argument,       0, 's'},
    {"client",      no_argument,       0, 't'},
    {"reset",       no_argument,       0, 'r'},  
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};


static int parse_cmd_line(app_info_t *app, int argc, char *argv[])
{
    app_config_t *cfg = &app->cfg;
    int i; 
    int rc = 0;
    int opt_index;
    int use_daemon = 0;
    int use_config_file = 0;
    int verbosity = 0;
    int use_help = 0;
    unsigned char config_filename[MAX_CONFIG_FILE_NAME_LENGTH];

    while (1) {
        int option_index = 0;
        int c;

        c = getopt_long(argc, argv, "c:chdvstrh:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'c': {
                if (optarg) {
                    use_config_file = 1;
                    snprintf(config_filename, MAX_CONFIG_FILE_NAME_LENGTH, "%s", optarg);
                    fprintf(stderr, "config_filename = %s\n", config_filename); 

                    if (validate_config_filename(app, config_filename)) {
                        save_config_filename(app, config_filename);
                    } else {
                        rc = -1;
                    }

                }
            }
            break;

            case 'd': {
                cfg->use_daemon = 1;
                fprintf(stderr, "use daemon\n"); 
            }
            break;

            case 'v': {
                fprintf(stderr, "VERSION=%d.%d.%d (%s @ %s)\n", MAJOR_VER, MINOR_VER, POINT_VER, __DATE__, __TIME__);
                exit(0);
            }
            break;

            case 'h': {
                use_help = 1;
                fprintf(stderr, "show help\n"); 
            }
            break;

            case 's': {
                cfg->client = 0;
                cfg->server = 1;
            }
            break;

            case 't': {
                cfg->server = 0;
                cfg->client = 1;
            }
            break;

            case 'r': {
                cfg->reset = 1;
            }
            break;

            default:
                fprintf(stderr, "ERROR: unknown option!!\n");
                return -1;
        }
    }

    if (use_help) {
        usage();
    }

    return rc;
}


static int init_app(app_info_t *app)
{
    app_config_t *cfg = &app->cfg;

    memset(app, 0, sizeof(*app));

    // now initialize the default config
 
    cfg->max_log_files = DEFAULT_NUM_LOG_FILES;
    cfg->max_log_size = DEFAULT_LOG_SIZE;

    snprintf(cfg->status_file_path, STATUS_FILE_PATH_LENGTH, DEFAULT_STATUS_FILE_PATH);
    snprintf(cfg->config_file_path, CONFIG_FILE_PATH_LENGTH, DEFAULT_CONFIG_FILE_PATH);
    snprintf(cfg->config_file_name, MAX_CONFIG_FILE_NAME_LENGTH, DEFAULT_CONFIG_FILE_NAME);
    snprintf(cfg->database_file_name, MAX_DATABASE_FILE_NAME_LENGTH, DEFAULT_DATABASE_FILE_NAME);
    snprintf(cfg->text_file_name[0], MAX_DATABASE_FILE_NAME_LENGTH, DEFAULT_TEXT_FILE_NAME);
    snprintf(cfg->text_file_name[1], MAX_DATABASE_FILE_NAME_LENGTH, DEFAULT_TEXT_FILE_NAME2);
    snprintf(cfg->text_file_name[2], MAX_DATABASE_FILE_NAME_LENGTH, DEFAULT_TEXT_FILE_NAME3); 
    snprintf(cfg->text_file_name[3], MAX_DATABASE_FILE_NAME_LENGTH, DEFAULT_TEXT_FILE_NAME4);
    snprintf(cfg->log_file_path, LOG_FILE_PATH_LENGTH, DEFAULT_LOG_FILE_PATH);

    return 0;
}

int get_int(char *buffer, const char *format, int *num)
{
    sscanf(buffer, format, num);
}

int get_uint64_t(char *buffer, const char *format, uint64_t *num)
{
    sscanf(buffer, format, num);
}

int get_string(char *buffer, const char *format, char *str, int str_max_len)
{
    int i;
    sscanf(buffer, format, str);
    for (i=0;i<strlen(str) && i<str_max_len; i++) {
        if (str[i] == '"') {
            str[i] = '\0';
        }
    }
}

static void parse(char *line, char **argv)
{
     while (*line != '\0') {       /* if not the end of line ....... */ 
          while (*line == ' ' || *line == '\t' || *line == '\n')
               *line++ = '\0';     /* replace white spaces with 0    */
          *argv++ = line;          /* save the argument position     */
          while (*line != '\0' && *line != ' ' && 
                 *line != '\t' && *line != '\n') 
               line++;             /* skip the argument until ...    */
     }
     *argv = '\0';                 /* mark the end of argument list  */
}

int create_log_files(app_info_t *app)
{
    app_config_t *cfg = &app->cfg;
    char filename[MAX_LOG_FILE_NAME_LENGTH+1];
    struct stat sb;
    int rc;

    // first check if config path exits, if not, create it
    if (stat(cfg->log_file_path, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        //fprintf(stderr, "log path exits\n");
    }
    else
    {
        fprintf(stderr, "log path does not exist, creating\n");
        rc = mkdir(cfg->log_file_path, 0777);
        if (rc) {
            fprintf(stderr, "unable to create log file path:%s error=%s\n", 
                cfg->log_file_path, strerror(errno));
            exit (-1);
        }

    }

    if(cfg->server){
        sprintf(filename,"%s/lanManager_server_log.txt", 
            cfg->log_file_path);
    } else {
         sprintf(filename,"%s/lanManager_client_log.txt", 
            cfg->log_file_path);
    }
        
    app->log_fd = open(filename, O_CREAT|O_RDWR|O_TRUNC, 0077);

    dup2(app->log_fd, fileno(stderr));
    dup2(app->log_fd, fileno(stdout));

    return 0;
}

int rotate_log_files(app_info_t *app)
{
    app_config_t *cfg = &app->cfg;
    char oldfilename[MAX_LOG_FILE_NAME_LENGTH+1];
    char newfilename[MAX_LOG_FILE_NAME_LENGTH+1];
    int rc;

    if(cfg->server){
        sprintf(oldfilename,"%s/lanManager_server_log.txt", 
            cfg->log_file_path);

        sprintf(newfilename,"%s/lanManager_server_log%d.txt", 
            cfg->log_file_path,
            app->log_file_idx);
    } else{
        sprintf(oldfilename,"%s/lanManager_client_log.txt", 
            cfg->log_file_path);

        sprintf(newfilename,"%s/lanManager_client_log%d.txt", 
            cfg->log_file_path,
            app->log_file_idx);
    }
    

    // close the current file
    close(app->log_fd);

    app->log_file_cnt += 1;
    if (app->log_file_cnt >= cfg->max_log_files) {
        rc = unlink(newfilename);
        if (rc) {
            syslog(LOG_INFO, "error unlinking file %s, %s\n", 
                newfilename,
                strerror(errno));
        }
    }


    rc = link(oldfilename, newfilename);
    if (rc) {
        syslog(LOG_INFO, "error linking file %s, %s\n", 
            newfilename,
            strerror(errno));
    }

    rc = unlink(oldfilename);
    if (rc) {
        syslog(LOG_INFO, "error unlinking file %s, %s\n", 
            newfilename,
            strerror(errno));
    }


    app->log_file_idx += 1;
    if (app->log_file_idx >= cfg->max_log_files) {
        app->log_file_idx -= cfg->max_log_files;
    }

        
    app->log_fd = open(oldfilename, O_CREAT|O_RDWR|O_TRUNC, 0077);

    dup2(app->log_fd, fileno(stderr));
    dup2(app->log_fd, fileno(stdout));

    return -1;
}

int check_log_rotation(app_info_t *app)
{
    app_config_t *cfg = &app->cfg;
    struct stat fs;

    fstat(app->log_fd, &fs);

    //fprintf(stderr, "total size, in bytes: %ld\n", fs.st_size);

    if (fs.st_size >= cfg->max_log_size) {
        rotate_log_files(app);
    }

    return 0;
}

int daemonize(app_info_t *app)
{
    app_config_t *cfg = &app->cfg;

    // do the daemon stuff
	pid_t pid = 0;
	int fd;

	// Fork off the parent process 
	pid = fork();

	// An error occurred 
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	// Success: Let the parent terminate 
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	// On success: The child process becomes session leader 
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
	}

	// // Ignore signal sent from child to parent process 
	// signal(SIGCHLD, SIG_IGN);

	// Fork off for the second time
	pid = fork();

	// An error occurred 
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	// Success: Let the parent terminate
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	// Set new file permissions
	umask(0);

	// Change the working directory to the root directory
	if (chdir("/") < 0) {
		exit(EXIT_FAILURE);
    }

    // do the specific stuff for this daemon
    create_log_files(app);

    return 0;
}

void error(const char *msg){
    perror(msg);
    exit(1);
}

void updateTrigger(int sig){
    updateMark = 1;
}

//==================================================================================
// Database functions
//==================================================================================

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

//=========================================================================
// Create table functions
//=========================================================================

void enable_foreign_keys(sqlite3 *db, int rc, int zErrMsg){
    char *sql;
    /* Create SQL statement */
    sql = "PRAGMA foreign_keys = ON;";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stderr, "Foreign key enabled\n");
   }

   return;
}

void create_table_index(sqlite3 *db, int rc, char *zErrMsg){
    char *sql;
    /* Create SQL statement */
   sql = "CREATE TABLE IF NOT EXISTS labAudit("  \
         "ID INTEGER PRIMARY KEY     AUTOINCREMENT," \
         "hostname           VARCHAR    NOT NULL," \
         "interface          VARCHAR    NOT NULL," \
         "ipAddress          VARCHAR    NOT NULL UNIQUE," \
         "cpuModel           VARCHAR    NOT NULL," \
         "totalMemory        VARCHAR    NOT NULL," \
         "usedMemory         VARCHAR    NOT NULL," \
         "freeMemory         VARCHAR    NOT NULL," \
         "availableMemory    VARCHAR    NOT NULL," \
         "ramMemory          VARCHAR    NOT NULL," \
         "numDisks           INTEGER    NOT NULL," \
         "timeStamp          VARCHAR    NOT NULL);";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stderr, "Index table created successfully\n");
   }

   return;
}

void create_table_interfaces(sqlite3 *db, int rc, char *zErrMsg){
    char *sql;
    /* Create SQL statement */
   sql = "CREATE TABLE IF NOT EXISTS labInterfaces("  \
         "ID INTEGER PRIMARY KEY     AUTOINCREMENT," \
         "hostname           VARCHAR    NOT NULL," \
         "interface          VARCHAR    NOT NULL," \
         "ipAddress          VARCHAR    NOT NULL UNIQUE," \
         "numInterfaces      INTEGER    NOT NULL);";
         //"FOREIGN KEY(hostname) REFERENCES labAudit(hostname));";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stderr, "Interfaces table created successfully\n");
   }

   return;
}

void create_table_disks(sqlite3 *db, int rc, char *zErrMsg){
    char *sql;
    /* Create SQL statement */
   sql = "CREATE TABLE IF NOT EXISTS labDisks("  \
         "ID INTEGER PRIMARY KEY     AUTOINCREMENT," \
         "hostname           VARCHAR    NOT NULL," \
         "ipAddress          VARCHAR    NOT NULL," \
         "diskName           VARCHAR    NOT NULL," \
         "diskSize           VARCHAR    NOT NULL," \
         "diskUsed           VARCHAR    NOT NULL," \
         "diskAvailable      VARCHAR    NOT NULL," \
         "diskUsePercent     VARCHAR    NOT NULL," \
         "diskMount          VARCHAR    NOT NULL," \
         "numDisks           INTEGER    NOT NULL);";
         //"FOREIGN KEY(hostname) REFERENCES labAudit(hostname));";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stderr, "Disk table created successfully\n");
   }

   return;
}

void create_table_boards(sqlite3 *db, int rc, char *zErrMsg){
    char *sql;
    /* Create SQL statement */
   sql = "CREATE TABLE IF NOT EXISTS labBoards("  \
         "ID INTEGER PRIMARY KEY     AUTOINCREMENT," \
         "hostname           VARCHAR    NOT NULL," \
         "ipAddress          VARCHAR    NOT NULL," \
         "scriptOutput       TEXT       NOT NULL);";
         //"FOREIGN KEY(hostname) REFERENCES labAudit(hostname));";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stderr, "Board table created successfully\n");
   }

   return;
}

void close_database(sqlite3 *db){
    sqlite3_close(db);
}

int open_database(sqlite3 *db, app_info_t *app, int rc)
{
   app_config_t *cfg = &app->cfg;
   const char *filename = cfg->database_file_name;
   char *zErrMsg = 0;

   /* Open database */
   fprintf(stderr, "Opening database\n");
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   }else{
      fprintf(stderr, "Opened database successfully\n");
   }
   create_table_index(db, rc, zErrMsg);
   create_table_interfaces(db, rc, zErrMsg);
   create_table_disks(db, rc, zErrMsg);
   create_table_boards(db, rc, zErrMsg);
   //rc = sqlite3_close(db);
   return 0;
}

//==========================================================================
// Drop table functions
//==========================================================================

void drop_table_index(sqlite3 *db, int rc, char *zErrMsg){
    char *sql;
    /* Create SQL statement */
   sql = "DROP TABLE IF EXISTS labAudit;";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stderr, "Index table dropped successfully\n");
   }

   return;
}

void drop_table_interfaces(sqlite3 *db, int rc, char *zErrMsg){
    char *sql;
    /* Create SQL statement */
   sql = "DROP TABLE IF EXISTS labInterfaces;";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stderr, "Interfaces table dropped successfully\n");
   }

   return;
}

void drop_table_disks(sqlite3 *db, int rc, char *zErrMsg){
    char *sql;
    /* Create SQL statement */
   sql = "DROP TABLE IF EXISTS labDisks;";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stderr, "Disk table dropped successfully\n");
   }

   return;
}

void drop_table_boards(sqlite3 *db, int rc, char *zErrMsg){
    char *sql;
    /* Create SQL statement */
   sql = "DROP TABLE IF EXISTS labBoards;";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stderr, "Board table dropped successfully\n");
   }

   return;
}

int reset_database(app_info_t *app)
{
   app_config_t *cfg = &app->cfg;
   const char *filename = cfg->database_file_name;
   sqlite3 *db;
   char *zErrMsg = 0;
   int  rc;

   /* Open database */
   fprintf(stderr, "Opening database\n");
   rc = sqlite3_open("/etc/lanManager/packetTest.db", &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(1);
   }else{
      fprintf(stderr, "Opened database successfully, dropping tables...\n");
   }
   drop_table_index(db, rc, zErrMsg);
   drop_table_interfaces(db, rc, zErrMsg);
   drop_table_disks(db, rc, zErrMsg);
   drop_table_boards(db, rc, zErrMsg);
   rc = sqlite3_close(db);
   return 0;
}

//==========================================================================
// Update database functions
//==========================================================================

int update_database_index(
        sqlite3 *db,
        char *table,
        char *hostname,
        char *interface,
        char *ipAddress,
        char *cpuModel,
        char *totalMem,
        char *usedMem,
        char *freeMem,
        char *availableMem,
        char *ramMem,
        int disks)
{
    char *zErrMsg = 0;
    int rc;
    int sql_length = 1028;
    time_t clk = time(NULL);
    sqlite3_stmt *res;
    const unsigned char *ret_value;
    const char *data = "Callback function called";

    char* sql = malloc(sql_length+1);
    if (sql == NULL) {
        err("Failed to alloc sql format string\n");
        return -1;
    }

    memset(sql, 0, sql_length+1);

    //rc = sqlite3_open("/etc/lanManager/packetTest.db", &db);
    
    /* Create merged SQL statement */
   snprintf(sql, sql_length, "INSERT OR REPLACE INTO %s (ID, hostname, interface, ipAddress, cpuModel, totalMemory, usedMemory, freeMemory, availableMemory, ramMemory, numDisks, timeStamp) VALUES ((SELECT ID FROM %s WHERE ipAddress = \"%s\"),'%s','%s','%s','%s','%s','%s','%s','%s','%s','%d','%s');",
           table, table, ipAddress, hostname, interface, 
           ipAddress, cpuModel, totalMem, usedMem, 
           freeMem, availableMem, ramMem, disks, ctime(&clk));

   /* Execute SQL statement */
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    
    if (rc == SQLITE_OK) {
        int step = sqlite3_step(res);
            sqlite3_finalize(res);
            rc = 0;
    } else {
        err("Failed to execute statement: %s\n", sqlite3_errmsg(db));
        rc = -1;
    }
    free(sql);
err:
    return rc;
}

int update_database_interfaces(
        sqlite3 *db,
        char *table,
        char *hostname,
        char interface[MAX_INTERFACES][MAX_INTERFACE_NAME_LENGTH],
        char ipAddress[MAX_INTERFACES][16],
        int numInterfaces)
{
    char *zErrMsg = 0;
    int rc;
    int sql_length = 1028;
    sqlite3_stmt *res;
    const unsigned char *ret_value;
    const char *data = "Callback function called";

    for(int i = 0; i < MAX_INTERFACES; i++){
     if(strlen(interface[i]) != 0){
        char* sql = malloc(sql_length+1);
        memset(sql, 0, sql_length+1);
        if (sql == NULL) {
            err("Failed to alloc sql format string\n");
            return -1;
        }

        /* Create merged SQL statement */
        snprintf(sql, sql_length, "INSERT OR REPLACE INTO %s (ID, hostname, interface, ipAddress, numInterfaces) VALUES ((SELECT ID FROM %s WHERE hostname = \"%s\" AND ipAddress = \"%s\"),'%s','%s','%s','%d');",
           table, table, hostname, ipAddress[i],
           hostname, interface[i], ipAddress[i], numInterfaces);

        /* Execute SQL statement */
        rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
        
        if (rc == SQLITE_OK) {
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
            rc = 0;
        } else {
            err("Failed to execute statement: %s\n", sqlite3_errmsg(db));
            rc = -1;
        }
        free(sql);
      }
    }
err:
    return rc;
}

int update_database_disks(
        sqlite3 *db,
        char *table,
        char *hostname,
        char *ipAddress,
        char localDiskFields[MAX_DISKS][DISK_FIELDS][16],
        int disks)
{
    char *zErrMsg = 0;
    int rc;
    int sql_length = 1028;
    sqlite3_stmt *res;
    const unsigned char *ret_value;
    const char *data = "Callback function called";

    int i = 0;
    while(i < MAX_DISKS){
        if(strcmp(localDiskFields[i][0], "") != 0){
            char* sql = malloc(sql_length+1);
            memset(sql, 0, sql_length+1);
            if (sql == NULL) {
                err("Failed to alloc sql format string\n");
                return -1;
            }

            /* Create merged SQL statement */
            snprintf(sql, sql_length, "INSERT OR REPLACE INTO %s (ID, hostname, ipAddress, diskName, diskSize, diskUsed, diskAvailable, diskUsePercent, diskMount, numDisks) VALUES ((SELECT ID FROM %s WHERE ipAddress = \"%s\" AND diskName = \"%s\"),'%s','%s','%s','%s','%s','%s', '%s','%s', '%d');",
            table, table, ipAddress, localDiskFields[i][0],
            hostname, ipAddress, localDiskFields[i][0],
            localDiskFields[i][1], localDiskFields[i][2], localDiskFields[i][3],
            localDiskFields[i][4], localDiskFields[i][5], disks);

            /* Execute SQL statement */
            rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
        
            if (rc == SQLITE_OK) {
                int step = sqlite3_step(res);
                sqlite3_finalize(res);
                rc = 0;
            } else {
                err("Failed to execute statement: %s\n", sqlite3_errmsg(db));
                rc = -1;
            }
            free(sql);
            i++;
        }
        else {
            if(strcmp(localDiskFields[0][0], "") != 0){
                disks = i;

                char* sql = malloc(sql_length+1);
                memset(sql, 0, sql_length+1);
                if (sql == NULL) {
                    err("Failed to alloc sql format string\n");
                    return -1;
                }

                /* Create merged SQL statement */
                snprintf(sql, sql_length, "INSERT OR REPLACE INTO %s (ID, hostname, ipAddress, diskName, diskSize, diskUsed, diskAvailable, diskUsePercent, diskMount, numDisks) VALUES ((SELECT ID FROM %s WHERE ipAddress = \"%s\" AND diskName = \"%s\"),'%s','%s','%s','%s','%s','%s', '%s','%s', '%d');",
                table, table, ipAddress, localDiskFields[0][0],
                hostname, ipAddress, localDiskFields[0][0],
                localDiskFields[0][1], localDiskFields[0][2], localDiskFields[0][3],
                localDiskFields[0][4], localDiskFields[0][5], disks);

                /* Execute SQL statement */
                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
        
                if (rc == SQLITE_OK) {
                    int step = sqlite3_step(res);
                    sqlite3_finalize(res);
                    rc = 0;
                } else {
                    err("Failed to execute statement: %s\n", sqlite3_errmsg(db));
                    rc = -1;
                }
                free(sql);
            }

            return disks;
        }
      }
    if(strcmp(localDiskFields[0][0], "") != 0){
        disks = i;

        char* sql = malloc(sql_length+1);
        memset(sql, 0, sql_length+1);
        if (sql == NULL) {
            err("Failed to alloc sql format string\n");
            return -1;
        }

        /* Create merged SQL statement */
        snprintf(sql, sql_length, "INSERT OR REPLACE INTO %s (ID, hostname, ipAddress, diskName, diskSize, diskUsed, diskAvailable, diskUsePercent, diskMount, numDisks) VALUES ((SELECT ID FROM %s WHERE ipAddress = \"%s\" AND diskName = \"%s\"),'%s','%s','%s','%s','%s','%s', '%s','%s', '%d');",
            table, table, ipAddress, localDiskFields[0][0],
            hostname, ipAddress, localDiskFields[0][0],
            localDiskFields[0][1], localDiskFields[0][2], localDiskFields[0][3],
            localDiskFields[0][4], localDiskFields[0][5], disks);

        /* Execute SQL statement */
        rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
        
        if (rc == SQLITE_OK) {
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
            rc = 0;
        } else {
            err("Failed to execute statement: %s\n", sqlite3_errmsg(db));
            rc = -1;
        }
        free(sql);
    }
err:
    return disks;
}

int update_database_boards(
        sqlite3 *db,
        char *table,
        char *hostname,
        char *ipAddress,
        char *localBoardFields)
{
    char *zErrMsg = 0;
    int rc;
    int sql_length = 1028;
    sqlite3_stmt *res;
    const unsigned char *ret_value;
    const char *data = "Callback function called";

        char* sql = malloc(sql_length+1);
        memset(sql, 0, sql_length+1);
        if (sql == NULL) {
            err("Failed to alloc sql format string\n");
            return -1;
        }

        /* Create merged SQL statement */
        snprintf(sql, sql_length, "INSERT OR REPLACE INTO %s (ID, hostname, ipAddress, scriptOutput) VALUES ((SELECT ID FROM %s WHERE hostname = \"%s\" AND ipAddress = \"%s\"),'%s','%s','%s');",
           table, table, hostname, ipAddress,
           hostname, ipAddress, localBoardFields);

        /* Execute SQL statement */
        rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
        
        if (rc == SQLITE_OK) {
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
            rc = 0;
        } else {
            err("Failed to execute statement: %s\n", sqlite3_errmsg(db));
            rc = -1;
        }
        free(sql);
err:
    return rc;
}

void popenCommand(char * command, char * inString){
  FILE *fp;
  char path[1035];

  /* Open the command for reading. */
  fp = popen(command, "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to run shell command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    strcpy(inString, path);
  }

  /* close */
  pclose(fp);
}

int popenDisksCommand(char * command, char inString[MAX_DISKS][64], char stringTokens[MAX_DISKS][DISK_FIELDS][16]){
  FILE *fp;
  char path[1035];
  char* pch;
  int iterator = 0, x = 0;

  /* Open the command for reading. */
  fp = popen(command, "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to run shell command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    if(iterator < MAX_DISKS){
        strcpy(inString[iterator], path);
        iterator++;
    }
  }

  /* close */
  pclose(fp);

  for(int i = 0; i < MAX_DISKS; i++){
    if(i < iterator){  
        pch = strtok (inString[i]," \n");
        x = 0;
        while (pch != NULL)
        {
            strcpy(stringTokens[i][x], pch);
            pch = strtok (NULL, " \n");
            x++;
        }
    }
  }
  printf("\n\n");
  return iterator;
}

void popenBoardsCommand(char * command, char * inString){
   FILE *fp;
  char path[1035];
  memset(inString, 0, 1024);
  /* Open the command for reading. */
  fp = popen(command, "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to run shell command\n" );
    exit(1);
  }
  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
      fprintf(stderr, "Current Path: %s\n", path);
    strcat(inString, path);
    //strcat(inString, "\n");
  }

  /* close */
  pclose(fp);
}

void ping_local_network(char local_ip[16], app_info_t *app, int cur_broadcast){
    FILE *fp;
    FILE *lanScan;
    char path[1035];
    char command[32];
    time_t clk = time(NULL);
    app_config_t *cfg = &app->cfg;
    char *filename;

    filename = cfg->text_file_name[cur_broadcast];
    
    memset(command, 0, 32);
    strcpy(command, "nmap -sP ");
    strcat(command, local_ip);
    /* Open the command for reading. */
    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to run command \"%s\", %s\n", command, strerror(errno));
    } else {
        /* Open up text file to save the nmap results */
        lanScan = fopen(filename, "w");
        fprintf(lanScan, ctime(&clk));
        /* Read the output a line at a time - output it. */
        while (fgets(path, sizeof(path)-1, fp) != NULL) {
            fprintf(lanScan, path);
        }
        /* close */
        fprintf(stderr, "Closing lanScan...\n");
        fclose(lanScan);
        fprintf(stderr, "lanScan closed\n");
        if(pclose(fp) == -1){
            fprintf(stderr, "File fp not closed correctly.\n");
        }
    }    
}

void get_main_interface(char * main_interface){
    char command[124];
    
    strcpy(command, "route | awk 'NR==3{printf \"%s\", $8}'");
    popenCommand(command, main_interface);
}

void get_primary_interface(char interface[16]){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    char check_interface[16];
    int iterator = 0;

    getifaddrs(&ifAddrStruct);

    get_main_interface(check_interface);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, check_interface)==0) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            strcpy(interface, ifa->ifa_name);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);

            return;
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);

    return;
}

void getInterfaces(char interfaces[MAX_INTERFACES][MAX_INTERFACE_NAME_LENGTH], char ip4[MAX_INTERFACES][16]){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    char check_interface[16];
    int iterator = 1;

    getifaddrs(&ifAddrStruct);

    get_main_interface(check_interface);
    strcpy(interfaces[iterator], check_interface);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "lo")!=0) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            
            if(iterator < MAX_INTERFACES){
                if(strcmp(ifa->ifa_name, check_interface)==0){
                    strcpy(interfaces[0], ifa->ifa_name);
                    strcpy(ip4[0], addressBuffer);
                } else {
                    strcpy(interfaces[iterator], ifa->ifa_name);
                    strcpy(ip4[iterator], addressBuffer);
                }
                
            }

            //fprintf(stderr, "%s IP Address %s\n", ifa->ifa_name, addressBuffer);
            iterator++;
        } else {
            if(iterator < MAX_INTERFACES){
                memset(interfaces[iterator], 0, MAX_INTERFACE_NAME_LENGTH);
            }
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}

//==============================================================================
// Load Config function
//==============================================================================

void load_config(char configSettings[NUM_CONFIG_OPTIONS][16], app_info_t *app){
    app_config_t *cfg = &app->cfg;
    char *filename;
    FILE *fp;
    char Setting[30];
    char Value[30];
    char curLine[MAX_LINE_LENGTH+1];
    int curScan = 0;

    filename = cfg->config_file_name;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "error opening config file: %s\n", filename);
        return;
    } 
    else {
        printf("Config file opened.\n");
        while (fgets(curLine, sizeof(curLine), fp)){
            if(curLine[0] != '#'){
              curScan = sscanf(curLine, "%[^=]=%s", &Setting, &Value);
              if(strcmp(Setting,"MulticastIP") == 0){
                strcpy(configSettings[0],Value);
                fprintf(stderr,"MulticastIP Set: %s\n", configSettings[0]);
              }
              else if(strcmp(Setting,"MulticastPort") == 0){
                strcpy(configSettings[1],Value);
                fprintf(stderr,"MulticastPort Set: %s\n", configSettings[1]);
              }
              else if(strcmp(Setting,"TcpIP") == 0){
                strcpy(configSettings[2],Value);
                fprintf(stderr,"TCP IP Set: %s\n", configSettings[2]);
              }
              else if(strcmp(Setting,"TcpPort") == 0){
                strcpy(configSettings[3],Value);
                fprintf(stderr,"TCP Port Set: %s\n", configSettings[3]);
              }
              else if(strcmp(Setting,"UpdateRate") == 0){
                strcpy(configSettings[4],Value);
                fprintf(stderr,"Update Rate Set: %s\n", configSettings[4]);
              }
              else if(strcmp(Setting,"ScanRate") == 0){
                strcpy(configSettings[5],Value);
                fprintf(stderr,"LAN Ping Rate Set: %s\n", configSettings[5]);
              }
              else if(strcmp(Setting,"MaxInterfaces") == 0){
                strcpy(configSettings[6],Value);
                fprintf(stderr,"Max Interfaces Set: %s\n", configSettings[6]);
              }
              else if(strcmp(Setting,"MaxDisks") == 0){
                strcpy(configSettings[7],Value);
                fprintf(stderr,"Max Disks Set: %s\n", configSettings[7]);
              }
              else if(strcmp(Setting,"lanIP") == 0){
                strcpy(configSettings[8],Value);
                fprintf(stderr,"Primary Broadcast IP Set: %s\n", configSettings[8]);
                NUM_BROADCASTS++;
              }
              else if(strcmp(Setting,"AltIP") == 0){
                strcpy(configSettings[9],Value);
                fprintf(stderr,"Secondary Broadcast IP Set: %s\n", configSettings[9]);
                NUM_BROADCASTS++;
              }
              else if(strcmp(Setting,"AltIP2") == 0){
                strcpy(configSettings[10],Value);
                fprintf(stderr,"Third Broadcast IP Set: %s\n", configSettings[10]);
                NUM_BROADCASTS++;
              }
              else if(strcmp(Setting,"AltIP3") == 0){
                strcpy(configSettings[11],Value);
                fprintf(stderr,"Fourth Broadcast IP Set: %s\n", configSettings[11]);
                NUM_BROADCASTS++;
              }
            }
        }
        printf("\n");
        fclose(fp);
     }
}

void check_mode_setting(app_info_t *app){
    app_config_t *cfg = &app->cfg;
    char *filename;
    FILE *fp;
    char Setting[30];
    char Value[30];
    char curLine[MAX_LINE_LENGTH+1];
    int curScan = 0;

    filename = cfg->config_file_name;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "error opening config file: %s\n", filename);
        return;
    } 
    else {
        while (fgets(curLine, sizeof(curLine), fp)){
            if(curLine[0] != '#'){
              curScan = sscanf(curLine, "%[^=]=%s", &Setting, &Value);
              if(strcmp(Setting,"RunMode") == 0){
                if(atoi(Value) == 1){
                    cfg->server = 1;
                    fprintf(stderr, "Server mode set through config.\n");
                }
              }
            }
        }
        printf("\n");
        fclose(fp);
     }
}

//========================================================================================
// TCP functions
//========================================================================================

void Server_Establish_TCP_Connection(unsigned short tcp_port, char* tcp_addr_str, sqlite3 *db, int updateRate, int sockfd, int portno, struct sockaddr_in serv_addr, struct sockaddr_in cli_addr){
     int newsockfd;
     socklen_t clilen;
     char hostbufHold[128];
     char ipbufHold[16];
     char buffer[1200];
     char bytes_to_receive;
     char received_bytes;
     struct packet
     {
        char hostbuf[128];              /* Buffer used to hold the hostname of the local machine */
        char inbuf[MAX_INTERFACES][MAX_INTERFACE_NAME_LENGTH]; /* Buffer used to hold the name of the interface */
        char ipbuf[MAX_INTERFACES][16]; /* Buffer used to hold the IP address of the interface */
        char model[64];                 /* Buffer used to hold the CPU Model of the local machine */
        char totalmem[64];              /* Buffer used to hold the Total Memory of the local machine */
        char usedmem[32];               /* Buffer used to hold the Used Memory of the local machine */
        char freemem[32];               /* Buffer used to hold the Free Memory of the local machine */
        char availablemem[32];          /* Buffer used to hold the Available Memory of the local machine */
        char RAMmem[32];                /* Buffer used to hold the Total RAM Size of the local machine */
        char localDiskFields[MAX_DISKS][DISK_FIELDS][16];
        int disks;
        int length;
     };
     struct scriptPacket 
     {
        char localBoardFields[1024];    /* Buffer used to hold the local boards detected */
     };
     struct in_addr **addr_list;
     struct hostent *hostentry;
     int n = 1;
     int numInterfaces = 0;
     struct timeval timeout;      
     timeout.tv_sec = 15;
     timeout.tv_usec = 0;
     int iterator = 0;
     time_t clk = time(NULL);

     struct packet *EmptyPacket = (struct packet *) buffer;
     struct packet *pkt = (struct packet *) buffer;
     struct scriptPacket *EmptyScriptPacket = (struct scriptPacket *) buffer;
     struct scriptPacket *scriptPkt = (struct scriptPacket *) buffer;
     char *payload = buffer + sizeof(*pkt);
     long int packet_size;

     bytes_to_receive = sizeof(*pkt);
     received_bytes = 0;
     memset(buffer, 0, 1200);

     signal(SIGALRM, updateTrigger);
     alarm(updateRate);

     do{
         /* If a new slave messages the server, update the database */
         /* Every 24 hours, update the database */
         //fprintf(stderr, "Waiting for connection...\n");
         listen(sockfd,5);
         clilen = sizeof(cli_addr);
         newsockfd = accept(sockfd, 
                  (struct sockaddr *) &cli_addr, 
                   &clilen);
       /* Check for timeout */
       if(newsockfd != -1){

         printf("Connection made!\n");
        if (newsockfd < 0) 
              error("ERROR on accept");
         
         if (n < 0) error("ERROR reading from socket");
         n = write(newsockfd,"Server: Send data",26);
         if (n < 0) error("ERROR writing to socket");
         if(n == 0){
            perror("TIMEOUT writing to socket");
            close(newsockfd);
            n = 1;
         }
         else {
            //fprintf(stderr, "Waiting for packet...\n");
            if (recv(newsockfd, pkt, sizeof(*pkt), 0) <= 0)
                perror("ERROR, there was a problem in reading the data");
            else
            {       
                    do {
                        received_bytes += (buffer + received_bytes, bytes_to_receive - received_bytes);
                    } while (received_bytes != bytes_to_receive);
        
                    fprintf(stderr, "Data Received from %s, at %s", pkt->hostbuf, ctime(&clk));
                    //fprintf(stderr, "Hostname: %s\n", pkt->hostbuf);
                    //for(int i = 0; i < MAX_INTERFACES; i++){
                    //    if(strlen(pkt->inbuf[i]) != 0){
                    //        fprintf(stderr, "Interface: %s\n", pkt->inbuf[i]);
                    //        fprintf(stderr, "Interface IP: %s\n", pkt->ipbuf[i]);
                    //    }
                    //}
                    //fprintf(stderr, "CPU: %s", pkt->model);
                    //fprintf(stderr, "Total Memory: %s\n", pkt->totalmem);
                    //fprintf(stderr, "Used Memory: %s\n", pkt->usedmem);
                    //fprintf(stderr, "Free Memory: %s\n", pkt->freemem);
                    //fprintf(stderr, "Available Memory: %s\n", pkt->availablemem);
                    //fprintf(stderr, "RAM Memory: %s\n", pkt->RAMmem);
                    //fprintf(stderr, "Number of disks detected: %d\n", pkt->disks);
                    //fprintf(stderr, "Size of Packet Received: %d\n\n", pkt->length);
         
     
     
                    if (n < 0) error("ERROR reading from socket");
                    //printf("Here is the message: %s\n",buffer);
                    n = write(newsockfd,"Server: I got your message",26);
                    if (n < 0) error("ERROR writing to socket");

                    for(int i = 0; i < MAX_INTERFACES; i++){
                        if(strlen(pkt->ipbuf[i]) != 0){
                            numInterfaces++;
                        }
                    }

                    pkt->disks = update_database_disks(db, "labDisks", pkt->hostbuf, pkt->ipbuf[0],
                                pkt->localDiskFields, pkt->disks);
                    update_database_index(db, "labAudit", pkt->hostbuf, pkt->inbuf[0], pkt->ipbuf[0],
                                pkt->model, pkt->totalmem, pkt->usedmem, 
                                pkt->freemem, pkt->availablemem, pkt->RAMmem, pkt->disks);
                    update_database_interfaces(db, "labInterfaces", pkt->hostbuf, pkt->inbuf,
                                pkt->ipbuf, numInterfaces);
                    
                    strcpy(hostbufHold, pkt->hostbuf);
                    strcpy(ipbufHold, pkt->ipbuf[0]);

                    bytes_to_receive = sizeof(*scriptPkt);
                    received_bytes = 0;
                    memset(buffer, 0, 1200);

                    if (recv(newsockfd, scriptPkt, sizeof(*scriptPkt), 0) <= 0)
                        perror("ERROR, there was a problem in reading the data");
                    else {
                        do {
                           received_bytes += (buffer + received_bytes, bytes_to_receive - received_bytes);
                        } while (received_bytes != bytes_to_receive);

                        n = write(newsockfd,"Server: I got your message",26);
                        if (n < 0) error("ERROR writing to socket");

                        update_database_boards(db, "labBoards", hostbufHold, ipbufHold,
                                scriptPkt->localBoardFields);
                    }
                    numInterfaces = 0;
                    pkt = EmptyPacket;
                    scriptPkt = EmptyScriptPacket;
                    bytes_to_receive = sizeof(*pkt);
                    received_bytes = 0;
                    memset(buffer, 0, 1200);

            }
            close(newsockfd);
         }
        }
        else {
            if(strcmp(strerror( errno ), "Resource temporarily unavailable") != 0 &&
                    strcmp(strerror( errno ), "Interrupted system call") != 0)
            {
                perror("ERROR accepting client");
                newsockfd = 0;
            }
        }

     } while (!updateMark);
         
     close(newsockfd);
     return; 
}

void Client_Establish_TCP_Connection(unsigned short tcp_port, char* tcp_addr_str, char * host, char * interface){
    int sockfd, portno, ret;
    int n = 1;
    int failed_connect_count = 0;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct hostent *hostentry;
    struct packet
    {
        char hostbuf[128];              /* Buffer used to hold the hostname of the local machine */
        char inbuf[MAX_INTERFACES][MAX_INTERFACE_NAME_LENGTH]; /* Buffer used to hold the name of the interface */
        char ipbuf[MAX_INTERFACES][16]; /* Buffer used to hold the IP address of the interface */
        char model[64];                 /* Buffer used to hold the CPU Model of the local machine */
        char totalmem[64];              /* Buffer used to hold the Total Memory of the local machine */
        char usedmem[32];               /* Buffer used to hold the Used Memory of the local machine */
        char freemem[32];               /* Buffer used to hold the Free Memory of the local machine */
        char availablemem[32];      /* Buffer used to hold the Available Memory of the local machine */
        char RAMmem[32];            /* Buffer used to hold the Total RAM Size of the local machine */
        char localDiskFields[MAX_DISKS][DISK_FIELDS][16];
        int disks;
        int length;
    };
    struct scriptPacket 
    {
        char localBoardFields[1024];    /* Buffer used to hold the local boards detected */
    };
    char command[124];
    char localDisks[128];
    char localBoards[32];
    time_t clk = time(NULL);
    int status = 1;
    struct timeval tv;
    tv.tv_sec = 30;  /* 30 Secs Timeout */
    tv.tv_usec = 0;  // Not init'ing this can cause strange errors

    portno = tcp_port;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(tcp_addr_str);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);

    serv_addr.sin_port = htons(portno);

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

    while(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){ 
        failed_connect_count += 1;
        if(failed_connect_count > 10){
            fprintf(stderr, "Server not responding, connection timeout...\n");
            close(sockfd);
            return;
        }
        fprintf(stderr, "Unable to connect to server, retrying...\n");
        sleep(CLIENT_RETRY_CONNECTION_TIMER);
    }   

    char buffer[1200];
    struct packet *pkt = (struct packet *) buffer;
    struct scriptPacket *scriptPkt = (struct scriptPacket *) buffer;
    char *payload = buffer + sizeof(*pkt);
    long int packet_size;

    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,1200);
    n = read(sockfd,buffer,1200);
    if (n == -1 || n == 0){ 
         perror("ERROR reading from socket");
         fprintf(stderr, "Attempting reconnect...\n");
        close(sockfd);

        sleep(3);

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");
        server = gethostbyname(tcp_addr_str);
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
        serv_addr.sin_port = htons(portno);

         if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) != -1){
            n = read(sockfd,buffer,1200);
            if(n == -1 || n == 0){
                perror("ERROR reading from socket, second attempt");
                close(sockfd);
                return;
            }
         }
         else{
            perror("Reconnect failed");
            close(sockfd);
            return;
         }
    }
    fprintf(stderr,"\n%s\n",buffer);
    
    for(int i = 0; i < MAX_INTERFACES; i++){
        memset(pkt->inbuf[i], 0, 16);
        memset(pkt->ipbuf[i], 0, 16);
    }


    fprintf(stderr,"Started Creating packet\n");
    //ret = gethostname(pkt->hostbuf, sizeof(pkt->hostbuf));
    //hostentry = gethostbyname(pkt->hostbuf);

    getInterfaces(pkt->inbuf, pkt->ipbuf);
     
    pkt->length = sizeof(*pkt);

    // Begin grabbing system data
    strcpy(command, "cat /proc/cpuinfo | grep -m 1 'model name' | awk '{for(i=4;i<=NF;++i)printf \"%s \", $i}'");
    popenCommand(command, pkt->model);
    
    strcpy(command, "free -m | awk 'NR==2{printf \"%sMB (%.2f%% USED)\", $2,$3*100/$2 }'");
    popenCommand(command, pkt->totalmem);
    
    strcpy(command, "free -m | awk 'NR==2{printf \"%sMB \", $3 }'");
    popenCommand(command, pkt->usedmem);
    
    strcpy(command, "free -m | awk 'NR==2{printf \"%sMB \", $4 }'");
    popenCommand(command, pkt->freemem);
    
    strcpy(command, "cat /proc/meminfo | awk 'NR==3{printf \"%sMB \", $2/1024 }'");
    popenCommand(command, pkt->availablemem);
    
    strcpy(command, "cat /proc/meminfo | awk 'NR==1{printf \"%sMB \", $2/1024}'");
    popenCommand(command, pkt->RAMmem); 

    strcpy(command, "df -h | grep '/dev/sd'");
    pkt->disks = popenDisksCommand(command, localDisks, pkt->localDiskFields);

    strcpy(command, "hostname | awk 'NR==1{printf \"%s\", $1}'");
    popenCommand(command, pkt->hostbuf);

    fprintf(stderr, "\nHostname: %s Interface: %s Interface IP: %s\n", pkt->hostbuf, pkt->inbuf[0], pkt->ipbuf[0]);
    
    fprintf(stderr, "Sending data for %s, at %s", pkt->hostbuf, ctime(&clk));
            fprintf(stderr, "Hostname: %s\n", pkt->hostbuf);
            for(int i = 0; i < MAX_INTERFACES; i++){
                if(strlen(pkt->inbuf[i]) != 0){
                    fprintf(stderr, "Interface: %s\n", pkt->inbuf[i]);
                    fprintf(stderr, "Interface IP: %s\n", pkt->ipbuf[i]);
                }
            }
            fprintf(stderr, "CPU: %s\n", pkt->model);
            fprintf(stderr, "Total Memory: %s\n", pkt->totalmem);
            fprintf(stderr, "Used Memory: %s\n", pkt->usedmem);
            fprintf(stderr, "Free Memory: %s\n", pkt->freemem);
            fprintf(stderr, "Available Memory: %s\n", pkt->availablemem);
            fprintf(stderr, "RAM Memory: %s\n", pkt->RAMmem);
            fprintf(stderr, "Size of 3D disk array: %d\n", sizeof(pkt->localDiskFields));
            fprintf(stderr, "Number of disks detected: %d\n", pkt->disks);
            fprintf(stderr, "Size of Packet Sent: %d\n", pkt->length);

    if(send(sockfd, pkt, sizeof(*pkt), 0) < 0)
       error("ERROR sending packet");
    else
       fprintf(stderr, "Done sending packet\n");
       
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,1200);
    n = read(sockfd,buffer,1200);
    if (n == -1 || n == 0) {
         perror("ERROR reading from socket");
         close(sockfd);
         return;
    }

    strcpy(command, "deltacast_board_info");
    popenBoardsCommand(command, scriptPkt->localBoardFields);

    sleep(1);

    if(send(sockfd, scriptPkt, sizeof(*scriptPkt), 0) < 0)
       error("ERROR sending packet");
    else
       fprintf(stderr, "Done sending script results packet\n");

    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,1200);
    n = read(sockfd,buffer,1200);
    if (n == -1 || n == 0) {
         perror("ERROR reading from socket");
         close(sockfd);
         return;
    }

    fprintf(stderr,"%s\n\n",buffer);
    close(sockfd);
    return;

}

//========================================================================================
// Run mode functions
//========================================================================================

int run_server(app_info_t *app) 
{
  char configSettings[NUM_CONFIG_OPTIONS][16];
  int updateRate, scanRate;
  int scanMark = 0;
  int rc;
  int zErrMsg;
  char command[124];
  char commandResult[124];
  int n = 1;
  int sockfd, portno;
  int sockMC, sockTCP;              /* socket descriptor */
  char send_str[MAX_LEN];           /* string to send */
  struct sockaddr_in mc_addr;       /* socket address structure */
  unsigned int send_len;            /* length of string to send */
  char* mc_addr_str;                /* multicast IP address */
  char* tcp_addr_str;               /* TCP IP address */
  char local_ip[MAX_BROADCAST][16]; /* Broadcast Network IPs */
  unsigned short mc_port;           /* multicast port */
  unsigned short tcp_port;          /* TCP port */
  unsigned char mc_ttl=15;          /* time to live (hop count) */
  char interface[16];               /* name of interface, like "eth0" */
  struct ifreq ifr;
  struct ifaddrs *ifaddr;
  struct ifaddrs *ifa;
  int family, s;
  char host[NI_MAXHOST];
  struct sockaddr_in recv_addr;
  struct sockaddr_in serv_addr, cli_addr;
  struct in_addr eth0_address; // bit of a misnomer, it should be called interface_address
                               // because it is the address of the interface
  char eth_name[MAX_INTERFACE_NAME_LENGTH];
  struct ip_mreq multicastreq;
  struct hostent *he;
  struct timeval timeout;      
     timeout.tv_sec = 15;
     timeout.tv_usec = 0;
  sqlite3 *db;

  load_config(configSettings, app);
  rc = sqlite3_open("/etc/lanManager/packetTest.db", &db);
  strcpy(command, "chmod 777 /etc/lanManager/packetTest.db");
    popenCommand(command, commandResult);
  open_database(db, app, rc);
  
  mc_addr_str    = configSettings[0];       /* multicast IP address */
  mc_port        = atoi(configSettings[1]); /* multicast port number */
  tcp_addr_str   = configSettings[2];       /* TCP ip address */
  tcp_port       = atoi(configSettings[3]); /* TCP port number */
  updateRate     = atoi(configSettings[4]); /* Server update rate */
  scanRate       = atoi(configSettings[5]); /* Server local area scan rate */
  MAX_INTERFACES = atoi(configSettings[6]); /* Max interfaces */
  MAX_DISKS      = atoi(configSettings[7]); /* Max disks */
  for (int broadcast_count = 0; broadcast_count < NUM_BROADCASTS; broadcast_count++) {
    /* Local Area Network IP */
    strcpy(local_ip[broadcast_count], configSettings[(8+broadcast_count)]);
  } 
  get_primary_interface(interface);         /* Interface to bind */

  /* validate the multicast port and TCP port numbers */
  if(mc_port == tcp_port){
    fprintf(stderr, "Invalid Multicast/TCP port number argument %d.\n",
            mc_port);
    fprintf(stderr, "Multicast port and TCP port must be two different ports.");
  }

  /* validate the multicast port range */
  if ((mc_port < MIN_PORT) || (mc_port > MAX_PORT)) {
    fprintf(stderr, "Invalid Multicast port number argument %d.\n",
            mc_port);
    fprintf(stderr, "Valid range is between %d and %d.\n",
            MIN_PORT, MAX_PORT);
    exit(1);
  }

  /* validate the TCP port range */
  if ((tcp_port < MIN_PORT) || (tcp_port > MAX_PORT)) {
    fprintf(stderr, "Invalid TCP port number argument %d.\n",
            mc_port);
    fprintf(stderr, "Valid range is between %d and %d.\n",
            MIN_PORT, MAX_PORT);
    exit(1);
  }

  /* create a socket for sending to the multicast address */
  if ((sockMC = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket() failed");
    exit(1);
  }
  
  /* set the TTL (time to live/hop count) for the send */
  if ((setsockopt(sockMC, IPPROTO_IP, IP_MULTICAST_TTL, 
       (void*) &mc_ttl, sizeof(mc_ttl))) < 0) {
    perror("setsockopt() failed");
    exit(1);
  } 

  /* bind to interface is interface is specified */
  if (interface) {
    fprintf(stderr, "bind to interface %s\n", interface);

    snprintf(eth_name, MAX_INTERFACE_NAME_LENGTH, "%s", interface);
    fprintf(stderr, "bind to eth_name %s\n", eth_name);

    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);

    rc = setsockopt(sockMC, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr));
    if (rc < 0) {
      perror("setsockopt() bindtodevice failed");
      exit(1);
    }


    if (getifaddrs(&ifaddr) == -1) {      
        close(sockMC);

        perror("getifaddrs failure");
        exit(1);
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr) {
            family = ifa->ifa_addr->sa_family;        
        }

        if ((!strcasecmp(ifa->ifa_name, eth_name)) && (family == AF_INET)) {
            s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);               
        }               
    }

    freeifaddrs(ifaddr); 
    inet_aton(host, &eth0_address);
    recv_addr.sin_addr.s_addr = eth0_address.s_addr;

    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(INADDR_ANY);


    rc = bind(sockMC, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
    if (rc < 0) {
      perror("unable to bind");
      close(sockMC);
      exit(1);
    }

    multicastreq.imr_multiaddr.s_addr = eth0_address.s_addr;
    rc = setsockopt(sockMC, IPPROTO_IP, IP_MULTICAST_IF, (char*)&eth0_address, sizeof(eth0_address));
    if (rc < 0) {
      fprintf(stderr,"unable to lock to interface for multicast\n");
    } else {
      fprintf(stderr,"locking multicast to output for : %s\n", host);
    }



  }

  
  /* construct a multicast address structure */
  memset(&mc_addr, 0, sizeof(mc_addr));
  mc_addr.sin_family      = AF_INET;
  mc_addr.sin_addr.s_addr = inet_addr(mc_addr_str);
  mc_addr.sin_port        = htons(mc_port);

  //printf(">> Begin typing (return to send, ctrl-C to quit):\n");

  strcpy(tcp_addr_str, host);

  /* clear send buffer */
  memset(send_str, 0, sizeof(send_str));
  
  fprintf(stderr, "\nProcessing...\n");
  /* Ping the local network before accepting client info */
  for (int cur_broadcast = 0; cur_broadcast < NUM_BROADCASTS; cur_broadcast++) {
    fprintf(stderr, "Pinging %s\n", local_ip[cur_broadcast]);
    ping_local_network(local_ip[cur_broadcast], app, cur_broadcast);
  }
  
  /* Send out a message on the multicast port to poll each active client */ 
  strcpy(send_str, host);
  send_len = strlen(send_str);

  if ((sendto(sockMC, send_str, send_len, 0, 
         (struct sockaddr *) &mc_addr, 
         sizeof(mc_addr))) != send_len) {
      perror("sendto() sent incorrect number of bytes");
      exit(1);
  }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = tcp_port;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) == -1){
         error("ERROR on setsockopt\n");
     }
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
            error("ERROR on binding");

     /* Set socket timeout options */
     if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        error("ERROR on timeout setsockopt\n");
     if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        error("ERROR on timeout setsockopt\n");

  /* clear send buffer */
  memset(send_str, 0, sizeof(send_str));

  for (;;) {
    check_log_rotation(app);
    /* make TCP connection and transfer data*/
    fprintf(stderr, "Attempting to establish TCP connection.\n\n");
    Server_Establish_TCP_Connection(tcp_port, tcp_addr_str, db, updateRate, sockfd, portno, serv_addr, cli_addr);
    fprintf(stderr, "Broadcasting to multicast port..\n");
    updateMark = 0;
    scanMark++;
    strcpy(send_str, host);
    send_len = strlen(send_str);
    
    /* scan LAN if the scanRate time has elapsed */
    if(scanMark >= scanRate){
        for (int cur_broadcast = 0; cur_broadcast < NUM_BROADCASTS; cur_broadcast++) {
            fprintf(stderr, "Pinging %s\n", local_ip[cur_broadcast]);
            ping_local_network(local_ip[cur_broadcast], app, cur_broadcast);
        }
        scanMark = 0;
    }
    
    /* Poll clients on the multicast port */
    if ((sendto(sockMC, send_str, send_len, 0, 
         (struct sockaddr *) &mc_addr, 
         sizeof(mc_addr))) != send_len) {
      perror("sendto() sent incorrect number of bytes");
      exit(1);
    }

    /* clear send buffer */
    memset(send_str, 0, sizeof(send_str));
  }

  close(sockfd);
  close(sockMC);
  close_database(db);

  exit(0);
}

int run_client(app_info_t *app) 
{
  char configSettings[NUM_CONFIG_OPTIONS][16];
  int rc;
  int sockMC, sockTCP;              /* socket descriptor */
  int flag_on = 1;                  /* socket option flag */
  struct sockaddr_in mc_addr;       /* socket address structure */
  char recv_str[MAX_LEN+1];         /* buffer to receive string */
  int recv_len;                     /* length of string received */
  struct ip_mreq mc_req;            /* multicast request structure */
  char* mc_addr_str;                /* multicast IP address */
  char* tcp_addr_str;               /* TCP IP address */
  unsigned short mc_port;           /* multicast port */
  unsigned short tcp_port;          /* TCP port */
  struct sockaddr_in from_addr;     /* packet source */
  unsigned int from_len;            /* source addr length */
  char interface[16];           /* name of interface, like "eth0" */
  struct ifreq ifr;
  struct ifaddrs *ifaddr;
  struct ifaddrs *ifa;
  int family, s;
  char host[NI_MAXHOST];
  struct sockaddr_in recv_addr;
  struct in_addr eth0_address; // bit of a misnomer, it should be called interface_address
                               // because it is the address of the interface
  char eth_name[MAX_INTERFACE_NAME_LENGTH];
  struct ip_mreq multicastreq;

  load_config(configSettings, app);
  
  mc_addr_str    = configSettings[0];       /* multicast IP address */
  mc_port        = atoi(configSettings[1]); /* multicast port number */
  tcp_addr_str   = configSettings[2];       /* TCP ip address */
  tcp_port       = atoi(configSettings[3]); /* TCP port number */
  MAX_INTERFACES = atoi(configSettings[6]); /* Max interfaces */
  MAX_DISKS      = atoi(configSettings[7]); /* Max disks */
  
  get_primary_interface(interface);
  printf("\nConfigurations set...\n");


  /* validate the multicast port and TCP port numbers */
  if(mc_port == tcp_port){
    fprintf(stderr, "Invalid Multicast/TCP port number argument %d.\n",
            mc_port);
    fprintf(stderr, "Multicast port and TCP port must run be two different ports.");
  }

  /* validate the multicast port range */
  if ((mc_port < MIN_PORT) || (mc_port > MAX_PORT)) {
    fprintf(stderr, "Invalid port number argument %d.\n",
            mc_port);
    fprintf(stderr, "Valid range is between %d and %d.\n",
            MIN_PORT, MAX_PORT);
    exit(1);
  }

  /* create socket to join multicast group on */
  if ((sockMC = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket() failed");
    exit(1);
  }
  
  /* validate the TCP port range */
  if ((tcp_port < MIN_PORT) || (tcp_port > MAX_PORT)) {
    fprintf(stderr, "Invalid TCP port number argument %d.\n",
            mc_port);
    fprintf(stderr, "Valid range is between %d and %d.\n",
            MIN_PORT, MAX_PORT);
    exit(1);
  }

  /* set reuse port to on to allow multiple binds per host */
  if ((setsockopt(sockMC, SOL_SOCKET, SO_REUSEADDR, &flag_on,
       sizeof(flag_on))) < 0) {
    perror("setsockopt() failed");
    exit(1);
  }

  /* bind to interface is interface is specified */
  if (interface) {
    fprintf(stderr, "bind to interface %s, port = %d\n", interface, mc_port);

    snprintf(eth_name, MAX_INTERFACE_NAME_LENGTH, "%s", interface);
    fprintf(stderr, "bind to eth_name %s\n", eth_name);

    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);

    rc = setsockopt(sockMC, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr));
    if (rc < 0) {
      perror("setsockopt() bindtodevice failed");
      exit(1);
    }

    if (getifaddrs(&ifaddr) == -1) {      
        close(sockMC);

        perror("getifaddrs failure");
        exit(1);
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr) {
            family = ifa->ifa_addr->sa_family;        
        }

        if ((!strcasecmp(ifa->ifa_name, eth_name)) && (family == AF_INET)) {
            s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);               
        }               
    }

    freeifaddrs(ifaddr); 
    inet_aton(host, &eth0_address);  

    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = inet_addr(mc_addr_str);
    recv_addr.sin_port = htons(mc_port);

    rc = bind(sockMC, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
    if (rc < 0) {
      perror("unable to bind");
      close(sockMC);
      exit(1);
    }

    /* construct an IGMP join request structure */
    mc_req.imr_multiaddr.s_addr = inet_addr(mc_addr_str);
    mc_req.imr_interface.s_addr = eth0_address.s_addr;
  
    /* send an ADD MEMBERSHIP message via setsockopt */
    if ((setsockopt(sockMC, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
         (void*) &mc_req, sizeof(mc_req))) < 0) {
      perror("setsockopt() IP_ADD_MEMERSHIP failed");
      exit(1);
    }
  } else {

    /* don't bind to a specific interface, choose any */
    fprintf(stderr, "bind to any interface, port = %d\n", mc_port);

    /* construct a multicast address structure */
    memset(&mc_addr, 0, sizeof(mc_addr));
    mc_addr.sin_family      = AF_INET;
    mc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    mc_addr.sin_port        = htons(mc_port);
  
    /* bind to multicast address to socket */
    if ((bind(sockMC, (struct sockaddr *) &mc_addr, 
         sizeof(mc_addr))) < 0) {
      perror("bind() failed");
      exit(1);
    }

    /* construct an IGMP join request structure */
    mc_req.imr_multiaddr.s_addr = inet_addr(mc_addr_str);
    mc_req.imr_interface.s_addr = htonl(INADDR_ANY);
  
    /* send an ADD MEMBERSHIP message via setsockopt */
    if ((setsockopt(sockMC, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
         (void*) &mc_req, sizeof(mc_req))) < 0) {
      perror("setsockopt() failed");
      exit(1);
    }
  }

  //fprintf(stderr, "Startup: Attempting to establish a TCP connection to server\n");
  //Client_Establish_TCP_Connection(tcp_port, tcp_addr_str, host, interface);

  for (;;) {          /* loop forever */
    check_log_rotation(app);
    fprintf(stderr, "Waiting for server poll...\n");
    /* clear the receive buffers & structs */
    memset(recv_str, 0, sizeof(recv_str));
    from_len = sizeof(from_addr);
    memset(&from_addr, 0, from_len);

    /* block waiting to receive a packet */
    if ((recv_len = recvfrom(sockMC, recv_str, MAX_LEN, 0, 
         (struct sockaddr*)&from_addr, &from_len)) < 0) {
      perror("recvfrom() failed");
      exit(1);
    }

    /* output received string */
    fprintf(stderr, "\nReceived %d bytes from %s: ", recv_len, 
           inet_ntoa(from_addr.sin_addr));
    fprintf(stderr, "%s\n", recv_str);
    strcpy(tcp_addr_str, inet_ntoa(from_addr.sin_addr));
    
    /* reply back to host with IP, and additional information */
    fprintf(stderr, "Attempting to establish TCP connection\n");
    Client_Establish_TCP_Connection(tcp_port, tcp_addr_str, host, interface);

  }
  
  close(sockMC);
  /* send a DROP MEMBERSHIP message via setsockopt */
  if ((setsockopt(sockMC, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
       (void*) &mc_req, sizeof(mc_req))) < 0) {
    perror("setsockopt() failed");
    exit(1);
  }

    
}

// =====================================================================
// Program start
// =====================================================================

int main(int argc, char *argv[]){
    app_info_t *app = &app_info;
    app_config_t *cfg = &app->cfg;
    int rc;
    struct stat file_stats;
   
    init_app(&app_info);

    fprintf(stderr, "labManager (C) Copyright 2017 Igolgi Inc.  Proprietary and Confidential\n");
    fprintf(stderr,"labManager Version: %d.%d.%d (%s @ %s)\n\n", MAJOR_VER,MINOR_VER,POINT_VER, __DATE__, __TIME__);
    

    syslog(LOG_NOTICE, "labManager (C) Copyright 2017 Igolgi Inc.  Proprietary and Confidential\n");
    syslog(LOG_NOTICE,"labManager Version: %d.%d.%d (%s @ %s)\n\n", MAJOR_VER,MINOR_VER,POINT_VER, __DATE__, __TIME__);

    check_mode_setting(&app_info);

    rc = parse_cmd_line(&app_info, argc, argv);
    if (rc) {
        fprintf(stderr, "error parsing cmd line\n");
        exit(-1);
    }

    if (cfg->use_daemon) {
        daemonize(app);
    }
    
    // sleep first to give everything a chance to come up
    sleep(10);

    check_log_rotation(app);

    if(cfg->reset){
        reset_database(app);
    }
    if(cfg->client){
        fprintf(stderr, "Config setting overridden, running as client.\n");
        return run_client(app);
    }
    else if(cfg->server){
        return run_server(app);
    } 
    else{
        return run_client(app);
    }
    
    fprintf(stderr, "Running client program as slave...\n");
    return run_client(app);
    
    exit:
    close(app->log_fd);

    fprintf(stderr, "done\n");
    exit(0);
}


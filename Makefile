
# Where to install
INSTDIR = /etc
WEBDIR = /var/www/html
WEBDIRALT = /var/www
LAUNCHDIR = /etc/init.d/
# 
#

all: lanManager

install: lanManager
	@if [ -d $(INSTDIR) ]; \
	then \
		mkdir $(INSTDIR)/lanManager;\
		cp lanManager $(INSTDIR)/lanManager;\
		cp labAuditConfig.ini $(INSTDIR)/lanManager;\
		cp lanScan.txt $(INSTDIR)/lanManager;\
		cp lanScan2.txt $(INSTDIR)/lanManager;\
		cp lanScan3.txt $(INSTDIR)/lanManager;\
		cp lanScan4.txt $(INSTDIR)/lanManager;\
		cp packetTest.db $(INSTDIR)/lanManager;\
		chmod -R a+rwX $(INSTDIR)/lanManager;\
		chmod a+x $(INSTDIR)/lanManager/lanManager;\
		chmod og-w $(INSTDIR)/lanManager/lanManager;\
		echo "Installed lanManager in $(INSTDIR)/lanManager";\
		else \
		echo "Sorry, $(INSTDIR) does not exist";\
	fi ;
	@if [ -d $(WEBDIR) ]; \
	then \
		cp -avr lanman $(WEBDIR);\
		chmod -R a+rwX $(WEBDIR)/lanman;\
		chmod a+x $(WEBDIR)/lanman;\
		chmod og-w $(WEBDIR)/lanman;\
		echo "Webpage installed in $(WEBDIR)/lanman";\
		else \
		echo "Sorry, $(WEBDIR) does not exist"; \
	fi ;
	@if [ -d $(WEBDIRALT) ]; \
	then \
		cp -avr lanman $(WEBDIRALT);\
		chmod -R a+rwX $(WEBDIRALT)/lanman;\
		chmod a+x $(WEBDIRALT)/lanman;\
		chmod og-w $(WEBDIRALT)/lanman;\
		echo "Webpage installed in $(WEBDIRALT)/lanman";\
		else \
		echo "Sorry, $(WEBDIRALT) does not exist";\
	fi ;
	@if [ -d $(LAUNCHDIR) ] ; \
		then \
			cp lanManager.sh $(LAUNCHDIR);\
			chmod +x $(LAUNCHDIR)lanManager.sh;\
			update-rc.d lanManager.sh defaults;\
			else \
			echo "Sorry, $(LAUNCHDIR) does not exist"; \
	fi ;

uninstall: lanManager
	@if [ -d $(INSTDIR) ]; \
	then \
		rm $(INSTDIR)/lanManager/lanManager; \
		rm $(INSTDIR)/lanManager/labAuditConfig.ini; \
		rm $(INSTDIR)/lanManager/lanScan.txt; \
		rm $(INSTDIR)/lanManager/lanScan2.txt;\
		rm $(INSTDIR)/lanManager/lanScan3.txt;\
		rm $(INSTDIR)/lanManager/lanScan4.txt;\
		rm $(INSTDIR)/lanManager/packetTest.db; \
		rm -d $(INSTDIR)/lanManager/;\
		echo "Uninstalled from $(INSTDIR)";\
		else \
		echo "Sorry, $(INSTDIR) does not exist";\
	fi ;
	@if [ -d $(WEBDIR) ]; \
	then \
		rm $(WEBDIR)/lanman/activeIP.php; \
		rm $(WEBDIR)/lanman/disks.php; \
		rm $(WEBDIR)/lanman/index.php; \
		rm $(WEBDIR)/lanman/interfaces.php; \
		rm $(WEBDIR)/lanman/memory.php; \
		rm $(WEBDIR)/lanman/action_page.php; \
		rm $(WEBDIR)/lanman/description_page.php; \
		rm $(WEBDIR)/lanman/style.css; \
		rm $(WEBDIR)/lanman/igolgi_banner.jpg; \
		rm $(WEBDIR)/lanman/igolgi_banner.png; \
		rm $(WEBDIR)/lanman/lanManager_deb_1.0-1.deb; \
		rm -d $(WEBDIR)/lanman/; \
		echo "Webpage uninstalled from $(WEBDIR)";\
		else \
		echo "Sorry, $(WEBDIR) does not exist"; \
		fi ;
	@if [ -d $(WEBDIRALT) ]; \
	then \
		rm $(WEBDIRALT)/lanman/activeIP.php; \
		rm $(WEBDIRALT)/lanman/disks.php; \
		rm $(WEBDIRALT)/lanman/index.php; \
		rm $(WEBDIRALT)/lanman/interfaces.php; \
		rm $(WEBDIRALT)/lanman/memory.php; \
		rm $(WEBDIRALT)/lanman/action_page.php; \
		rm $(WEBDIRALT)/lanman/description_page.php; \
		rm $(WEBDIRALT)/lanman/style.css; \
		rm $(WEBDIRALT)/lanman/igolgi_banner.jpg; \
		rm $(WEBDIRALT)/lanman/igolgi_banner.png; \
		rm $(WEBDIRALT)/lanman/lanManager_deb_1.0-1.deb; \
		rm -d $(WEBDIRALT)/lanman/;\
		echo "Webpage uninstalled from $(WEBDIRALT)";\
		else \
		echo "Sorry, $(WEBDIRALT) does not exist";\
	fi ;
	@if [ -d $(LAUNCHDIR) ] ; \
		then \
			rm $(LAUNCHDIR)lanManager.sh;\
			rm /var/log/lanManager.sh.err;\
			rm /var/log/lanManager.sh.log;\
			rm /var/run/lanManager.sh.pid;\
			else \
			echo "Sorry, $(LAUNCHDIR) does not exist"; \
	fi ;

client_package: 
	sudo ./Makepackage_client.sh

server_package: 
	sudo ./Makepackage.sh

packages: client_package server_package

clean:
	rm -f lanManager

lanManager: lanManager.c
	gcc -o lanManager lanManager.c sqlite3.c -lpthread -ldl -lrt



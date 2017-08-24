DEBDIR="lanManager_client_$1"
INSTDIR="/etc"
WEBDIR="/var/www/html"
WEBDIRALT="/var/www"
LAUNCHDIR="/etc/init.d"
BINDIR="/usr/bin"

mkdir $DEBDIR
mkdir $DEBDIR/etc
mkdir $DEBDIR/etc/init.d/
mkdir $DEBDIR/etc/lanManager
mkdir $DEBDIR/usr
mkdir $DEBDIR/usr/bin/
mkdir $DEBDIR/usr/bin/lanManager

if [ -d $DEBDIR/var/www/.ssh ];
    then
        cp known_hosts $DEBDIR/var/www/.ssh
        chmod -R a+rwX $DEBDIR/var/www/.ssh
        chmod a+x $DEBDIR/var/www/.ssh
        chmod og-w $DEBDIR/var/www/.ssh
    fi
if [ -d $DEBDIR$INSTDIR ]; 
	then 
		cp lanManager $DEBDIR$INSTDIR/lanManager
		cp labAuditConfig.ini $DEBDIR$INSTDIR/lanManager
		cp lanScan.txt $DEBDIR$INSTDIR/lanManager
        cp lanScan2.txt $DEBDIR$INSTDIR/lanManager
        cp lanScan3.txt $DEBDIR$INSTDIR/lanManager
        cp lanScan4.txt $DEBDIR$INSTDIR/lanManager
		cp packetTest.db $DEBDIR$INSTDIR/lanManager
		chmod -R a+rwX $DEBDIR$INSTDIR/lanManager
		chmod a+x $DEBDIR$INSTDIR/lanManager/lanManager
		chmod og-w $DEBDIR$INSTDIR/lanManager/lanManager
		echo "Installed lanManager in $DEBDIR$INSTDIR/lanManager"
		else 
		echo "Sorry, $DEBDIR$INSTDIR does not exist"
	fi  
if [ -d $DEBDIR$LAUNCHDIR ] ; 
		then 
			cp lanManager.sh $DEBDIR$LAUNCHDIR
			chmod +x $DEBDIR$LAUNCHDIR/lanManager.sh
			else 
			echo "Sorry, $DEBDIR$LAUNCHDIR does not exist"
	fi 
if [ -d $DEBDIR$BINDIR ] ;
    then
        cp lanManager $DEBDIR$BINDIR/lanManager
        cp About_lanManager.txt $DEBDIR$BINDIR/lanManager
        echo "Installed into $DEBDIR$BINDIR"
        else
        echo "Sorry, $DEBDIR$BINDIR does not exist"
    fi

mkdir $DEBDIR/DEBIAN
cp postinst $DEBDIR/DEBIAN
cp control $DEBDIR/DEBIAN
gedit $DEBDIR/DEBIAN/control

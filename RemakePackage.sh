# RemakePackage.sh takes a version number, such as 1.0-1, and creates new client and server packages.
# ./RemakePackage.sh <version#>

echo "$1"
sudo ./Makepackage_client.sh $1
dpkg-deb --build -Zgzip lanManager_client_$1/
cp lanManager_client_1.0-1.deb /home/ehenricks/projects/lanManager/lanman/
sudo ./Makepackage.sh $1
dpkg-deb --build -Zgzip lanManager_server_$1/
sudo apt-get remove lanmanager
sudo dpkg -i /home/ehenricks/projects/lanManager/lanManager_server_$1.deb
vi /etc/lanManager/labAuditConfig.ini
sudo /etc/init.d/lanManager.sh stop

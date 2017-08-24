#!/bin/sh
#Name of package
package_name="lanManager_client_1.0-1.deb"
#Username of taget device
usr_name="igolgi"
#Directory path for local .deb file
from_dir="/var/www/lanman/"
#Directory path for target device
to_dir="/home/$usr_name/"

local_network="10.1.10."
min_ip=1
max_ip=256

given_ip=$1

sshpass -p igolgi scp -o StrictHostKeyChecking=no $from_dir$package_name $usr_name@$given_ip:$to_dir
sshpass -p igolgi ssh -o StrictHostKeyChecking=no $usr_name@$given_ip sudo dpkg -i $to_dir$package_name
echo "lanManager_install.sh was called for $given_ip"


#!/bin/sh
#Name of package
package_name="lanManager_client_1.0-1.deb"
#Username of taget device
usr_name="igolgi"
#Directory path for local .deb file
from_dir="/home/projects/lanManager/"
#Directory path for target device
to_dir="/home/$usr_name/"

local_network="10.1.10."
min_ip=1
max_ip=256

for cur_ip in {$min_ip..$max_ip}
do
    sshpass -p igolgi scp $from_dir$package_name $usr_name@$local_network$cur_ip:$to_dir
    sshpass -p igolgi ssh $usr_name@$local_network$cur_ip sudo dpkg -i $to_dir$package_name
done

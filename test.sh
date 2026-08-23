#!/bin/bash

make clean
make all
sudo mkdir -p /lib/modules/$(uname -r)/extra
sudo rm -f /lib/modules/$(uname -r)/extra/*.*
sudo cp ./*ko /lib/modules/$(uname -r)/extra/
ls /lib/modules/$(uname -r)/extra/ -l
sudo depmod -a
#echo 'options chardev_abc myint=1 mystring="ABC"' | sudo tee /etc/modprobe.d/chardev_abc.conf #給 modprobe 用
#cat /lib/modules/$(uname -r)/modules.dep //看的是「模組檔案之間的相依關係」。
sudo modprobe chardev_abc #我給 module 名稱，你幫我找檔案、處理 dependency 再載入。
lsmod | grep chardev_abc #載入狀態與相依關係
lsmod | grep request #沒有相依因為chardev_abc沒使用request_test();
cat /proc/devices | grep pico_sensor_abc #取得系統分配的 Major 號碼
read -p "input major number: " major
sudo mknod /dev/pico_sensor c "$major" 0 #手動建立設備檔
sudo chmod 666 /dev/pico_sensor
ls /dev/pico_sensor -l
make -C $(pwd)/user_space
./user_space/application "/dev/pico_sensor" "hello arth"
echo "arth hello" > /dev/pico_sensor
cat /dev/pico_sensor
#Kernel module 開發很常遇到這種情況：code 改對了，但 kernel 還在跑舊的 .ko。 
sudo rm /dev/pico_sensor
sudo modprobe -r chardev_abc #會嘗試卸載，並且把因為它而載入、而且目前沒有被其他 module 使用的相依模組一起卸載。
make -C $(pwd)/user_space clean
dmesg |tail -5
#echo "chardev_abc" > /etc/modules-load.d/master.conf #一開機就自動將模組載入

#sudo insmod chardev_abc.ko
#sudo rmmod chardev_abc
#dmesg | tail

# 安裝成系統 Module（正式安裝）
# 重開機後請直接使用 sudo modprobe chardev_abc，而不需要再用 insmod。
# 新增模組後，常需要執行 depmod 來更新描述檔。預設會遞迴掃描整個 /lib/modules/$(uname -r)/ 目錄以及它底下的「所有子資料夾」。
# pi@raspberry:~/vscode_session $ sudo mkdir /lib/modules/$(uname -r)/extra/
# pi@raspberry:~/vscode_session $ sudo cp ./chardev_abc.ko  /lib/modules/$(uname -r)/extra/
# pi@raspberry:~/vscode_session $ ls /lib/modules/$(uname -r)/extra/
# chardev_abc.ko
# pi@raspberry:~/vscode_session $ sudo depmod -a
# pi@raspberry:~/vscode_session $ sudo modprobe chardev_abc
# pi@raspberry:~/vscode_session $ sudo modprobe -r chardev_abc
# pi@raspberry:~/vscode_session $ dmesg |tail
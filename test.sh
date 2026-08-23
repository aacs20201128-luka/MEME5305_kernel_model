#!/bin/bash

#fuser /dev/pico_sensor #找出是哪個 process 還開著
sudo fuser -k /dev/pico_sensor #讓fuser殺掉所有使用 /dev/pico_sensor 的 process
sudo modprobe -r chardev_abc
make -C $(pwd)/user_space clean
make clean
make all
sudo mkdir -p /lib/modules/$(uname -r)/extra
sudo rm -f /lib/modules/$(uname -r)/extra/*.*
sudo cp ./*ko /lib/modules/$(uname -r)/extra/
ls /lib/modules/$(uname -r)/extra/ -l
sudo depmod -a
#echo 'options chardev_abc user_pid=1 mystring="ABC"' | sudo tee /etc/modprobe.d/chardev_abc.conf #給 modprobe 用
#cat /lib/modules/$(uname -r)/modules.dep //看的是「模組檔案之間的相依關係」。
sudo modprobe chardev_abc #我給 module 名稱，你幫我找檔案、處理 dependency 再載入。
lsmod | grep chardev_abc #載入狀態與相依關係
lsmod | grep request #沒有相依因為chardev_abc沒使用request_test();
cat /proc/devices | grep pico_sensor_abc #取得系統分配的 Major 號碼
#read -p "input major number: " major
#sudo mknod /dev/pico_sensor c "$major" 0 #手動建立設備檔
sudo chmod 666 /dev/pico_sensor
#ls /sys/class/charmodule_class -l
#ls /dev/pico_sensor -l

cat /dev/pico_sensor & #放在背景 wait_event_interruptible()，shell prompt 還是會回來。
cat_pid=$!
sleep 2 #sleep 要放在 echo 前面，目的是「等 cat 先睡好」
echo "Wait Queue Test" > /dev/pico_sensor
sleep 1 # 等 cat 被喚醒後真的 read/print
kill "$cat_pid" 2> /dev/null
wait "$cat_pid" 2>/dev/null

make -C $(pwd)/user_space
./user_space/application "/dev/pico_sensor" "hello arth" & #背景執行不卡終端機
app_pid=$! #代表「最近一個背景執行程序的 PID」。
echo "$app_pid" | sudo tee /sys/module/chardev_abc/parameters/user_pid #module的parameters
echo "為了解pause寫個東西進去" > /dev/pico_sensor #pause() 馬上被喚醒，接著立刻往下執行
sleep 2 #等application執行到read
echo "解 application 的 read" > /dev/pico_sensor
wait "$app_pid" 2>/dev/null
#cat /dev/pico_sensor & #"hello arth"

#Kernel module 開發很常遇到這種情況：code 改對了，但 kernel 還在跑舊的 .ko。 
#sudo rm /dev/pico_sensor
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

## Terminal 是外殼，Shell 是指揮官，你輸入的指令通常是 Shell 生出來的子 process；
##  前景要等，背景不用等。
# 終端機 Terminal：像一個「視窗／介面」，本身通常也是一個 process。
# Shell：終端機裡真正接收你指令的程式，例如 bash，它也是一個 process。
# 你輸入 cat、ls、vim：shell 通常會建立新的子 process 去執行它。
# 前景程式：shell 會等它跑完，所以你暫時拿不到 prompt。 
# 背景程式：shell 不等它，prompt 立刻回來，可以繼續下指令。
# &：把程式丟到背景，例如 cat /dev/pico_sensor &。
# Ctrl+C：通常是中斷目前前景程式。
# Ctrl+Z：把目前前景程式暫停。
# $ bg：讓暫停的程式去背景繼續跑。
# $ fg：把背景程式拉回前景。
# prompt 就是 shell 顯示出來，等你輸入指令的那一行提示字。
## shell process 建立一個子 process 來執行 cat，然後因為 cat 是前景工作，所以 shell 會等待它結束。
### 所以 echo 對 cat 的效果不是：解掉 → cat 結束 ，
### 而是：解掉這一次 read() → cat 繼續它自己的迴圈 → 再 read 一次
### 用：kill "$procid" //終止該 process；  2>/dev/null //把錯誤訊息丟掉。
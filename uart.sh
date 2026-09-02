#!/bin/bash

make clean
make pico
sudo insmod pico_sensor.ko
sudo chmod 666 /dev/pico_sensor
make -C ./user_space/ pico_c
make -C ./user_space/ pico
while true; do
    ./user_space/pico_application
    sleep 3
done
#./user_space/pico_application
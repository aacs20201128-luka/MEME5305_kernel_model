#!/bin/bash

make clean
make dht11
sudo insmod dht11_sensor.ko
sudo chmod 666 /dev/dht11_sensor
make -C ./user_space/ dht11_c
make -C ./user_space/ dht11
while true; do
    ./user_space/dht11_application
    sleep 3
done
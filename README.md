# HW_4.2: MQTT 
This repository contains MQTT publisher main.cpp (mbed) and subscriber mqtt_client.py.
mbed records roll/pitch/yaw values every 100ms and publishes it every 1s. PC (python) 
subscribes to "Mbed" topic containing roll/pitch/yaw data and print them out when any one 
of them exceeds 10 deg.

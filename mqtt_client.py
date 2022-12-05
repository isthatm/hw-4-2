import paho.mqtt.client as paho
import time
import re

# https://os.mbed.com/teams/mqtt/wiki/Using-MQTT#python-client

# MQTT broker hosted on local machine
mqttc = paho.Client()

# Settings for connection
# TODO: revise host to your IP
host = "192.168.43.84" #phone
#host = "192.168.137.1" #pc
topic = "Mbed"

# Callbacks
def on_connect(self, mosq, obj, rc):
    print("Connected rc: " + str(rc))

def on_message(mosq, obj, msg):
    message = str(msg.payload)
    subTopic = msg.topic
    res = re.findall("\d+\.\d+", message)
    new_list = [float(i) for i in res]
    if (new_list[0] > 10 or new_list[1] > 10 or new_list[2] > 10):
        print("[Received] Topic: " + subTopic + ", Message: " + message + "\n")

def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed OK")

def on_unsubscribe(mosq, obj, mid, granted_qos):
    print("Unsubscribed OK")

def degFilter(mosq, obj, msg):
    res = [int(i) for i in str(msg.payload).split() if i.isdigit()]

# Set callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe
mqttc.on_unsubscribe = on_unsubscribe

# Connect and subscribe
print("Connecting to " + host + "/" + topic)
mqttc.connect(host, port=1883, keepalive=60)
mqttc.subscribe(topic, 0)

mqttc.loop_forever()
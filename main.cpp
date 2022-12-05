#include "mbed.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "NetworkInterface.h"
#include "accelerometer.h"
#include "gyro.h"

// GLOBAL VARIABLES
WiFiInterface *wifi;
//MQTT::Client<MQTTNetwork, Countdown> *client;

InterruptIn btn2(BUTTON1);
EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread t;
Ticker publisher;
Accelerometer acc;
Gyro gyro;
double Accel[3]={0};
double Gyro[3]={0};
double  accAngleX=0;
double  accAngleY=0;
double elapsedTime=0;
double roll;  double roll10; double *r = &roll; 
double pitch; double pitch10; double *p = &pitch;
double yaw; double yaw10; double *y = &yaw;
double gyroAngleX=0;
double gyroAngleY=0;
int counter=0;
int idR[32] = {0};
int indexR = 0;
volatile int message_num = 0;
volatile int arrivedcount = 0;
volatile bool closed = false;

const char *topic = "Mbed";

Thread mqtt_thread(osPriorityHigh);
EventQueue mqtt_queue;

void record(void) {

  acc.GetAcceleromterSensor(Accel);
  acc.GetAcceleromterCalibratedData(Accel);

  accAngleX = (atan(Accel[1] / sqrt(Accel[0]*Accel[0] + Accel[2]*Accel[2])) * 180 / SENSOR_PI_DOUBLE);
  accAngleY = (atan(-1 * Accel[0] / sqrt(Accel[1]*Accel[1] + Accel[2]*Accel[2])) * 180 / SENSOR_PI_DOUBLE);

  gyro.GetGyroSensor(Gyro);

  elapsedTime=0.1; //100ms by thread sleep time
  // Currently the raw values are in degrees per seconds, deg/s, so we need to multiply by sendonds (s) to get the angle in degrees
  gyroAngleX = gyroAngleX + Gyro[0] * elapsedTime; // deg/s * s = deg
  gyroAngleY = gyroAngleY + Gyro[1] * elapsedTime;
  
  yaw =  yaw + Gyro[2] * elapsedTime;
  roll = accAngleX;
  pitch = accAngleY;
}

void messageArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;
    char msg[300];
    sprintf(msg, "Message arrived: QoS%d, retained %d, dup %d, packetID %d\r\n", message.qos, message.retained, message.dup, message.id);
    //printf(msg);
    ThisThread::sleep_for(2000ms);
    char payload[300];
    sprintf(payload, "Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    //printf(payload);
    ++arrivedcount;
}

void publish_message(MQTT::Client<MQTTNetwork, Countdown> *client1) {
  //message_num++;
  MQTT::Message message;
  char buff[100];
  sprintf(buff, "Roll: %f, Pitch: %f, Yaw: %f ", *r, *p, *y);
  message.qos = MQTT::QOS0;
  message.retained = false;
  message.dup = false;
  message.payload = (void *)buff;
  message.payloadlen = strlen(buff) + 1;
  int rc = client1->publish(topic, message);

  printf("rc:  %d\r\n", rc);
  printf("Pubished message: %s\r\n", buff);
}

//void startPublish(void) { mqtt_queue.call_every(1s, &publish_message, &client); }

void close_mqtt() { 
    closed = true; 
}

int main() {

  wifi = WiFiInterface::get_default_instance();
  if (!wifi) {
    printf("ERROR: No WiFiInterface found.\r\n");
    return -1;
  }

  printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
  int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD,
                          NSAPI_SECURITY_WPA_WPA2);
  if (ret != 0) {
    printf("\nConnection error: %d\r\n", ret);
    return -1;
  }

  NetworkInterface *net = wifi;
  MQTTNetwork mqttNetwork(net);
  MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

  // TODO: revise host to your IP
  const char *host = "192.168.43.84";
  const int port=1883;
  printf("Connecting to TCP network...\r\n");
  printf("address is %s/%d\r\n",host, port); // check setting

  int rc = mqttNetwork.connect(host, port); //(host, 1883);
  if (rc != 0) {
    printf("Connection error.");
    return -1;
  }
  printf("Successfully connected!\r\n");

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.MQTTVersion = 3;
  data.clientID.cstring = "Mbed";

  if ((rc = client.connect(data)) != 0) {
    printf("Fail to connect MQTT\r\n");
  }

  if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0){
            printf("Fail to subscribe\r\n");
    }
  
  mqtt_thread.start(callback(&mqtt_queue, &EventQueue::dispatch_forever));
  mqtt_queue.call_every(100ms,&record);
  //btn2.rise(mqtt_queue.event(&publish_message, &client));
  mqtt_queue.call_every(1s, &publish_message, &client);
  //btn2.rise(mqtt_queue.event(startPublish));
  //mqtt_queue.call(&publish_message, &client);
  
  /*
  int num = 0;
  while (num != 1) {
    client.yield(100);
    ++num;
  }
  */

  while (1) {
    if (closed)
      break;
    client.yield(500);
    ThisThread::sleep_for(1s);
  }
  
  //t.start(callback(&queue, &EventQueue::dispatch_forever));

  printf("Ready to close MQTT Network......\n");

  if ((rc = client.unsubscribe(topic)) != 0) {
    printf("Failed: rc from unsubscribe was %d\n", rc);
  }
  if ((rc = client.disconnect()) != 0) {
    printf("Failed: rc from disconnect was %d\n", rc);
  }

  mqttNetwork.disconnect();
  printf("Successfully closed!\n");

  return 0;
}



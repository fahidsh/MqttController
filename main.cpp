/**
    Datum: 06.05.2022
    Autor: Fahid Shehzad
    Beschreibung: Zwei Motoren gesteuert über Mqtt
*/
#include "mbed.h"
#include <cstdio>
#include <cstring>
#include <stdio.h>
#include <string>

#include "TCPSocket.h"
#include <MQTTClientMbedOs.h>

#include "ESP8266Interface.h"

// ESP8266Interface wifi(PB_10, PB_11, false); // Sturmboard
ESP8266Interface wifi(PC_10, PC_11, false); // bastel lösung

#ifndef SLEEP_TIME
#define SLEEP_TIME 100ms
#endif
#ifndef SLEEP_TIME_BETWEEN_STEPS
#define SLEEP_TIME_BETWEEN_STEPS 500ms
#endif

#ifndef DEBUG_LOG
// use the following line while Debugging
#define DEBUG_LOG log_message
// use the following line in Production
//#define DEBUG_LOG log_message_production
#endif

// Mqtt Broker
const std::string MQTT_SERVER = "mqtt.shehzad.eu";
// Mqtt Broker port (TCP 1883, WS 443, MQTTS 8883)
const unsigned int MQTT_PORT = 1883;
// Mqtt username (optional)
const std::string MQTT_USER = "";
// Mqtt password (optional)
const std::string MQTT_PASSWORD = "";
// Topic to subscribe to
const std::string MQTT_SUB_TOPIC = "public/mbed/fahid";
// Topic to publish to
const std::string MQTT_PUB_TOPIC = "public/mbed/fahid";
// Topic for last will message
const std::string MQTT_WILL_TOPIC_PREFIX = "public/mbed/fahid";
// QOS for Subscription
const MQTT::QoS MQTT_SUB_QOS = MQTT::QOS2;
// QOS for publish
const MQTT::QoS MQTT_PUB_QOS = MQTT::QOS0;
// Publish Retain
const bool MQTT_PUBLISH_RETAIN = false;

std::string mqtt_client_name = "mbed-app-";
std::string mqtt_last_will_topic = MQTT_WILL_TOPIC_PREFIX;
const unsigned int MQTT_MESSAGE_MAXLENGTH = 100;
MQTTPacket_connectData MQTT_CONNECTION = MQTTPacket_connectData_initializer;
unsigned int mqtt_messages_sent = 0;
unsigned int mqtt_messages_received = 0;
bool critical_failure = false;
bool CONTINUE_EXECUTION = true;
volatile bool mqtt_publish = false;

/**
********************************************************

********************************************************
*/
SocketAddress socket_address;
TCPSocket tcp_socket_subscriber;
MQTTClient mqtt_client_var(&tcp_socket_subscriber);

/**
********************************************************

********************************************************
*/
void log_message(int, const char *, ...);
void log_message_production(int, const char *, ...);
bool is_command(std::string);
int correct_mqtt_client_name();
int correct_mqtt_last_will_topic();
int connect_to_wifi();
int resolve_mqtt_servername();
int open_socket();
int set_mqtt_port();
int set_mqtt_connection_params();
void publish_message_to_mqtt(std::string, std::string = MQTT_PUB_TOPIC);
void process_incoming_mqtt_message(MQTT::MessageData);
int connect_to_mqtt_server();
int disconnect_from_mqtt_server();
int subscribe_to_mqtt_topic();
int unsubscribe_from_mqtt_topic();
typedef int (*Step)();
int execute_step(Step, int);
void restart_controller();
void read_mqtt_message();
bool init_mqtt_client();

void motor1_controller();
void motor2_controller();
void motor_lauf_vorwart();
void motor_lauf_ruckwart();
void motor_stop();
/**
********************************************************

********************************************************
*/

DigitalOut led(LED1);
#define MOTOR_SCHRITT 2ms

// PortOut motor1(PortC, 0b1111);
// PortOut motor2(PortC, 0b11110000);
PortOut motor1(PortC, 0b1111);
PortOut motor2(PortC, 0b1101100000);
int motorlauf_links[] = {0b0001, 0b0011, 0b0010, 0b0110,
                         0b0100, 0b1100, 0b1000, 0b1001};
int motorlauf_rechts[] = {0b10001, 0b10001, 0b10000, 0b11000, 0b01000,
                          0b01010, 0b00010, 0b00011, 0b00001};

int position_motor1 = 0;
int position_motor2 = 0;
enum MotorState { STOP, VORWART, RUECKWART };
volatile MotorState motor_richtung = STOP;

// Taster 1,2,3 auf Sturmboard, alle als Interrupts mit PullDown Modus
// InterruptIn taster1(PA_1, PullDown);
// InterruptIn taster2(PA_6, PullDown);
// InterruptIn taster3(PA_10, PullDown);
// Taster 1,2,3 auf MF-Shield
// Taster 1,2,3 auf Sturmboard, alle als Interrupts mit PullDown Modus
InterruptIn taster1(PA_1, PullUp);
InterruptIn taster2(PA_4, PullUp);
InterruptIn taster3(PB_0, PullUp);

// Zwei Threads, jeweils für beide Motoren
Thread motor1_thread;
Thread motor2_thread;
/**
********************************************************

********************************************************
*/

bool init() { return init_mqtt_client(); }

// main() runs in its own thread in the OS
int main() {

  bool INIT_SUCCESS = init();
  if (!INIT_SUCCESS) {
    // printf("ERROR: unable to initialize properly, restarting
    // Microcontroller");
    CONTINUE_EXECUTION = false;
    // restart_controller();
  }

  // Threads für beide Motoren starten
  motor1_thread.start(motor1_controller);
  motor2_thread.start(motor2_controller);

  // Interrupt von taster1
  taster1.rise(&motor_lauf_vorwart);
  // Interrupt von taster2
  taster2.rise(&motor_lauf_ruckwart);
  // Interrupt von taster3
  taster3.rise(&motor_stop);

  Timer mqtt_check;
  mqtt_check.start();
  // LowPowerTicker mqtt_read;
  // mqtt_read.attach(&read_mqtt_message, 2000ms);
  // int seconds = 0;
  // int minutes = 0;
  // int hours = 0;
  int check_mqtt_after = 2500;
  int last_mqtt_check = 0;

  // while (CONTINUE_EXECUTION) {
  while (1) {
    // get time in milliseconds
    long time_since_last_check = mqtt_check.elapsed_time().count() / 1000;

    if (time_since_last_check >= last_mqtt_check + check_mqtt_after) {
      if (mqtt_publish) {
        ThisThread::sleep_for(500ms);
        publish_message_to_mqtt(std::to_string(motor_richtung));
        mqtt_publish = false;
      } else {
        read_mqtt_message();
      }
      last_mqtt_check = time_since_last_check;
    }
    ThisThread::sleep_for(50ms);
    /*
    if (time_since_last_check / 1000 >= seconds + 1) {
      seconds = time_since_last_check / 1000;
      minutes = seconds / 60;
      hours = minutes / 60;
      printf("%02d : %02d : %02d\n", hours % 24, minutes % 60, seconds % 60);
    }
    */
  }
  mqtt_check.stop();
  return 0;
  // printf("Programm beendet, reset to continue\n");
}

/**
********************************************************











********************************************************
*/
void log_message_production(int, const char *, ...) { return; }
void log_message(int sender = 99, const char *message_with_args = "", ...) {
  va_list arg;
  va_start(arg, message_with_args);
  // max length of message, printf style
  char message[250];
  // take the message into char array
  vsprintf(message, message_with_args, arg);
  va_end(arg);
  // save the message as string (optional)
  // std::string message_string = message;

  switch (sender) {
  case 110 ... 199: // supposed to be DEBUG messages
    printf("DEBUG: %s\n", message);
    break;
  case 210 ... 299: //
    printf("INFO: %s\n", message);
    break;
  case 310 ... 399:
    printf("ERROR: %s\n", message);
    break;
  case 410 ... 499:
    printf("FETAL: %s\n", message);
    break;
  default:
    printf("%s\n", message);
  }
}

/**
********************************************************

********************************************************
*/

int correct_mqtt_client_name() {

  std::string mac_address = wifi.get_mac_address(); // ab:cd:ef:12:34:56
  // we want to take only the last 6 characters, so we will remove the first 8
  // symbols first 9 symbols are: ab:cd:ef: after removing them we have:
  // 12:34:56
  mac_address.replace(0, 9,
                      ""); // first 9 characters replaced with nothing (removed)

  mac_address.replace(2, 1, ""); // string is now: 1234:56

  mac_address.replace(4, 1, ""); // string is now: 123456

  // yes, c/c++ doesn't have a simple method to replace a char/string with
  // another like java

  // let's check the last 6 characters of client name to find out if name is
  // already set
  std::size_t mac_found = mqtt_client_name.find(mac_address);
  // std::string::npos means, mac was not found, otherwiese the first position
  // of max will be returned
  if (mac_found ==
      std::string::npos) { // if mac is not found in client name, then
    // let's add the new mac address at the end of mqtt-client-name
    mqtt_client_name.append(mac_address);
  }
  // for debugging, let print the new processed mac address
  // printf("Mac Address of wifi controller has been replaced from %s to %s",
  // wifi.get_mac_address(), mac_address.c_str());
  DEBUG_LOG(210, "Mqtt Client Name: %s", mqtt_client_name.c_str());

  return 0;
}
/**
********************************************************

********************************************************
*/

int correct_mqtt_last_will_topic() {
  mqtt_last_will_topic = mqtt_last_will_topic.append("/")
                             .append(mqtt_client_name)
                             .append("/online");
  return 0;
}

/**
********************************************************

********************************************************
*/

int connect_to_wifi() {

  // get the current status of wifi connection
  nsapi_connection_status wifi_status = wifi.get_connection_status();

  /*
  NSAPI_STATUS_LOCAL_UP           = 0,        < local IP address set
  NSAPI_STATUS_GLOBAL_UP          = 1,        < global IP address set
  NSAPI_STATUS_DISCONNECTED       = 2,        < no connection to network
  NSAPI_STATUS_CONNECTING         = 3,        < connecting to network
  NSAPI_STATUS_ERROR_UNSUPPORTED  = NSAPI_ERROR_UNSUPPORTED
  */

  int status =
      -1; // wifi connection status as int, intially set as error status

  // if wifi connection is UP, then just set status variable to zero=OK
  if (wifi_status == NSAPI_STATUS_LOCAL_UP ||
      wifi_status == NSAPI_STATUS_GLOBAL_UP) {
    status = 0;
    DEBUG_LOG(200, "Already connected to Wifi");
  } else {
    // Otherwise try to connect to wifi
    status = wifi.connect(
        MBED_CONF_NSAPI_DEFAULT_WIFI_SSID,     // Wifi SSID from mbed_app.json
        MBED_CONF_NSAPI_DEFAULT_WIFI_PASSWORD, // Wifi Password from
                                               // mbed_app.json
        NSAPI_SECURITY_WPA_WPA2);

    if (status == 0) { // if conn has a value of '0', connection was successful
      // Wifi Connected
      DEBUG_LOG(200, "Connected to Wifi Network Successfully.");
      wifi.get_ip_address(&socket_address);
      DEBUG_LOG(200, "IP: %s", socket_address.get_ip_address());
      wifi.set_as_default();
    } else {
      DEBUG_LOG(300, "wifi connection error: %d", status);
    }
  }
  return status;
}

/**
********************************************************

********************************************************
*/

int resolve_mqtt_servername() {
  int status = wifi.gethostbyname(MQTT_SERVER.c_str(), &socket_address);
  if (status == 0) {
    DEBUG_LOG(200, "DNS Query Result of [%s]: %s", MQTT_SERVER.c_str(),
              socket_address.get_ip_address());
  }
  return status;
}

/**
********************************************************

********************************************************
*/

int open_socket() {

  // get the current socket status
  nsapi_error_t socket_status =
      tcp_socket_subscriber.getpeername(&socket_address);

  /*
  NSAPI_ERROR_OK	on success.
  NSAPI_ERROR_NO_SOCKET	if socket is not connected.
  NSAPI_ERROR_NO_CONNECTION	if the remote peer was not set.
  */
  int status = -1;

  // if socket is already open/connected, then just set status to zero=OK and
  // skip to return
  if (socket_status == NSAPI_ERROR_OK) {
    status = 0;
  } else {
    // if socket status was not OK, then open the socket
    tcp_socket_subscriber.open(&wifi);

    status = tcp_socket_subscriber.connect(socket_address);

    if (status == 0) {
      DEBUG_LOG(200, "Connected to Server %s[%s] Successfully",
                MQTT_SERVER.c_str(), socket_address.get_ip_address());
    }
  }

  return status;
}

/**
********************************************************

********************************************************
*/

int set_mqtt_port() {

  if (socket_address.get_port() != MQTT_PORT) {
    socket_address.set_port(MQTT_PORT);
    DEBUG_LOG(210, "Port set to %d", MQTT_PORT);
  }

  return 0;
}

/**
********************************************************

********************************************************
*/

int set_mqtt_connection_params() {

  char *client_id = (char *)mqtt_client_name.c_str();

  if (MQTT_CONNECTION.clientID.cstring != client_id) {

    // create a ast will message, it is used when client is dead/disconnected
    MQTTPacket_willOptions WILL_MESSAGE;
    WILL_MESSAGE.qos = MQTT_PUB_QOS;
    WILL_MESSAGE.retained = MQTT_PUBLISH_RETAIN;
    char *last_will_topic = (char *)mqtt_last_will_topic.c_str();
    char *last_will_message = (char *)"0";
    WILL_MESSAGE.topicName.cstring = last_will_topic;
    WILL_MESSAGE.message.cstring = last_will_message;

    MQTT_CONNECTION.MQTTVersion = 3;
    MQTT_CONNECTION.struct_version = 0;
    MQTT_CONNECTION.clientID.cstring = client_id;
    MQTT_CONNECTION.will = WILL_MESSAGE;
    MQTT_CONNECTION.willFlag = true;

    // MQTT_CONNECTION.cleansession = 1;
    if (strlen(MQTT_USER.c_str()) > 1) {
      char *user = (char *)MQTT_USER.c_str();
      char *pass = (char *)MQTT_PASSWORD.c_str();
      MQTT_CONNECTION.username.cstring = user;
      MQTT_CONNECTION.password.cstring = pass;
    }

    DEBUG_LOG(210, "Mqtt Connection Options set", 199);
  }

  return 0;
}

/**
********************************************************

********************************************************
*/

void show_mqtt_options() {
  DEBUG_LOG(220, "Client Name: %s", MQTT_CONNECTION.clientID.cstring);
  DEBUG_LOG(220, "Cleansession: %d", MQTT_CONNECTION.cleansession);
  DEBUG_LOG(220, "Mqtt Version: %d", MQTT_CONNECTION.MQTTVersion);
  DEBUG_LOG(220, "Mqtt Struct Version: %d", MQTT_CONNECTION.struct_version);
  DEBUG_LOG(220, "Mqtt User: %s", MQTT_CONNECTION.username.cstring);
  DEBUG_LOG(220, "Mqtt Pass: %s", MQTT_CONNECTION.password.cstring);
  DEBUG_LOG(220, "Mqtt Will Topic: %s", MQTT_CONNECTION.will.topicName.cstring);
  DEBUG_LOG(220, "Mqtt Will Message: %s", MQTT_CONNECTION.will.message.cstring);
}

/**
********************************************************

********************************************************
*/

void publish_message_to_mqtt(std::string mqtt_message,
                             std::string topic_to_publish_to) {

  char parsed_mqtt_message[MQTT_MESSAGE_MAXLENGTH];

  char *topic = (char *)topic_to_publish_to.c_str();

  sprintf(parsed_mqtt_message, "%s", mqtt_message.c_str());
  int message_length = strlen(parsed_mqtt_message);

  MQTT::Message MQTT_MESSAGE;
  MQTT_MESSAGE.qos = MQTT_PUB_QOS;
  MQTT_MESSAGE.retained = MQTT_PUBLISH_RETAIN;
  MQTT_MESSAGE.dup = false;
  MQTT_MESSAGE.payload = (void *)parsed_mqtt_message;
  MQTT_MESSAGE.payloadlen = message_length;

  int status = mqtt_client_var.publish(topic, MQTT_MESSAGE);

  DEBUG_LOG(200, "STATUS PUB: {%d, %d, %d, %s, %s}", status, mqtt_messages_sent,
            message_length, topic, mqtt_message.c_str());

  mqtt_messages_sent++;
}

/**
********************************************************

********************************************************
*/

void process_incoming_mqtt_message(MQTT::MessageData &md) {
  MQTT::Message &message = md.message;
  mqtt_messages_received++;
  int msg_length = message.payloadlen;

  std::string mqtt_message = (char *)message.payload;
  mqtt_message = mqtt_message.substr(0, msg_length);

  if (!is_command(mqtt_message)) {
    DEBUG_LOG(0, "MSG RCVD: {%d, %d, %s}", mqtt_messages_received, msg_length,
              mqtt_message.c_str());
  }
}

/**
********************************************************

********************************************************
*/

bool is_command(std::string input) {
  if (std::to_string(motor_richtung) == input) {
    return true;
  }

  if (input == "0") {
    motor_richtung = STOP;
    DEBUG_LOG(100, "Mqtt Command: %s: Motor Stop", input.c_str());
  } else if (input == "1") {
    motor_richtung = VORWART;
    DEBUG_LOG(100, "Mqtt Command: %s: Motor Lauf Vorwart", input.c_str());
  } else if (input == "2") {
    motor_richtung = RUECKWART;
    DEBUG_LOG(100, "Mqtt Command: %s: Motor Lauf Ruckwart", input.c_str());
  } else if (input == "3") {
    DEBUG_LOG(100, "Mqtt Command: %s: Beispiel Befehl", input.c_str());
  } else
    return false; // if one of above was true then else will not be executed

  return true;
}

/**
********************************************************

********************************************************
*/

int connect_to_mqtt_server() {

  int status = -1;

  if (mqtt_client_var.isConnected()) {

    status = 0;

  } else {
    status = mqtt_client_var.connect(MQTT_CONNECTION);
    if (status == 0) {
      DEBUG_LOG(210, "Connected to Mqtt Broker Successfully");
      mqtt_client_var.setDefaultMessageHandler(process_incoming_mqtt_message);
      // publish the message to status topic
      publish_message_to_mqtt(std::string("1"), mqtt_last_will_topic);
    } else {
      DEBUG_LOG(310, "Error connection to Mqtt Broker. Code: %d", status);
    }
  }
  return status;
}
/**
********************************************************

********************************************************
*/

int disconnect_from_mqtt_server() {
  int status = mqtt_client_var.disconnect();
  if (status == 0) {
    DEBUG_LOG(200, "Disconnected from Mqtt Broker");
  } else {
    DEBUG_LOG(300, "Error from disconnect: %d", status);
  }
  return status;
}
/**
********************************************************

********************************************************
*/

int subscribe_to_mqtt_topic() {
  int status = mqtt_client_var.subscribe(MQTT_SUB_TOPIC.c_str(), MQTT_SUB_QOS,
                                         process_incoming_mqtt_message);
  if (status == 0) {
    DEBUG_LOG(210, "Subscribed to Topic: '%s' Successfully",
              MQTT_SUB_TOPIC.c_str());
  } else {
    DEBUG_LOG(310, "Error in subscribing to Mqtt Topic[%s], Code: %d",
              MQTT_SUB_TOPIC.c_str(), status);
  }

  return status;
}

/**
********************************************************

********************************************************
*/

int unsubscribe_from_mqtt_topic() {
  int status = mqtt_client_var.unsubscribe(MQTT_SUB_TOPIC.c_str());
  if (status == 0) {
    DEBUG_LOG(200, "Unsubscribed from Topic: '%s'", MQTT_SUB_TOPIC.c_str());
  } else {
    DEBUG_LOG(300, "Error from unsubscribe: %d", status);
  }
  return status;
}

/**
********************************************************

********************************************************
*/

/*
    This function executes a given function and examine its return value
    and keeps executing the function until a return value of 0 is received
    or max_attempts have been made

    the function name to execute is passed as parameter
    parameter function must have a return type of int
*/
typedef int (*Step)();
int execute_step(Step what_step, int max_attempts = 5) {
  int attempts = 0;
  int status = -1;
  if (!critical_failure) {
    while (status != 0 && attempts < max_attempts) {
      status = what_step();
      // DEBUG_LOG(0,".");
      ThisThread::sleep_for(SLEEP_TIME_BETWEEN_STEPS);
      attempts++;
    }
  }

  // ThisThread::sleep_for(SLEEP_TIME_BETWEEN_STEPS);
  // if critical failure has not occured and status is not 0/OK
  if (status != 0 && !critical_failure) {
    critical_failure = true;
  }
  return status;
}

/**
********************************************************

********************************************************
*/

void restart_controller() { NVIC_SystemReset(); }

/**
********************************************************

********************************************************
*/
void read_mqtt_message() {
  if (!mqtt_publish) {
    mqtt_client_var.yield();
  }
}

/**
********************************************************

********************************************************
*/

bool init_mqtt_client() {

  int status = 0;

  // add mac-address to the end of client_name, so it is always unique per
  // device
  DEBUG_LOG(100, "Correcting Mqtt Client name");
  status = execute_step(correct_mqtt_client_name);

  DEBUG_LOG(100, "Setting up Mqtt Client Last Will");
  status = execute_step(correct_mqtt_last_will_topic);

  // connect to wifi
  DEBUG_LOG(100, "Connecting to Wifi");
  status = execute_step(connect_to_wifi);

  // resolve server to ip
  DEBUG_LOG(100, "Resolving Mqtt-Host to IP-Address");
  status = execute_step(resolve_mqtt_servername);

  // set port
  DEBUG_LOG(100, "Setting Port for Mqtt-Server");
  status = execute_step(set_mqtt_port);

  // start a socket connection
  DEBUG_LOG(100, "Opening Socket");
  status = execute_step(open_socket);

  // set mqtt connection parameters
  DEBUG_LOG(100, "Setting Mqtt-Connection Parameters");
  status = execute_step(set_mqtt_connection_params);

  // connect to mqtt server
  DEBUG_LOG(100, "Connecting to Mqtt Server");
  status = execute_step(connect_to_mqtt_server, 1);

  // subscribe to mqtt topic
  DEBUG_LOG(100, "Subscribing to Mqtt-Topic");
  status = execute_step(subscribe_to_mqtt_topic);

  return (status == 0) ? true : false;
}
/**
********************************************************

********************************************************
*/

void motor1_controller() {
  while (true) {
    // nur was tun wenn motor_richtung nicht gleich null ist.
    if (motor_richtung != STOP) {
      // array Wert an die Pins von Port schicken
      motor1 = motorlauf_links[position_motor1 % 8];

      // wenn motor_richtung ist vorwart, dann addiere 1
      // wenn motor_richtung ist rückwart, dann subtractiere 1
      position_motor1 += (motor_richtung == VORWART) ? 1 : -1;
    }
    ThisThread::sleep_for(MOTOR_SCHRITT);
  }
}

void motor2_controller() {
  DigitalOut m2[] = {PC_5, PC_6, PC_8, PC_9};
  while (true) {

    if (motor_richtung != STOP) {
      // array wert an die Pins schicken, um 4 bits verschieben
      motor2 = motorlauf_rechts[position_motor2 % 8] << 5;
      position_motor2 += (motor_richtung == VORWART) ? 1 : -1;
    }

    ThisThread::sleep_for(MOTOR_SCHRITT);
  }
}
/**
********************************************************

********************************************************
*/

void motor_stop() {
  motor_richtung = STOP;
  mqtt_publish = true;
}
void motor_lauf_vorwart() {
  motor_richtung = VORWART;
  mqtt_publish = true;
}
void motor_lauf_ruckwart() {
  motor_richtung = RUECKWART;
  mqtt_publish = true;
}

/**
********************************************************

********************************************************
*/
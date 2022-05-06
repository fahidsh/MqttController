/**
    Datum: 06.05.2022
    Autor: Fahid Shehzad
    Beschreibung: Motorsteurung
*/

#include "mbed.h"

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
//#define DEBUG_LOG //
#endif

PortOut motor1(PortC, 0b1111);
PortOut motor2(PortC, 0b11110000);
int motorlauf_links[] = {0b0001, 0b0011, 0b0010, 0b0110,
                         0b0100, 0b1100, 0b1000, 0b1001};
int motorlauf_rechts[] = {0b1001, 0b1001, 0b1000, 0b1100, 0b0100,
                          0b0110, 0b0010, 0b0011, 0b0001};
int position = 0;
volatile int motor_richtung = 1; // -1 für Rückwarts, 0 zum stoppen

#define GEDRUCKT 1
// Taster 1,2,3 auf Sturmboard
InterruptIn taster1(PA_1, PullDown);
InterruptIn taster2(PA_6, PullDown);
InterruptIn taster3(PA_10, PullDown);

Thread motor1_thread;
Thread motor2_thread;

/**
********************************************************

********************************************************
*/
void log_message(int, const char *, ...);
/**
********************************************************

********************************************************
*/
void motor1_controller() {
  Timer t;
  t.start();
  int time_s = 0;

  while (true) {
    long time_ms = t.elapsed_time().count() / 1000;
    if (motor_richtung != 0) {
      motor1 = motorlauf_links[position % 8];
      position += motor_richtung;
      if (time_ms / 1000 >= time_s + 2) {
        DEBUG_LOG(0, "pos1: %d, %d", time_s, position);
        time_s = time_ms / 1000;
      }
    }
    ThisThread::sleep_for(2ms);
  }
}

void motor2_controller() {
  Timer t;
  t.start();
  int time_s = 0;

  while (true) {
    long time_ms = t.elapsed_time().count() / 1000;
    if (motor_richtung != 0) {
      motor2 = motorlauf_rechts[position % 8] << 4;
      if (time_ms / 1000 >= time_s + 2) {
        DEBUG_LOG(0, "pos2: %d, %d", time_s, position);
        time_s = time_ms / 1000;
      }
    }
    ThisThread::sleep_for(2ms);
  }
}

bool init() { return true; }

void motor_lauf_vorwart() { motor_richtung = 1; }
void motor_lauf_ruckwart() { motor_richtung = -1; }
void motor_stop() { motor_richtung = 0; }

// main() runs in its own thread in the OS
int main() {
  motor1_thread.start(motor1_controller);
  motor2_thread.start(motor2_controller);

  taster1.rise(&motor_lauf_vorwart);
  taster2.rise(&motor_lauf_ruckwart);
  taster3.rise(&motor_stop);
  while (true) {
    ThisThread::sleep_for(50ms);
  }
}

/**
********************************************************











********************************************************
*/

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

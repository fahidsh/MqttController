/**
    Datum: 06.05.2022
    Autor: Fahid Shehzad
    Beschreibung: Motorsteurung
*/

#include "mbed.h"

#define SLEEP_TIME 50ms
#define MOTOR_SCHRITT 2ms

// Motor 1 ist auf C0 bis C3 angeschlossen
PortOut motor1(PortC, 0b1111);
// Motor 2 ist auf C4 bis C7 angeschlossen
PortOut motor2(PortC, 0b11110000);
// Array für Motorlauf nach Links/Normal
int motorlauf_links[] = {0b0001, 0b0011, 0b0010, 0b0110,
                         0b0100, 0b1100, 0b1000, 0b1001};
// Array für Motorlauf nach Recht/Umgekehrt
int motorlauf_rechts[] = {0b1001, 0b1001, 0b1000, 0b1100, 0b0100,
                          0b0110, 0b0010, 0b0011, 0b0001};

// controll variable
int position = 0;
// Variable um die Laufrichtung zu setzen
volatile int motor_richtung = 1; // -1 für Rückwarts, 0 zum stoppen

// Taster 1,2,3 auf Sturmboard, alle als Interrupts mit PullDown Modus
InterruptIn taster1(PA_1, PullDown);
InterruptIn taster2(PA_6, PullDown);
InterruptIn taster3(PA_10, PullDown);

// Zwei Threads, jeweils für beide Motoren
Thread motor1_thread;
Thread motor2_thread;

// Kontroll Methode für Motor 1
void motor1_controller() {
  while (true) {
    // nur was tun wenn motor_richtung nicht gleich null ist.
    if (motor_richtung != 0) {
      // array Wert an die Pins von Port schicken
      motor1 = motorlauf_links[position % 8];
      // position um erhöhen/mindern
      position += motor_richtung;
    }
    ThisThread::sleep_for(MOTOR_SCHRITT);
  }
}

// Kontroll Methode für Motor 2
void motor2_controller() {
  while (true) {
    if (motor_richtung != 0) {
      // array wert an die Pins schicken, um 4 bits verschieben
      motor2 = motorlauf_rechts[position % 8] << 4;
    }
    ThisThread::sleep_for(MOTOR_SCHRITT);
  }
}

// setzt Variable motor_richtung = 1, Motore laufen normal
void motor_lauf_vorwart() { motor_richtung = 1; }
// setzt Variable motor_richtung = -1, Motore laufen Rückwart
void motor_lauf_ruckwart() { motor_richtung = -1; }
// setzt Variable motor_richtung = 0, Motore stoppen
void motor_stop() { motor_richtung = 0; }

// main() runs in its own thread in the OS
int main() {

  // Threads für beide Motoren starten
  motor1_thread.start(motor1_controller);
  motor2_thread.start(motor2_controller);

  // Interrupt von taster1
  taster1.rise(&motor_lauf_vorwart);
  // Interrupt von taster2
  taster2.rise(&motor_lauf_ruckwart);
  // Interrupt von taster3
  taster3.rise(&motor_stop);

  // nichts zu tun, einfach in endloss Schleife bleiben
  while (true) {
    ThisThread::sleep_for(SLEEP_TIME);
  }
}

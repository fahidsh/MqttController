#include "mbed.h"
#include <string>

DigitalOut led(LED1);

#define DEBUNG_LOG printf
volatile bool publish_aktive = false;

bool init() { return true; }

void flip_led() {
  led = !led;
  // using printf in Ticker function is not allowed
}

void publish_message(std::string message) {
  publish_aktive = true;
  Timer pm;
  pm.start();
  DEBUNG_LOG("%s\n", message.c_str());
  // einfach bisschen Zeit verschwinden
  long time_in_milli = pm.elapsed_time().count() / 1000;
  while (time_in_milli < 1000) {
    time_in_milli = pm.elapsed_time().count() / 1000;
  }
  DEBUNG_LOG("%lums verschwindet\n", time_in_milli);
  publish_aktive = false;
}

void read_message() {
  if (publish_aktive) {
    return;
  }

  Timer rm;
  rm.start();

  int wiederhole = 10;
  int summe = 0;
  for (int i = 0; i < wiederhole; i++) {
    summe += i;
  }
  long time_in_milli = rm.elapsed_time().count() / 1000;
  DEBUNG_LOG("es hat %lu ms gedaurt die Summe von Zahlen bis %d zu berechnen, "
             "die Summe ist %d\n",
             time_in_milli, wiederhole, summe);
  rm.stop();
}

int main() {
  Ticker led_flip;
  led_flip.attach(&flip_led, 2000ms);

  Timer timer;
  timer.start();
  int seconds = 0;
  int minutes = 0;
  int hours = 0;
  while (true) {
    long milliseconds = timer.elapsed_time().count() / 1000;
    if (milliseconds / 1000 >= seconds + 1) {
      seconds = milliseconds / 1000;
      minutes = seconds / 60;
      hours = minutes / 60;
      // printf("%02d : %02d : %02d\n", hours % 24, minutes % 60, seconds % 60);

      /*
      Jede Sekunde soll nach eine neue Nachricht gepruft werden
      Aber nicht in die Sekunde 11 und 12, hier soll publiziert werden
      Taktzeit 15 Sekunden immer
      */
      if (seconds % 15 < 10) {
        read_message();
      } else {
        publish_message("Publish Aktiviert");
      }
    }
  }
}
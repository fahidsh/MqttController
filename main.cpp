#include "mbed.h"

DigitalOut led(LED1);

bool init() {
    return true;
}

void flip_led() {
    led = ! led;
    // using printf in Ticker function is not allowed
}

int main () {
    Ticker led_flip;
    led_flip.attach(&flip_led, 2000ms);

    Timer timer;
    timer.start();
    int seconds = 0;
    int minutes = 0;
    int hours = 0;
    while(true){
        long milliseconds = timer.elapsed_time().count()/1000;
        if(milliseconds / 1000 >= seconds+1) {
            seconds = milliseconds / 1000;
            minutes = seconds / 60;
            hours = minutes / 60;
            printf("%02d : %02d : %02d\n", hours % 24, minutes % 60, seconds % 60);
        }
    }
}
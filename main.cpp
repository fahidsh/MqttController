#include "mbed.h"

// Base: https://os.mbed.com/forum/mbed/topic/1828/

#define PC_14_SEL_MASK    ~(3UL << 12)
#define PC_14_SET_MASK    (1UL << 46)
#define PC_14_CLR_MASK    ~(PC_14_SET_MASK)
#define PC_14_AS_OUTPUT   LPC_PINCON->PINSEL1&=PC_14_SEL_MASK;LPC_GPIO0->FIODIR|=PC_14_SET_MASK
#define PC_14_AS_INPUT    LPC_GPIO0->FIOMASK &= PC_14_CLR_MASK; 
#define PC_14_SET         LPC_GPIO0->FIOSET = PC_14_SET_MASK
#define PC_14_CLR         LPC_GPIO0->FIOCLR = PC_14_SET_MASK
#define PC_14_IS_SET      (bool)(LPC_GPIO0->FIOPIN & PC_14_SET_MASK)
#define PC_14_IS_CLR      !(PC_14_IS_SET)
#define PC_14_MODE(x)     LPC_PINCON->PINMODE1&=PC_14_SEL_MASK;LPC_PINCON->PINMODE1|=((x&0x3)<<12)
 
DigitalOut myled(LED1);
InterruptIn mybool(PC_14);

void led2on(void) {
    myled = 1;
}
 
void led2off(void) {
    myled = 0;
}   
 
int main() {
 
    // Setup InterruptIn as normal.
    mybool.mode(PullUp);
    mybool.rise(&led2on);
    mybool.fall(&led2off);
    
    // Now cheat, manually switch it to an output.
    PC_14_AS_OUTPUT;
 
    while(1) {
        myled = 1;
        PC_14_SET; // Set the bool and led2on() is called.
        thread_sleep_for(200);
        
        myled = 0;
        PC_14_CLR; // Clear the bool and led2off() is called.
        thread_sleep_for(200);
    }
}
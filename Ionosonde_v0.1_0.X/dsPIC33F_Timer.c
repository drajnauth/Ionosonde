#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "main.h"         
#include "dsPIC33F_Timer.h"

// Time variables
unsigned int PR;
extern volatile unsigned int ElapsedTime;

void setupTimer ( unsigned char tnum, unsigned int overflow)
{
    switch (tnum) {
        case 1:
            PR1 = overflow;
            T1CONbits.TON = 0;                      // Disable Timer T1
            T1CONbits.TSIDL = 0;
            T1CONbits.TGATE = 0;
            T1CONbits.TCKPS = 0;
            T1CONbits.TCS = 0;
            break;

        case 2:
            PR2 = overflow;
            TMR2 = 0;
            T2CONbits.TON = 0;                      // Disable Timer T1
            T2CONbits.TSIDL = 0;
            T2CONbits.TGATE = 0;
            T2CONbits.TCKPS = 0;
            T2CONbits.TCS = 0;
            T2CONbits.T32 = 0;
            break;

        case 3:
             break;

        case 4:
             break;

        case 5:
             break;



    }
}

void startTimer ( unsigned char tnum)
{
    switch (tnum) {
        case 1:
            TMR1 = 0;
            IFS0bits.T1IF = 0;
            IPC0bits.T1IP = TIMER_INTERRUPT_PRIORITY;    // Set T1 Interrupt priority
            IEC0bits.T1IE = 1;                          // Enable T1 Interrupt
            T1CONbits.TON = 1;                          // Enable T1
            break;

        case 2:
            TMR2 = 0;
            IFS0bits.T2IF = 0;
            IPC1bits.T2IP = TIMER_INTERRUPT_PRIORITY;    // Set T1 Interrupt priority
            IEC0bits.T2IE = 1;                          // Enable T1 Interrupt
            T2CONbits.TON = 1;                          // Enable T1
            break;

        case 3:
             break;

        case 4:
             break;

        case 5:
             break;
    }
}


unsigned int getTimerValue (unsigned char tnum)
{
    switch (tnum) {
        case 1:
            return TMR1;
            break;

        case 2:
            return TMR2;
            break;

        case 3:
             break;

        case 4:
             break;

        case 5:
             break;
    }
    
    return 0;
    
}



void stopTimer ( unsigned char tnum)
{
    switch (tnum) {
        case 1:
            PR1 = 0;
            IPC0bits.T1IP = 0;                      // Set T1 interrupt priority to 0
            IEC0bits.T1IE = 0;                      // Disable T1 Interrupt
            T1CONbits.TON = 0;                      // Disable Timer T1
            break;

        case 2:
            PR2 = 0;
            IPC1bits.T2IP = 0;                      // Set T1 interrupt priority to 0
            IEC0bits.T2IE = 0;                      // Disable T1 Interrupt
            T2CONbits.TON = 0;                      // Disable Timer T1
            break;

        case 3:
             break;

        case 4:
             break;

        case 5:
             break;
    }
}
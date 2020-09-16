
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "main.h"
#include "dsPIC33F_IO.h"

void controlTX (char con)
{
    if (con) {
        ODCBbits.ODCB10 = 0;            // Open Drain Disabled
        TRISBbits.TRISB10 = 0;          // TX PIN Output
        PORTBbits.RB10 = 1;             // Turn on PIN
    } else {
        PORTBbits.RB10 = 0;             // Turn off PIN        
        TRISBbits.TRISB10 = 1;          // TX PIN Input
        ODCBbits.ODCB10 = 1;            // Open Drain Enabled
    }
}

void controlRX (char con)
{
    if (con) {
        ODCBbits.ODCB11 = 0;            // Open Drain Disabled
        TRISBbits.TRISB11 = 0;          // TX PIN Output
        PORTBbits.RB11 = 1;             // Turn on PIN
    } else {
        PORTBbits.RB11 = 0;             // Turn off PIN        
        TRISBbits.TRISB11 = 1;          // TX PIN Input
        ODCBbits.ODCB11 = 1;            // Open Drain Enabled
    }
}


void setupIO (void)
{
    ODCA = 0;                       // Disable Open Drain Ouputs
    ODCB = 0;

    PMCON = 0;                      // Disable Parallel Port Pins
    PMAEN = 0;

    AD1CON1 = 0;                    // Disable ADC
    AD1CON2 = 0;
    AD1CSSL = 0;
    AD1PCFGL = 0xFFFF;              // All ports are Digital - No analog ports

    TRISBbits.TRISB13 = 0;           // LED
    TRISBbits.TRISB11 = 0;           // RX PIN
    TRISBbits.TRISB10 = 0;           // TX PIN

    // Enable Open Drain
//    ODCBbits.ODCB10 = 0;             // Open Drain
//    ODCBbits.ODCB11 = 0;
  
    setupPPS();
    
    PORTBbits.RB13 = 0;
    PORTBbits.RB11 = 0;
    PORTBbits.RB10 = 0;

}

void setupPPS(void)
{

/// Unlock PPS Registers
asm volatile (
"MOV #OSCCON, w1 \n"
"MOV #0x46, w2 \n"
"MOV #0x57, w3 \n"
"MOV.b w2, [w1] \n"
"MOV.b w3, [w1] \n"
"BCLR OSCCON,#6"
);

// U1 - RP15 is Rx & RP14 for Tx
    RPINR18bits.U1CTSR = 0b11111;       // CTS tied to Vss (GND)
    RPINR18bits.U1RXR = 15;             // U1RX tied to RP15 (RB15/AN9)
    RPOR7bits.RP14R = 0b00011;          // U1TX tied to RP14 (RB14/AN10)

// Lock Registers
asm volatile (
"MOV #OSCCON, w1 \n"
"MOV #0x46, w2 \n"
"MOV #0x57, w3 \n"
"MOV.b w2, [w1] \n"
"MOV.b w3, [w1] \n"
"BSET OSCCON, #6"
);


}

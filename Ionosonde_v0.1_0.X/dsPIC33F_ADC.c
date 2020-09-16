
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "main.h"
#include "dsPIC33F_IO.h"
#include "dsPIC33F_UART.h"
#include "dsPIC33F_ADC.h"


// Program Flags
extern volatile unsigned int flags;

// ADC Buffers
volatile unsigned int adcbuf[ADC_BUFFER_SIZE];
volatile unsigned int adctr;

// Timer
extern volatile unsigned int t1start, t1stop;

void resetADC (void)
{
    AD1CON1 = 0;
    AD1CON2 = 0;
    AD1CON3 = 0;
    AD1CON4 = 0;
    AD1CSSL = 0;
    AD1CHS0 = 0;
    AD1CHS123 = 0;    
}

void ADCSetup (void)
{
    resetADC();
    
/////// Set up data format
    // Set 10-bit or 12-bit mode 
    AD1CON1bits.AD12B = 0;                  // Set to 10 bit mode
    AD1CON1bits.FORM = 0;                   // Unsigned Integer captured
    
/////// Setup range of conversion
    // Set voltage reference source
    AD1CON2bits.VCFG = 0b000;               // VREFH = Avdd, VREFL=Avss
    
/////// Set up ADC Timing
    // Set Sample Clock Source Select bits. Set how sampling end and conversion begins.
    AD1CON1bits.SSRC = 0b111;               // Internal counter ends sampling and starts conversion (auto-convert)
    AD1CON3bits.ADRC = 0;                   // Set to  internal system clock for ADC clock source
    // Set time to sample input (i.e. time to charge SH cap) 
    // TAD for dsPIC33EV is 73ns.  Tau for Sample Hold capacior is about 14ns, and 
    // 5xTau = 73ns.  1Tad should be sufficient to charge capacitor
    // Set ADC Conversion Clock Source .
    AD1CON3bits.SAMC = 2;                   // Set Auto sample time to 1 Tad, to be at least Data Sheet Tsamp
    // AD1CON3bits.ADCS is Conversion time (i.e. time to convert to digital value): 
    // Need min 117.6ns for 14TAD, for 80Mhz Osc, Tcy = 2/80=25ns
    // TAD = 117.6ns = Tcy (ADCS+1) = 25ns (4 + 1) = 125ns, 
    // ADCS = n+1, for ADCS = 4, Register set to 4 = 0b00100 for max sample rate
    // for ADCS = 12, TAD = 14ns(12+1) = 182ns
//    AD1CON3bits.ADCS = 4;                   // Set to 4 for max sample time (about 574Khz) 
    AD1CON3bits.ADCS = 6;                // Set to 6 for 409 Khz (2.44us per sample) 
    
    // Set port pins for analog inputs 
    AD1PCFGLbits.PCFG0 = 0;                 // PCFG0 or AN0 set for analog

    // Set Sample/Hold channels 
    AD1CHS0bits.CH0NB = 0;                  // Set Channel 0 Negative Input for MUX B as VREFL   
    AD1CHS0bits.CH0NA = 0;                  // Set Channel 0 Negative Input for MUX A as VREFL 
    AD1CHS0bits.CH0SA = 0;                  // Set Channel 0 positive input is AN0 for MUX A
    AD1CHS0bits.CH0SB = 0;                  // Set Channel 0 positive input is AN0 for MUX A 
    
    // Set how sampling will occur 
    AD1CON2bits.CSCNA = 0;                  // Do not scan Inputs.   
    AD1CON2bits.CHPS = 0;                   // Only use CH0 for input 
    AD1CON2bits.SMPI = 0b0;                 // Generate interrupt after each conversions
//    AD1CON2bits.SMPI = 0b1111;              // Generate interrupt after 16 conversions
    AD1CON2bits.BUFM = 0;                   // Fill buffer from the Start address
    AD1CON2bits.ALTS = 0;                   // uses channel input selects for Sample MUX A
    AD1CON1bits.ADSIDL = 0;                 // ADC Continues operation in Idle mode
    
    // Setup Interrupt
    IPC3bits.AD1IP = ADC_INTERRUPT_PRIORITY;    // Priority
    IEC0bits.AD1IE = 1;                     // Enabling ADC1 interrupt.

}   

void startSampling (void)
{
    flags |= ENABLE_SAMPLING;
    flags &= ~ADC_BUFFER_READY;
    adctr = 0;

    // Start Sampling. SAMP bit is auto-set after a conversion
    AD1CON1bits.ASAM = 1;                   // Sampling begins immediately after last conversion; 
    AD1CON1bits.SAMP = 1;                   // Start sampling. 

    // Enable ADC
    AD1CON1bits.ADON = 1;  
//    Delay_us(20);
}

void stopSampling (void)
{
    flags &= ~ENABLE_SAMPLING;
    // Stop Sampling
    AD1CON1bits.ASAM = 0;                   
    AD1CON1bits.SAMP = 0;

    // Disable ADC
    AD1CON1bits.ADON = 0;                 
    
}

void Delay_us(unsigned int delay)
{
    int i;
    for (i = 0; i < delay; i++)
    {
        __asm__ volatile ("repeat #39");
        __asm__ volatile ("nop");
    }
}
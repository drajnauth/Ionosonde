#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <dsp.h>

#include "main.h"

#include "dsp_factors_32b.h"
#include "dsPIC33F_FFT.h"
#include "dsPIC33F_I2C.h"
#include "dsPIC33F_Timer.h"


#include "dsPIC33F_IO.h"
#include "dsPIC33F_UART.h"
#include "dsPIC33F_ADC.h"
#include "VE3OOI_Si5351_v2.0.h"


// FBS
#pragma config BWRP = WRPROTECT_OFF     // Boot Segment Write Protect (Boot Segment may be written)
#pragma config BSS = NO_FLASH           // Boot Segment Program Flash Code Protection (No Boot program Flash segment)
#pragma config RBS = NO_RAM             // Boot Segment RAM Protection (No Boot RAM)

// FSS
#pragma config SWRP = WRPROTECT_OFF     // Secure Segment Program Write Protect (Secure segment may be written)
#pragma config SSS = NO_FLASH           // Secure Segment Program Flash Code Protection (No Secure Segment)
#pragma config RSS = NO_RAM             // Secure Segment Data RAM Protection (No Secure RAM)

// FGS
#pragma config GWRP = OFF               // General Code Segment Write Protect (User program memory is not write-protected)
#pragma config GSS = OFF                // General Segment Code Protection (User program memory is not code-protected)

// FOSCSEL
#pragma config FNOSC = FRCPLL           // Oscillator Mode (Internal Fast RC (FRC) w/ PLL)
#pragma config IESO = OFF               // Internal External Switch Over Mode (Start-up device with user-selected oscillator source)

// FOSC
#pragma config POSCMD = NONE            // Primary Oscillator Source (Primary Oscillator Disabled)
#pragma config OSCIOFNC = OFF            // OSC2 Pin Function (OSC2 pin has clock out function)
#pragma config IOL1WAY = OFF            // Peripheral Pin Select Configuration (Allow Multiple Re-configurations)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor (Both Clock Switching and Fail-Safe Clock Monitor are disabled)

// FWDT
#pragma config WDTPOST = PS1            // Watchdog Timer Postscaler (1:1)
#pragma config WDTPRE = PR32            // WDT Prescaler (1:32)
#pragma config WINDIS = OFF             // Watchdog Timer Window (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (Watchdog timer enabled/disabled by user software)

// FPOR
#pragma config FPWRT = PWR128           // POR Timer Value (128ms)
#pragma config ALTI2C = OFF             // Alternate I2C  pins (I2C mapped to SDA1/SCL1 pins)

// FICD
#pragma config ICS = PGD1               // Comm Channel Select (Communicate on PGC1/EMUC1 and PGD1/EMUD1)
#pragma config JTAGEN = OFF             // JTAG Port Enable (JTAG is Disabled)



// FFT Buffer
long signal[FFT_BLOCK_LENGTH] __attribute__ ((space(ymemory), far, aligned(FFT_BLOCK_LENGTH*2*2), far));

// UART Variables
extern volatile char RcvBuff[RBUFF];                            // UART TTY Serial Command Buffer
extern volatile char commands[MAX_COMMAND_ENTRIES];
extern volatile unsigned long  numbers[MAX_COMMAND_ENTRIES];
extern volatile unsigned char rchar, ibuf;                      // UART Serial Command Counters

// Program Flags
volatile unsigned int flags;

// ADC Buffers
extern volatile unsigned int adcbuf[ADC_BUFFER_SIZE];
extern volatile unsigned int adctr;
volatile unsigned int missed;
volatile unsigned int adcmax, adcmin, adcdc;
volatile long adcscale;
volatile long adctotal;

unsigned int peakFrequencyBin = 0; 
unsigned long peakFrequency = 0;
long long ffttotal, fftbintotal;
long fftavg, fftpeakavg, fftmaxavg, fftavgthresh;

unsigned char notfind, findavgctr;
unsigned int finds, findloc, findmaxavg;
double ratio, ratiol, ratioh;
long long sumsq;
double normcorr;


unsigned long tstart, tpulse, tsample, tend;
double height, hzcorr;

unsigned int signalRef[15] ={
2,
2,
4,
5,
8,
20,
47,
512,
266,
37,
14,
7,
8,
6,
8
};

#define SUMSQREF 337400


FoundStruct fnd;

#define Si5351_Calibration 138
//#define DEBUG_PRINT  

////////////////////////////
// Interrupt Handlers
///////////////////////////
void __attribute__ ( ( __interrupt__ , auto_psv ) ) _ADC1Interrupt ( void )
{
    // clear the ADC interrupt flag
    IFS0bits.AD1IF = 0;
    if (! (flags & ENABLE_SAMPLING)) { 
        return;
        
    } else {
        if (!adctr) {
            adcmin = adcmax = ADC1BUF0;
            adctotal = adcdc = 0;
        }
        adcbuf[adctr++] = ADC1BUF0;
        adctotal += ADC1BUF0;
        if (adcmax < ADC1BUF0) adcmax = ADC1BUF0;
        if (adcmin > ADC1BUF0) adcmin = ADC1BUF0;

        if (adctr >= ADC_BUFFER_SIZE) {
            stopSampling();
            flags |= ADC_BUFFER_READY;
        }
    }

}

void __attribute__((__interrupt__,no_auto_psv)) _U1TXInterrupt(void)
{
    IFS0bits.U1TXIF = 0; // clear TX interrupt flag
}


void __attribute__((__interrupt__,no_auto_psv)) _U1RXInterrupt(void)
{
// Interrupt handler for Uart 1
    IFS0bits.U1RXIF = 0; // clear RX interrupt flag
    
    LoadTTYBuffer ();

    if (U1STAbits.PERR || U1STAbits.FERR || U1STAbits.OERR) {
        U1STAbits.PERR = 0;
        U1STAbits.FERR = 0;
        U1STAbits.OERR = 0;
    }
}


void __attribute__((__interrupt__,no_auto_psv)) _T1Interrupt(void)
{
/* Using 1:1 prescaler and 16bit counter overflows about every 1.6 ms
 * Fosc = 79.2275 Mhz, and Fcy = Fosc/2 = 39.614 Mhz
 * With 1:1 and 16bit Counter overflow every 1/Fcy*65536=1.6ms
 * 1 Tick = 1/Fcy = 25ns
 */

// Clear Timer1 interrupt
    IFS0bits.T1IF = 0;
    LED = ~LED;   
    stopTimer(1);
    flags &= ~T1RUNNING;

}

void __attribute__((__interrupt__,no_auto_psv)) _T2Interrupt(void)
{
/* Using 1:1 prescaler and 16bit counter overflows about every 1.6 ms
 * Fosc = 79.2275 Mhz, and Fcy = Fosc/2 = 39.614 Mhz
 * With 1:1 and 16bit Counter overflow every 1/Fcy*65536=1.6ms
 * 1 Tick = 1/Fcy = 25ns
 */

// Clear Timer2 interrupt
    IFS0bits.T2IF = 0;
}



int main(void) {
    unsigned long blkcnt1;
    
    while (OSCCONbits.COSC != 0b001);	// Wait for PLL as Current Oscillator Selection
    RCONbits.SWDTEN = 0;                // Disable Watchdog Timer
    OSCTUN = 0b011111;                  // Increase RC OSC by x%

    OSCCON = 0;
	// Configure FRC to operate the device at 40MIPS
	// Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
	// Fosc= 7.37M*43/(2*2)=79.2275Mhz for 40MIPS input clock
    CLKDIV = 0;                     // N2: Divide by 2, N1: Divide by 2
    CLKDIVbits.PLLPOST = 0;         // N2: Divide by 2
    CLKDIVbits.PLLPRE = 0;          // N1: Divide by 2
    PLLFBD = 0x2A;                  //M1 is 43

#ifndef FOSC
#define FOSC    (79227500)
//#define FCY     (FOSC/2)
#define FCY      (unsigned long)40209000 //default FCY 40MHz
#define TCY     (25)                     //25ns
#endif
    
    
    while(OSCCONbits.LOCK  != 1);    // Wait for PLL is locked, or PLL start-up timer is satisfied

    OSCTUN = 0b111111;                  // Measured Frequency Fcy=40.209 Mhz
    
    setupSi5351 (Si5351_Calibration);
    setupIO ();
    ADCSetup();
    setupTTYUART ();

    Reset();
    sendString (VERSIONHEADER);
    
    LED = 0;
    
    while (1) {
        if (!(flags & LEDOFF) || !(flags & ENABLE_SAMPLING) || !(flags & T1RUNNING)) { 
            if (blkcnt1++ > 0xEFFF) {
                LED = ~LED;
                blkcnt1 = 0;
            }
        }
       
        if (flags & EXECUTE_TTY) {
            sendString ("\r\n");
            Execute ();
            memset((void *)RcvBuff, 0, sizeof(RcvBuff));
            ibuf = 0;
            sendString ("\r\nRDY> ");
            flags |= MONITOR_TTY;
            flags &= ~EXECUTE_TTY;
        }
        
    }

}

void calculateADCScale (void) 
{
    adctotal /= (long)ADC_BUFFER_SIZE;
    adcdc = (unsigned int)adctotal;
    adcscale = 0x8FFFFFFF/2/(adcmax-adcdc);
    
    if (adcscale > (long)MAXADCSCALE) adcscale = (long)MAXADCSCALE;
}


long FFTAverage (unsigned long freq, unsigned int sbin, unsigned int ebin) 
{
    unsigned int i, j, k;
    long long bufftotal;
    
    controlRX (0);
    controlTX (0);
    flags &= ~ADC_BUFFER_READY;
    flags &= ~ENABLE_SAMPLING;
    setupSi5351 (Si5351_Calibration);
    
    SetFrequency (SI_CLK0, SI_PLL_A, freq);
    EnableSi5351Clock (SI_CLK0);
    controlRX (1);
    
    ADCSetup ();
    memset ((void *)adcbuf, 0, sizeof(adcbuf));
    
    startSampling();
    
    //Wait for Sample to Complete
    // Taking ADC_BUFFER_SIZE (2048) samples at 400 Khz which is about 5.1 ms
    while (!(flags & ADC_BUFFER_READY) );
    controlRX (0);
    flags &= ~ADC_BUFFER_READY;
    flags &= ~ENABLE_SAMPLING;
    PowerDownSi5351Clock (SI_CLK2);
    PowerDownSi5351Clock (SI_CLK0);

    calculateADCScale();

    bufftotal = 0;
    for(k=0,i=0; i<(ADC_BUFFER_SIZE-FFT_BLOCK_LENGTH); i+=32) {        // FFT_BLOCK_LENGTH
        memset ((void *)&signal, 0, sizeof(signal));
        for (j=0; j<FFT_BLOCK_LENGTH; j++) {
            signal[j] = ((long)adcbuf[i+j] - (long)adcdc)*adcscale;
        }

        FFTReal32bIP ( (LOG2_BLOCK_LENGTH - 1), FFT_BLOCK_LENGTH, (long *) &signal[0], 
            (long *) __builtin_psvoffset(&psvTwdlFctr32b[0]), 
            (int) __builtin_psvpage(&psvTwdlFctr32b[0]));
        MagnitudeCplx32bIP((FFT_BLOCK_LENGTH/2)+1,signal);
                    
        for (j=sbin; j<=ebin; j++) {
            bufftotal += (long long)signal[j];
            k++;
        }
  
    }
    bufftotal /= (long long)(k+1);

    return (long)bufftotal;
}


unsigned char FFTSearchBuffer (unsigned int fbin, unsigned char mult)
{
    unsigned int i, j, k;
    long sub[FFT_AVG_BINH - FFT_AVG_BINL + 1];
    long long xsusq;
    
    memset ((void *)&fnd, 0, sizeof(fnd));
    
    findloc = finds = 0;
    notfind = 0;
    
    calculateADCScale();
    
    fftbintotal = 0;
    findmaxavg = 0;
    fftmaxavg = 0;
    findavgctr = 0;
    for(i=0; i<(ADC_BUFFER_SIZE-FFT_BLOCK_LENGTH); i++) {        // FFT_BLOCK_LENGTH
        memset ((void *)&signal, 0, sizeof(signal));
        for (j=0; j<FFT_BLOCK_LENGTH; j++) {
            signal[j] = ((long)adcbuf[i+j] - (long)adcdc)*adcscale;
        }

        FFTReal32bIP ( (LOG2_BLOCK_LENGTH - 1), FFT_BLOCK_LENGTH, (long *) &signal[0], 
            (long *) __builtin_psvoffset(&psvTwdlFctr32b[0]), 
            (int) __builtin_psvpage(&psvTwdlFctr32b[0]));
        MagnitudeCplx32bIP((FFT_BLOCK_LENGTH/2)+1,signal);
                    
        signal[0] = signal[1] = 0;

// Load passband values to determine normalized correlation         
        fftavg = 0;
        memset ((void *)&sub, 0, sizeof(sub));
        for (k=FFT_AVG_BINL; k<=FFT_AVG_BINH; k++) {
            sub[k-FFT_AVG_BINL] = signal[k];
            if (signal[k] > fftavg) fftavg = signal[k];     // the maximum value.
        }
        
// Calculate normalized correlation         
        ffttotal = sumsq = 0;
        for (k=0; k<=(FFT_AVG_BINH - FFT_AVG_BINL); k++) {
            ratio = ((double)sub[k] / (double) fftavg) * (double)SIGNAL_SCALE;
            sub[k] = (long)ratio;
            sumsq += ((long long) (sub[k]*sub[k]));
            ffttotal += ((long long)(sub[k]*signalRef[k]));
        }      
        xsusq = (long long)SIGNAL_REF_SQ*(long long)sumsq;
        normcorr = (double)ffttotal / (double) sqrtl((long double)xsusq);
        
// average of passband        
        ffttotal = 0;
        for (k=FFT_AVG_BINL; k<=FFT_AVG_BINH; k++) {
            ffttotal += signal[k];
        }
        ffttotal /= (long long)(FFT_AVG_BINH-FFT_AVG_BINL+1);
        fftpeakavg = (long)ffttotal;
           
        ratiol = (double)signal[FFT_100K_BIN-1] / (double)fftpeakavg; 
        ratio = (double)signal[FFT_100K_BIN] / (double)fftpeakavg; 
        ratioh = (double)signal[FFT_100K_BIN+1] / (double)fftpeakavg;

        
#ifdef DEBUG_PRINT  
//        for (k=FFT_AVG_BINL; k<=FFT_AVG_BINH; k++) {
//           printf ("%ld,",signal[k]);
//        }
#endif       
       
        if (ratio >= (double)mult && fftpeakavg > fftavgthresh && normcorr > (double)FFT_CORR_THRESH ) {
            if (!findloc) {
                findloc = i; 
                finds = 1;
                fnd.found = 1;
                fftbintotal = signal[FFT_100K_BIN];
                findavgctr = 1;
            } else {
                finds++;
                findavgctr++;
                fftbintotal += signal[FFT_100K_BIN];
                if (findavgctr >= 4) {
                    fftbintotal /= findavgctr;
                    findavgctr = 0;
                    if (fftbintotal > fftmaxavg) {
                        fftmaxavg = fftbintotal;
                        findmaxavg = i;
                    }
                }    
                
            }
            
            if (notfind) {
                if (--notfind < 2) notfind = 0;
            }
            
#ifdef DEBUG_PRINT  
//        sendString ("1\r\n");
#endif 

#ifndef DEBUG_PRINT               
            sendChar('+');  
#endif
           
        } else {
#ifndef DEBUG_PRINT               
            sendChar('.');  
#endif

#ifdef DEBUG_PRINT  
//        sendString ("0\r\n");
#endif        
            

            if (notfind < 3) {
               notfind++;
            }
            if (findloc && finds && notfind > 2) {
                pushFinds (findloc, finds, findmaxavg);
                findloc = 0;
                finds = 0;
                fftbintotal = 0;
                findavgctr = 0;
            }          
        } 
    }
    
    if (findloc && finds)
        if (!fnd.finds[0] || !fnd.finds[1] || !fnd.finds[2]) {
        pushFinds (findloc, finds, findmaxavg);
    }          
    sendString ("\r\n");
    return fnd.found;
    
}


void pushFinds (unsigned int findloc, unsigned int finds, unsigned int maxavgpos)
{    
    if (finds > fnd.finds[0]) {
        fnd.finds[2] = fnd.finds[1];
        fnd.findloc[2] = fnd.findloc[1];
        fnd.maxavgpos[2] = fnd.maxavgpos[1];
        fnd.finds[1] = fnd.finds[0];
        fnd.findloc[1] = fnd.findloc[0];
        fnd.maxavgpos[1] = fnd.maxavgpos[0];
        fnd.finds[0] = finds;
        fnd.findloc[0] = findloc;
        fnd.maxavgpos[0] = maxavgpos;
       
    } else if (finds < fnd.finds[0] && finds > fnd.finds[1]) {
        fnd.finds[2] = fnd.finds[1];
        fnd.findloc[2] = fnd.findloc[1];
        fnd.maxavgpos[2] = fnd.maxavgpos[1];
        fnd.finds[1] = finds;
        fnd.findloc[1] = findloc;
        fnd.maxavgpos[1] = maxavgpos;
         
    } else if (finds < fnd.finds[1] && finds > fnd.finds[2]) {
        fnd.finds[2] = finds;
        fnd.findloc[2] = findloc;
        fnd.maxavgpos[2] = maxavgpos;
         
    }
        
}


void detectEcho (unsigned char mult)
{
    unsigned int i;
    double timeus;
    
   //Start Collecting Data
    controlRX (1);
    
    tend = (unsigned long)TMR2;
    startSampling();
    
    // Wait for Sample to Complete
    // Taking ADC_BUFFER_SIZE (2048) samples at 400 Khz which is about 5.1 ms
    while (!(flags & ADC_BUFFER_READY) );
  
// This does not work because ADC int blocks timer interrupt.
//    tend2 = (unsigned long)TMR2;        // this is the time to take 2048 samples
    
    stopTimer(2);    
    controlRX (0);
    flags &= ~ADC_BUFFER_READY;
    flags &= ~ENABLE_SAMPLING;
    PowerDownSi5351Clock (SI_CLK2);
    PowerDownSi5351Clock (SI_CLK0);

    i = FFTSearchBuffer (FFT_100K_BIN, mult);
    height = (double) (tpulse - tstart);
    timeus = (double)Si5351OVERHEAD * (double)TIMERUSCNT;;
    if (height > timeus) height -= timeus;
    height /= (double)TIMERUSCNT;
    printf ("Pulse Tx Time: %4.2f us\r\n", height);
    
    height = (double)(tend - tstart);
    height /= (double)TIMERUSCNT;
    printf ("Rx Start Time: %4.2f us\r\n", (height));
    
    if (i) {
        printf ("Potential Echo at: %u, Max Position: %u (%u), Total Finds: %u\r\n", fnd.findloc[0], fnd.maxavgpos[0], fnd.maxavgpos[0]-fnd.findloc[0], fnd.finds[0]);
        printf ("Potential Echo at: %u, Max Position: %u (%u), Total Finds: %u\r\n", fnd.findloc[1], fnd.maxavgpos[1], fnd.maxavgpos[1]-fnd.findloc[1], fnd.finds[1]);
        printf ("Potential Echo at: %u, Max Position: %u (%u), Total Finds: %u\r\n", fnd.findloc[2], fnd.maxavgpos[1], fnd.maxavgpos[2]-fnd.findloc[2], fnd.finds[2]);

        printf ("Echo Detected at element %u\r\n", fnd.findloc[0]);
        
        // TIME_PER_SAMPLE is the measured ADC interrupt rate i.e. time to take one sample
        timeus = height + ((double)TIME_PER_SAMPLE * (double)fnd.maxavgpos[0]/(double)2);
        height += ((double)TIME_PER_SAMPLE * (double)fnd.findloc[0]);
        printf ("Detect Time: %4.2f us, Adjusted Time: %4.2f\r\n", height, timeus);

        height *= (double)LIGHTSPEED;       // This is round trip time to and from
        height /= (double)2;
        
        timeus *= (double)LIGHTSPEED;       // This is round trip time to and from
        timeus /= (double)2;
        printf ("Raw Height: %lu, Adjusted Height: %lu meters\r\n", (unsigned long)height, (unsigned long)timeus);       
    }

    flags &= ~ADC_BUFFER_READY;
    flags &= ~DO_FFT;
    flags &= ~DO_FFT_DETECT;
    
}

void doIonosondeSample (unsigned long freq, unsigned int dly, unsigned char mult) 
{
    unsigned long ticks;
    LED = 1;
    
    if (!fftavgthresh) {
        sendString ("Calculating Peak Thresholds\r\n");
        fftpeakavg = FFTAverage (freq, FFT_AVG_BINL, FFT_AVG_BINH);
        fftavgthresh = (FFT_AVG_THRESH_MULT*fftpeakavg)/10;
    }
    printf ("Thresholds. Peak: %ld (%2.2f), CorrCoeff: %2.2f, MinSNR: %u\r\n", 
            fftavgthresh, (double)FFT_AVG_THRESH_MULT/(double)10, 
            (double)FFT_CORR_THRESH, (unsigned int)mult);
   
// Initialize everything to not introduce delay after pulse is send and detected
    // Get ADC Ready
    memset ((void *)adcbuf, 0, sizeof(adcbuf));
    ADCSetup ();
    
    controlRX (0);
    controlTX (0);
    flags &= ~ADC_BUFFER_READY;
    flags &= ~ENABLE_SAMPLING;
    setupSi5351 (Si5351_Calibration);
        
    // Enable LO for 100 Khz below freq
    SetFrequency (SI_CLK0, SI_PLL_A, (freq-100000));
    SetFrequency (SI_CLK2, SI_PLL_B, freq);
    EnableSi5351Clock (SI_CLK0);

    if (dly > (unsigned int)Si5351OVERHEAD) {
        ticks = (unsigned long)dly - (unsigned long)Si5351OVERHEAD;
        ticks *= (unsigned long)TIMER100US;
        ticks /= (unsigned long)100;
    } else ticks = (unsigned long)0;
   
    // Timer used to determine time for echo
    setupTimer (2, 0xFFFF);  
    startTimer (2); 
    sendPulse (ticks);  // Send pulse for 500 us, it takes 360 us to execute routine   
    
    detectEcho (mult);

    LED = 0;
}


// Send a pulse at freq Hz for us microseconds
void sendPulse (unsigned int ticks)
{
    unsigned long tickinc;
    
    controlTX (1);
    tstart = (unsigned long)TMR2;
    EnableSi5351Clock (SI_CLK2); 

    tpulse = tickinc = (unsigned long)TMR2;
    while ( (unsigned int)(tpulse - tickinc) < ticks ) {
        tpulse = (unsigned long)TMR2;
    }
    
    DisableSi5351Clock (SI_CLK2);
    tpulse = (unsigned long)TMR2;
    controlTX (0);
}


void timerUSDelay (unsigned int us)
{
    unsigned int i;
    if (us > 1628) {
        i = 0xFFFF;
    } else if (us < 20) {
        i = ((unsigned int)(TIMERUSCNT_X4 * 20)>>2);
    } else {
        i = (unsigned int) ((unsigned long)(((unsigned long)TIMERUSCNT_X4) * us)>>2);
    }
    setupTimer (1, i);  
    flags |= T1RUNNING;
    startTimer (1);  
    while (flags & T1RUNNING);          
}

void Execute (void) 
{
    unsigned int num, i, j;
    
    num = parse ( (char *)RcvBuff );
    switch (commands[0]) {

        
        case 'A':  // Get average
            if (numbers[0] < (unsigned long)SI_MIN_OUT_FREQ || numbers[0] > (unsigned long)SI_MAX_OUT_FREQ) {
                sendString ("Err\r\n");
                break;
            }
            sendString ("Calculating Peak Thresholds\r\n");
            fftpeakavg = FFTAverage (numbers[0], FFT_AVG_BINL, FFT_AVG_BINH);
            fftavgthresh = (FFT_AVG_THRESH_MULT*fftpeakavg)/10;
            printf ("Noise Average: %ld\r\n", fftavgthresh);
            break;

        case 'E':             // enable output pin
            if (commands[1] == 'T') {
                if (flags & TXENABLED) {
                    controlTX (0);
                    flags &= ~TXENABLED;
                    sendString ("TX CTL DIS");
                } else {
                    controlTX (1);
                    flags |= TXENABLED;;
                    sendString ("TX CTL EN");
                }
        
            } else if (commands[1] == 'R'){
                if (flags & RXENABLED) {
                    controlRX (0);
                    flags &= ~RXENABLED;
                    sendString ("RX CTL DIS");
                } else {
                    controlRX (1);
                    flags |= RXENABLED;
                    sendString ("RX CTL EN");
                }
        
            } else sendString ("Err");
            break;

        case 'F':                           // Help                      
            // Validate inputs
            if (numbers [0] > 2) {
                sendString ("Bad Channel");
                break;
            }
            if (commands [1] != 'A' && commands [1] != 'B') {
                sendString ("Bad PLL");
                break;
            }
            
            if (numbers[1] < SI_MIN_OUT_FREQ || numbers[1] > SI_MAX_OUT_FREQ) {
                sendString ("Frequency out of range");
                break;
            }
      
            // set frequency
            if (numbers[0] == 0) {
                if (commands[1] == 'B') SetFrequency (SI_CLK0, SI_PLL_B, numbers[1]);
                else SetFrequency (SI_CLK0, SI_PLL_A, numbers[1]);
                EnableSi5351Clock (SI_CLK0);

        
            } else if (numbers[0] == 1) {
                if (commands[1] == 'B') SetFrequency (SI_CLK1, SI_PLL_B, numbers[1]);
                else SetFrequency (SI_CLK1, SI_PLL_A, numbers[1]);  
                EnableSi5351Clock (SI_CLK1);
             
            } else if (numbers[0] == 2) {
                if (commands[1] == 'B') SetFrequency (SI_CLK2, SI_PLL_B, numbers[1]);
                else SetFrequency (SI_CLK2, SI_PLL_A, numbers[1]);  
                EnableSi5351Clock (SI_CLK2);
       
            }
            break;
            
        case 'H':                           // Help
            sendString ("A [freq] - Average Noise\r\n");
            sendString ("E [T] [R] - Enable/Toggle Rx/Tx PIN Diode\r\n");
            sendString ("F [clk] [freq] - Set & Enable Frequency\r\n");
            sendString ("I [freq] [Tx dly] [SNR] - Do Ionosonde\r\n");
            sendString ("R - Reset\r\n");
            sendString ("S [freq] [snr] - Test Receiver\r\n");
            break;

        case 'I':               // Do Ionosonde Sample
            if (numbers[0] < (unsigned long)SI_MIN_OUT_FREQ || numbers[0] > (unsigned long)SI_MAX_OUT_FREQ) {
                sendString ("Err\r\n");
                break;
            }
            if (!numbers[1] || numbers[1] > (unsigned long)5000) {  // Tx time in us
                sendString ("Err\r\n");
                break;
            }
            if (!numbers[2] || numbers[2] > (unsigned long)10) {    // Min SNR
                sendString ("Err\r\n");
                break;
            }
            if (commands[1] == 'Z') {
                fftavgthresh = 0;
            }
            doIonosondeSample ((unsigned long)numbers[0], (unsigned int)numbers[1], (unsigned char)numbers[2]);
            break;
            
        case 'R':                       // Reset system
            Reset ();
            break;
            
        case 'S':                       // Sample
            if (numbers[0] < (unsigned long)SI_MIN_OUT_FREQ || numbers[0] > (unsigned long)SI_MAX_OUT_FREQ) {
                sendString ("Err\r\n");
                break;
            }
            
            if (!numbers[1] || numbers[1] > (unsigned long)10) {    // Min SNR
                sendString ("Err\r\n");
                break;
            }
            
            LED = 1;
    
            sendString ("Calculating Peak Thresholds\r\n");
            fftpeakavg = FFTAverage (numbers[0], FFT_AVG_BINL, FFT_AVG_BINH);
            fftavgthresh = (FFT_AVG_THRESH_MULT*fftpeakavg)/10;
            printf ("Thresholds. Peak: %ld (%2.2f), CorrCoeff: %2.2f, MinSNR: %u\r\n", 
                fftavgthresh, (double)FFT_AVG_THRESH_MULT/(double)10, 
                (double)FFT_CORR_THRESH, (unsigned int)numbers[1]);
   
            memset ((void *)adcbuf, 0, sizeof(adcbuf));
            ADCSetup ();
    
            controlRX (0);
            controlTX (0);
            flags &= ~ADC_BUFFER_READY;
            flags &= ~ENABLE_SAMPLING;
            setupSi5351 (Si5351_Calibration);
        
            // Timer used to determine time for echo
            setupTimer (2, 0xFFFF);  
            startTimer (2);
            
            // Enable LO for 100 Khz below freq
            SetFrequency (SI_CLK0, SI_PLL_A, (numbers[0]-100000));
            tstart = (unsigned long)TMR2;
            EnableSi5351Clock (SI_CLK0);
            Delay_us (500);
            tpulse = (unsigned long)TMR2;
            
            detectEcho ((unsigned char)numbers[1]);  
            break;

        case 'X':                       // Sample 
            LED = 0;
            flags &= ~ADC_BUFFER_READY;
            flags &= ~ENABLE_SAMPLING;
            ADCSetup ();
            startSampling();
            while (!(flags & ADC_BUFFER_READY) );
            LED = 0;
            break;
            
            
            
            
            
        default:
            DecodeError (0);
    }
}




void Reset ( void )
{
    controlTX (0);
    controlRX (0);
    
    setupSi5351 (Si5351_Calibration);
    
    stopTimer(1);
    stopTimer(2);
    
    memset((void *)RcvBuff, 0, sizeof(RcvBuff));
	memset ((char *)numbers, 0, sizeof(numbers));
	memset ((char *)commands, 0, sizeof(commands));
    memset ((void *)adcbuf, 0, sizeof(adcbuf));
 
    rchar = ibuf = 0;

    flags = 0;

    missed = adcmax = adcmin = adcdc = 0;
    adcscale = adctotal = 0;

    peakFrequencyBin = 0; 
    peakFrequency = 0;
    ffttotal = sumsq = 0;
    fftavg = fftpeakavg = fftavgthresh = 0;

    notfind = finds = findloc = 0;
    height = normcorr = ratio = ratiol = ratioh = (double)0.0;

    tstart = tend = tpulse = tsample = 0;

    resetADC();
 
    flags = flags | MONITOR_TTY;
}

void DecodeError (unsigned int ErrCode)
{
    switch (ErrCode) {
        case 1:
            sendString ("No XYZ\r\n");
            break;

        default:
            sendString ("Err\r\n");

    }

}

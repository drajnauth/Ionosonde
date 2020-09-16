
#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H

#define VERSIONHEADER "\r\n\r\nVE3OOI dsPIC IONOSONDE v0.1_1\r\n\r\nRDY> "

//#define DEBUG_PRINT               

// Flag Defines
#define CHAR_READY 0x1
#define MONITOR_TTY 0x2
#define EXECUTE_TTY 0x4
#define ENABLE_SAMPLING 0x8
#define ADC_BUFFER_READY 0x10
#define NO_FFT 0x20
#define DO_FFT 0x40
#define DO_FFT_DETECT 0x80
#define TXENABLED 0x100
#define RXENABLED 0x200
#define T1RUNNING 0x400
#define T2RUNNING 0x800
#define LEDOFF 0x1000

#define ADC_BUFFER_SIZE 2048

#define FFT_100K_BIN 62                   
#define FFT_AVG_BINL 55                   
#define FFT_AVG_BINH 69                   
#define FFT_PEAK_MULT 4                 // was 35 for average across entire array only positive values  
#define SIGNAL_REF_SQ 337400
#define SIGNAL_SCALE 1024

#define MAXADCSCALE 4000000
#define AVGTHRESHOLD 450000


#define FFT_CORR_THRESH (0.7)   // Normalized Correlation coefficient threahold
#define FFT_AVG_THRESH_MULT 12      // this is the peak threahold compared to average. Its 10x i.e. if 12 it means 12/10=1.2
#define FFT_DET_THRESHOLD 5
#define FFT_DET_FORGIVENESS 3
#define LIGHTSPEED (299.792458)
#define Si5351OVERHEAD 281
#define HZCORRECTION 22890

        
void Execute (void);     
void Reset (void);
void DecodeError (unsigned int mult);
unsigned char FFTSearchBuffer (unsigned int fbin, unsigned char mult);
void sampleGenFFT (unsigned long freq, unsigned int num);

long FFTAverage (unsigned long freq, unsigned int sbin, unsigned int ebin);
void calculateADCScale (void);
       
void doIonosondeSample (unsigned long freq, unsigned int dly, unsigned char mult);
void sendPulse (unsigned int ticks);
void detectEcho (unsigned char mult);
void pushFinds (unsigned int findloc, unsigned int finds, unsigned int maxavgpos);

void timerUSDelay (unsigned int cnt);

#define MAXFINDS 3    
typedef struct {
    unsigned char found;
    unsigned int findloc[MAXFINDS];
    unsigned int finds[MAXFINDS];
    unsigned int maxavgpos[MAXFINDS];
} FoundStruct;

#endif	/* XC_HEADER_TEMPLATE_H */


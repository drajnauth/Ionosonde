

#ifndef DSPIC33E_ADC_H
#define	DSPIC33E_ADC_H

// Set priority of 0-7 Interrups. Higher number may interrup lower number
#define ADC_INTERRUPT_PRIORITY 5

#define TIME_PER_SAMPLEx256 624
#define TIME_PER_SAMPLE (2.431)          // 2.341us per sample...abount 411245 Hz sample time. This is the measured value

void startSampling (void);
void stopSampling (void);
void resetADC(void);
void ADCSetup (void);
void Delay_us(unsigned int delay);

#endif	/* DSPIC33E_ADC_H */


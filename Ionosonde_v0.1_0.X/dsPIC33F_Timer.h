/* 
 * File:   dsPIC_IO_v1.h
 * Author: ddr
 *
 * Created on March 21, 2015, 5:25 PM
 */

#ifndef DSPIC_TIMER_V1_H
#define	DSPIC_TIMER_V1_H

// TCY is 25 ns
// For 100us, 4000 ticks

#define TIMER100US  4025  // 4025/100 = 40.25 cnt/us
#define TIMER500US  20125   // 20125/500 = 40.25 cnt/us
#define TIMERUSCNT_X4  161  // Fixed point count per us  (i.e. need to divide result by 4)
#define TIMERUSCNT  (40.25) // Floating point count per us  
#define TIMERCNT  (24.8)    // 24.8 ns per count  


// Set priority of 0-7 Interrups. Higher number may interrupt lower number
#define TIMER_INTERRUPT_PRIORITY 7

//Time 1 Tick Interval
#define TIMER_TICK_TIME 25              // ns
#define TIMER_OVERFLOW_TIME 1654        // us

void setupTimer (unsigned char, unsigned int overflow);
void startTimer (unsigned char);
void stopTimer (unsigned char);

unsigned int getTimerValue (unsigned char tnum);

#endif	/* DSPIC_TIMER_V1_H */


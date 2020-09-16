/* 
 * File:   dsPIC33F_IO.h
 * Author: ddr
 *
 * Created on December 13, 2017, 10:26 PM
 */

#ifndef DSPIC33F_IO__H
#define	DSPIC33F_IO__H

#define LED PORTBbits.RB13
#define TX_PIN_ENABLE PORTBbits.RB10
#define RX_PIN_ENABLE PORTBbits.RB11

void setupIO (void);
void setupPPS(void);

void controlRX (char con);
void controlTX (char con);

#endif	/* DSPIC33F_IO__H */


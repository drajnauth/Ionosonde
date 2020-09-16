#ifndef I2C_H
#define	I2C_H

#include <xc.h>

#ifndef FCY
#define FCY             (unsigned long)40209000 //default FCY 40MHz
#endif
#include <libpic30.h>
#ifndef I2C_BAUDRATE
#define I2C_BAUDRATE    (unsigned long)100000 //default baud rate 100kHz
#endif
#define I2C_ERROR           -1
#define I2C_OK              1

void I2C_Init(void);
int setBaudRate(void);
int I2C_WriteReg(char dev_addr, char reg_addr, char value);
int I2C_ReadReg(char dev_addr, char reg_addr, char *value);
void I2C_ReadFromReg_Burst(char address, char deviceRegister, char* data, char burstNum);
int I2C_Test(char dev_addr);


#endif	/* I2C_H */


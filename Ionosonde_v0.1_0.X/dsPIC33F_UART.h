
#ifndef DSPIC_UART_H
#define	DSPIC_UART_H

// Serial buffer
#define RBUFF 20
// Maximum number of command and numbers
#define MAX_COMMAND_ENTRIES 4

// Set priority of 0-7 Interrups. Higher number may interrupt lower number
#define UART_INTERRUPT_PRIORITY 4


void setupTTYUART(void);
void EnableUart (void);
void DisableUart (void);

void LoadTTYBuffer (void);

char receiveChar( void  );
void sendDecNumber( unsigned long  );
void sendHex(unsigned int );
void sendChar( char value );
void sendString( char * );
unsigned char parse ( char * );


#endif	/* DSPIC_UART_H */


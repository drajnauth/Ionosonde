#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "main.h"
#include "dsPIC33F_IO.h"
#include "dsPIC33F_UART.h"

volatile char RcvBuff[RBUFF];                            // UART TTY Serial Command Buffer
volatile char commands[MAX_COMMAND_ENTRIES];
volatile unsigned long numbers[MAX_COMMAND_ENTRIES];

volatile unsigned char rchar, ibuf;                      // UART Serial Command Counters

extern volatile unsigned int flags;


void setupTTYUART(void)
{
// Configure UART 1
    U1MODEbits.UARTEN = 0;          // TX, RX DISABLED, ENABLE at end of func
    U1MODEbits.USIDL = 0;           // Continue in Idle
    U1MODEbits.IREN = 0;            // No IR translation
    U1MODEbits.RTSMD = 0;           // Simplex Mode
    U1MODEbits.UEN = 0;             // TX,RX enabled, CTS,RTS not
    U1MODEbits.WAKE = 0;            // No Wake up (since we don't sleep here)
    U1MODEbits.LPBACK = 0;          // No Loop Back
    U1MODEbits.ABAUD = 0;           // No Autobaud (would require sending '55')
    U1MODEbits.URXINV = 0;          // IdleState = 1  (for dsPIC)
    U1MODEbits.BRGH = 0;            // 16 clocks per bit period (Low Speed)
    U1MODEbits.PDSEL = 0;           // 8bit, No Parity
    U1MODEbits.STSEL = 0;           // One Stop Bit

// Load a value into Baud Rate Generator.  Example is for 9600.
//  U1BRG = (Fcy/(16*BaudRate))-1
//  U1BRG = (40.209M/(16*9600))-1
//  U1BRG = 260.7
//    U1BRG = 261;                    // 40.209Mhz osc, 9600 Baud
    U1BRG = 21;                     // 40.209Mhz osc, 115200 Baud
//    U1BRG = 43;                     // 40.209Mhz osc, 57600 Baud
//    U1BRG = 64;                     // 40.209Mhz osc, 38400 Baud

    U1STAbits.UTXISEL1 = 0;         //Int when Char is transferred (1/2 config!)
    U1STAbits.UTXINV = 0;           //N/A, IRDA config
    U1STAbits.UTXISEL0 = 0;         //Other half of Bit15
    U1STAbits.UTXBRK = 0;           //Disabled
    U1STAbits.UTXEN = 0;            //TX pins controlled by periph
    U1STAbits.URXISEL = 0;          //Int. on character recieved
    U1STAbits.ADDEN = 0;            //Address Detect Disabled
    
    EnableUart ();

}


void DisableUart (void)
{
    IFS0bits.U1TXIF = 0;            // Clear the Transmit Interrupt Flag
    IEC0bits.U1TXIE = 0;            // Disable Transmit Interrupts
    IFS0bits.U1RXIF = 0;            // Clear the Recieve Interrupt Flag
    IEC0bits.U1RXIE = 0;            // Enable Recieve Interrupts

// Set priority of 4 only. Interrups <= 4 are ignored until interrup has completed
    IPC2bits.U1RXIP = 0;
    IPC3bits.U1TXIP = 0;
    U1MODEbits.UARTEN = 0;          // Enable Uart.  Must be set first
    U1STAbits.UTXEN = 0;            // Must be set after UARTEN bit set

}

void EnableUart (void)
{
    IFS0bits.U1TXIF = 0;            // Clear the Transmit Interrupt Flag
    IEC0bits.U1TXIE = 0;            // Disable Transmit Interrupts
    IFS0bits.U1RXIF = 0;            // Clear the Recieve Interrupt Flag
    IEC0bits.U1RXIE = 1;            // Enable Recieve Interrupts

    IPC2bits.U1RXIP = UART_INTERRUPT_PRIORITY;
    IPC3bits.U1TXIP = UART_INTERRUPT_PRIORITY;
    U1MODEbits.UARTEN = 1;          // Enable Uart.  Must be set first
    U1STAbits.UTXEN = 1;            // Must be set after UARTEN bit set

}

void LoadTTYBuffer (void)
{
    rchar = toupper ( receiveChar() );
    if (!(flags & MONITOR_TTY) ) return;

    if (rchar == 0xD) {
        RcvBuff[ibuf] = 0x0;
        flags = flags | EXECUTE_TTY;
        flags = flags & ~MONITOR_TTY;

    } else if (rchar == 0xA) {
        memset((void *)RcvBuff, 0, sizeof(RcvBuff));
        ibuf = 0;

    } else if (rchar < 127 && rchar > 31) {
        RcvBuff[ibuf++] = rchar;
        if (ibuf >= RBUFF) {
            memset((void *)RcvBuff, 0, RBUFF);
            ibuf = 0;
            sendString ("RDY> ");
        } else {
            sendChar( rchar);
        }

    } else {
        sendString ("ERR: 0x");
        sendHex (rchar);
        sendString ("\r\n");
    }
}


void sendChar(char value)
{
    while ( !U1STAbits.TRMT );
    while ( U1STAbits.UTXBF );
    U1TXREG = (unsigned char) value;
    
}

char receiveChar(void)
{

//        while (!IFS0bits.U1RXIF );
    return (char) U1RXREG;

}

void sendString( char  *text  )
{
    unsigned char k;
    k = 0;
    while ( text[k] != 0 )
        sendChar( (char) text[k++]);
}

void sendDecNumber( unsigned long n )
{
    char string[sizeof(n)+1];
    memset ((char *)string, 0, sizeof(string));

    ultoa ((char *)string, n, 10);
    sendString (string);

}

void sendHex(unsigned int value)
{
    char string[sizeof(value)+1];
    memset ((char *)string, 0, sizeof(string));

    utoa (string, value, 16);
    sendString (string);
}


unsigned char parse ( char *str )
{
	static unsigned char i, j, k;
    
	memset ((char *)numbers, 0, sizeof(numbers));
	memset ((char *)commands, 0, sizeof(commands));

	for (i=0, j=0, k=0; i<strlen(str); i++) {
		if ( isalpha( str[i] ) ) {
            if (j < MAX_COMMAND_ENTRIES) {
                commands[j++] = str[i];
            }	
		} else if ( isdigit( str[i] ) ) {
	        if (k < MAX_COMMAND_ENTRIES) {
                numbers[k++] = strtol ( (char *)&str[i], NULL, 10 );
            }	
			while ( isdigit( str[i+1] ) ) {
				if (i >= strlen(str)) break;
				i++;
			}
		} else if ( isgraph( str[i] ) ) {
            if (j < MAX_COMMAND_ENTRIES) {
                commands[j++] = str[i];
            }	
		}
	}
    return j;
}




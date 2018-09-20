/*************************************************************************
Title:    Interrupt USART library with receive/transmit circular buffers
Author:   Peter Fleury <pfleury@gmx.ch>   http://jump.to/fleury
File:     $Id: uart.c,v 1.5.2.7 2005/07/03 11:29:18 Peter Exp $
Software: AVR-GCC 3.3 
Hardware: any AVR with built-in U[S]ART,
          tested on AT90S8515 at 4 Mhz and ATmega162, 88 and 1280 @16 & 20Mhz

DESCRIPTION:
    An interrupt is generated when the UART has finished transmitting or
    receiving a byte. The interrupt handling routines use circular buffers
    for buffering received and transmitted data.
    
    The RX_BUFFER_SIZE and TX_BUFFER_SIZE defines determine the buffer size
    in bytes. Note that these constants must be a power of 2.

    The UseUARTn defines (where n is the UART number) must be set for the
    relevant Xon/Xoff flow control code to be compiled. Each USART can be 
    defined to use Xon/Xoff flow control individually by defining 
    "UART_FLOWCTRL U_XONXOFF" (without any UART number) to apply Xon/Xoff
    flow control to all USARTs used.
    
USAGE:
    Refer to the header file uart.h for a description of the routines. 
    See also example test_uart.c.

NOTES:
    Based on Atmel Application Note AVR306
                    
*************************************************************************/

#ifndef UART_OPTS_H
#define UART_OPTS_H

#include "StdUART.def"         // Standardized USART register definitions.

#define UseUART0               // RS485 to/from the PC
//#define Uart0TxLedPort PORTE   // Used for Tx/RX control
//#define Uart0TxLedBit  PE4
//#define Uart0RxLedPort PORTE   // Just for Rx LED
//#define Uart0RxLedBit  PE5

#define UART0_RX_FLOWCTRL UFC_NONE
#define UART0_TX_FLOWCTRL UFC_NONE

/*------------------------- Universal Options --------------------------*/

// Unrem this to set the default flow control for all UARTS
// Can be overridden by the individual "UARTn_FLOWCTRL" defines.

//#define UART_RX_FLOWCTRL UFC_NONE UHS_XONXOFF UHS_RTSCTS
//#define UART_TX_FLOWCTRL UFC_NONE UHS_XONXOFF UHS_RTSCTS
 
/* Size of the circular receive buffer, must be power of 2 */
/* Both the transmit and receive buffers will be this size */

#ifndef UART_RX_BUFFER_SIZE
  #define UART_RX_BUFFER_SIZE 64
#endif

// Size of the circular transmit buffers. Must be power of 2

#ifndef UART_TX_BUFFER_SIZE
  #define UART_TX_BUFFER_SIZE 64
#endif

// Receive buffer flow control levels (Xon/Xoff or hardware)

#define UART_XOFF_LEVEL ((UART_RX_BUFFER_SIZE * 3) / 4)
#define UART_XON_LEVEL  (UART_RX_BUFFER_SIZE / 4)

// Application callback when the USART "putc" function is waiting for a
// slot in the transmit buffer to become available. Can be a null macro
// but if not then it must be a function that doesn't take "too long"
// to return (to the "putc" that called it). You must ensure that the 
// called code can never call the same putc function it was called from.

#define APP_UART_PUTC_WAIT {}

// Application callback when the UART "gets" function is waiting for a
// the next byte to be received. Can be a null macro, but if not then a
// similar no-recursion rule applies here as with APP_UART_PUTC_WAIT.

#define APP_UART_GETC_WAIT {}

/*--------------------------- UART1 Options ---------------------------*/

#define UseUART1                      // Unrem to use RS232.h library to control hardware UART1

#define UART1_RX_FLOWCTRL UFC_NONE
#define UART1_TX_FLOWCTRL UFC_NONE

// Define the i/o port and two of its pins to drive the (normally red) 
// "transmitting" indicator LED and the (normally green) "receiving" LED
// Rem out the first define to remove the LED indicator code for UART1.

  #define Uart1TxLedPort  PORTD       // Rem this out to remove Rx/Tx LED code on Uart1
  #define Uart1TxLedBit   PD4      // TxD LED and Transmit/Receive control line
  #define Uart1RxLedPort  PORTD       // Rem this out to remove Rx/Tx LED code on Uart1
  #define Uart1RxLedBit   PD6      // Default RxD LED (next pin from TxD on MexgaX8)

// Define the i/o port and two of its pins for RS485 direction control  (same pin as Tx LED)
 
	#define UART1_TX_RX_PORT  Uart1TxLedPort
	#define UART1_TX_RX_PIN   Uart1TxLedBit
	
	#define UseUART2
	
#define UART2_RX_FLOWCTRL UFC_NONE
#define UART2_TX_FLOWCTRL UFC_NONE

/*  #define Uart2TxLedPort  PORTD       // Rem this out to remove Rx/Tx LED code on Uart2
  #define Uart2TxLedBit   PD3      // TxD LED and Transmit/Receive control line
  #define Uart2RxLedPort  PORTD       // Rem this out to remove Rx/Tx LED code on Uart2
  #define Uart2RxLedBit   PD2      // Default RxD LED (next pin from TxD on MexgaX8)*/

  #define Uart2TxLedPort  PORTE       // Rem this out to remove Rx/Tx LED code on Uart2
  #define Uart2TxLedBit   PE4      // TxD LED and Transmit/Receive control line
  #define Uart2RxLedPort  PORTE       // Rem this out to remove Rx/Tx LED code on Uart2
  #define Uart2RxLedBit   PE5      // Default RxD LED (next pin from TxD on MexgaX8)

	#define UART2_TX_RX_PORT  Uart2TxLedPort
	#define UART2_TX_RX_PIN   Uart2TxLedBit

	#define DEBUG_PORT PORTG
	#define DEBUG_PIN PG5

#endif


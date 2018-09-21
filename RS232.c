/*************************************************************************
Title:    Interrupt UART library with receive/transmit circular buffers
Author:   Peter Fleury <pfleury@gmx.ch>   http://jump.to/fleury
File:     $Id: uart.c,v 1.5.2.7 2005/07/03 11:29:18 Peter Exp $
Software: AVR-GCC 3.3 
Hardware: any AVR with built-in UART, 
          tested on AT90S8515 at 4 Mhz and ATmega162, 88 and 1280 @16 & 20Mhz

DESCRIPTION:
    An interrupt is generated when the UART has finished transmitting or
    receiving a byte. The interrupt handling routines use circular buffers
    for buffering received and transmitted data.
    
    The RX_BUFFER_SIZE and TX_BUFFER_SIZE defines determine the buffer size
    in bytes. Note that these constants must be a power of 2.

    The UseUARTn defines (where n is the UART number) must be set for the
    relevent code to be compiled.
    
    Each USART's flow control can be defined individually by defining
    "UART_FLOWCTRLn" to "UFC_NONE" or "UFC_XONXOFF", where "n" is the USART
    number (0..3). Define "UART_FLOWCTRL" (without any UART number) to
    apply that flow control to all USARTs used.

    Each USART can also optionally be assigned two general purpose i/o pins
    for driving two LED indicators. One (normally green) LED used to indicate 
    receive activity and one (normally red) LED to indicate transmit activity.
    When defined, these pins are set up and controlled automatically by this 
    library. This library uses positive logic, so the cathode of each LED 
    should be connected to the 0V power line and the anode connected via a
    suitable resistor to the appropriate pin. The "Transmit LED" pin can 
    also be used to enable an external RS485 type driver since this pin will
    only be high while the USART is actually transmitting (auto-turnaround).
    
    The USARTn_RXC_vect ISR is used to control the receive LED. It 
    unconditionally switches on the LED on entry and checks the state of the
    RxD pin just before exiting. It only switches the LED off if RxD is
    high. Thus the receive LED will stay on when a contiguous, unbroken
    stream of frames is being received since the (low) start bit of the next 
    frame will be on the bus at this time.

    The USARTn_TXC_vect ISR is used to switch off an external transmitter
    when the USART is being used for half-duplex communications and/or
    the transmit LED and is only defined and enabled if indicator LED pins
    are actually assigned and/or the UARTn_HALF_DUPLEX conditional is defined.
    Define its value as 1 if the pin needs to be driven high to enable the
    transmitter or 0 if the control pin needs to be driven low to enable
    the RS485 transmitter driver.
    
USAGE:
    Refer to the header file uart.h for a description of the routines. 
    See also example test_uart.c.

NOTES:
    Based on Atmel Application Note AVR306
                    
*************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "StandardTypes.h"
#include "Std_IO.h"
#include "Std_IO_Macros.h"
#include "AsciiCtrl.h"
#include "StdUART.def"                   // Standardised USART register, interrupt and bit names 

#define   STD_UART_INLINE
#include "StdUART.h"                     // Inlined hardware independent functions for USART interfacing
#include "RS232_Opts.h"                  // Application-specific RS232/USART options
#include "RS232.h"                       // Interrupt driven, ring-buffered RS232 style I/O 

// Define UART_FLOWCTRL to set the default handshaking for all UARTS.

#if defined(UART_FLOWCTRL)
  #define UART0_RX_FLOWCTRL UART_FLOWCTRL
  #define UART1_RX_FLOWCTRL UART_FLOWCTRL
  #define UART2_RX_FLOWCTRL UART_FLOWCTRL
  #define UART3_RX_FLOWCTRL UART_FLOWCTRL

  #define UART0_TX_FLOWCTRL UART_FLOWCTRL
  #define UART1_TX_FLOWCTRL UART_FLOWCTRL
  #define UART2_TX_FLOWCTRL UART_FLOWCTRL
  #define UART3_TX_FLOWCTRL UART_FLOWCTRL
#endif

/*-------------------------------- Global Constants and Macros --------------------------------*/

/* size of RX/TX buffers */

#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1)
#define UART_TX_BUFFER_MASK (UART_TX_BUFFER_SIZE - 1)

#if(UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK)
  #error RX buffer size is not a power of 2
#endif

#if(UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK)
  #error TX buffer size is not a power of 2
#endif

#if !(defined(UseUART0) || defined(UseUART1) || defined(UseUART2) || defined(UseUART3))
  #error No UART is being used - '#define one or more of 'UseUART0', 'UseUART1', 'UseUART2', 'UseUART3''
#endif

#if defined(__AVR_ATmega161__)
  #error "AVR ATmega161 currently not supported by this libaray !"
#elif UARTS < 1
  #error "no UART for MCU available"
#endif

/***********************************************************************************************/
/*                                        *** UART0 ***                                        */
/***********************************************************************************************/

#ifdef UseUART0

/*----------------------------- Structure to hold the state of UART0 --------------------------*/

#if !defined(UART0_RX_FLOWCTRL) || (UART0_RX_FLOWCTRL > UFC_RTSCTS)
  #error "No flow control defined for UART0 receiver - #define UART0_RX_FLOWCTRL = UFC_NONE, UFC_XONXOFF or UFC_RTSCTS"
#endif
#if !defined(UART0_TX_FLOWCTRL) || (UART0_TX_FLOWCTRL > UFC_RTSCTS)
  #error "No flow control defined for UART0 transmitter - #define UART0_TX_FLOWCTRL = UFC_NONE, UFC_XONXOFF or UFC_RTSCTS"
#endif
#define _RX_FLOWCTRL UART0_RX_FLOWCTRL            // Define flow control options
#define _TX_FLOWCTRL UART0_TX_FLOWCTRL            // Define flow control options

/*----------------------------- Structure to hold the state of UART0 --------------------------*/

static struct{
  volatile byte    TxBuf[UART_TX_BUFFER_SIZE];
  volatile byte    RxBuf[UART_RX_BUFFER_SIZE];
  volatile byte    TxHead;
  volatile byte    TxTail;
  volatile byte    RxHead;
  volatile byte    RxTail;
  volatile byte    LastRxError;
#if(_RX_FLOWCTRL != UFC_NONE)
  volatile byte    RxCount;
  #if(_RX_FLOWCTRL == UFC_XONXOFF)
  volatile byte    SendX;
  #endif
#endif
#if(_TX_FLOWCTRL == UFC_XONXOFF)
  volatile boolean CanTx;
#endif
} Uart0;

#define _Uart Uart0                               // The USART "object"

// Undefine the Generic Definitions

#undef U_Port
#undef U_RxDbit
#undef U_TxDbit
#undef U_XckPort
#undef U_XckBit

// USART0 I/O Ports and Pins

#define U_Port             Uart0Port              // The port that the USART0 RxD,TxD pins are on
#define U_RxDbit           Uart0RxDbit            // The port bit that the USART0 RxD pin is on
#define U_TxDbit           Uart0TxDbit            // The port pin that the USART0 TxD pin is on
#define U_XckPort          Uart0XckPort           // The port register that the USART0 XCK pin is on
#define U_XckBit           Uart0XckBit            // The USART0 XCK bit number in the XCK port register

#define U_ModemPort        UART0ModemPort         // Optional USART0 Modem control port (RTS and CTS)
#define U_bitRTS           UART0bitRTS
#define U_bitCTS           UART0bitCTS

#if defined(Uart0TxLedPort)
	#define U_TxLedPort      Uart0TxLedPort         // The transmit indicator LED port
	#define U_TxLedBit       Uart0TxLedBit          // The pin used to drive the (red) transmit LED
#endif
#if defined(Uart0TxLedPort)
	#define U_RxLedPort      Uart0RxLedPort         // The receive indicator LED port
	#define U_RxLedBit       Uart0RxLedBit          // The pin used to drive the (green) receive LED
#endif

/*--------- Use the private generic macros to flesh out the USART0 inline functions -----------*/

#define U_SetBaudrate      Uart0SetBaudrate       // Set the Baud rate registers and the U2X bit
#define U_SetCtrl          Uart0SetCtrl           // Enable or disable USART0 and/or its interrupts
#define U_EnableInts       Uart0EnableInts        // Enable or disable USART0 interrupts
#define U_TxCompleteInt    Uart0TxCompleteInt     // Enable or disable TxC ISR
#define U_RxCompleteInt    Uart0RxCompleteInt     // Enable or disable RxC ISR
#define U_TxBufferEmptyInt Uart0TxBufferEmptyInt  // Enable or disable UDRE ISR
#define U_GetTxBufferEmptyInt Uart0GetTxBufferEmptyInt // True if Transmit Complete Interrupt is enabled
#define U_Write            Uart0Write             // Busy write to Tx Buffer reg
#define U_Read             Uart0Read              // Busy read from Rx buffer reg
#define U_RxIsComplete     Uart0RxIsComplete      // boTrue if Receive is complete
#define U_GetRxStatus      Uart0GetRxStatus       // Get the Receive status flags
#define U_ClearIntFlags    Uart0ClearIntFlags     // Clear all interrupt enable flags
#define U_TxBufferIsEmpty  Uart0TxBufferIsEmpty   // boTrue if can send next byte
#define U_TxBufferIsFull   Uart0TxBufferIsFull    // boTrue if cannot send next byte
#define U_TxIsComplete     Uart0TxIsComplete      // boTrue if all bytes sent  
#define U_ClearTxComplete  Uart0ClearTxComplete   // Clear the "Tx Complete" flag

#define U_GetPinRxD        Uart0GetPinRxD         // Get state of the RxD pin
#define U_GetPinTxD        Uart0GetPinTxD         // Get state of the TxD pin
#define U_GetPinXCK        Uart0GetPinXCK         // Get state of the XCX pin

#define U_SetFormat        Uart0SetFormat         // Set the frame format and operating mode
#define U_Enable           Uart0Enable            // Enable/Disable Transmitter and/or Receiver
#define U_FlushRxBuffer    Uart0FlushRxBuffer     // Flush the receive buffer
#define U_IsReceiving()    (!Uart0GetPinRxD())    // Returns True if RxD pin is low (start bit detection)

/*---------- Assign the private generic interrupt vector macros to USART1 functions -----------*/

#define U_RXC_vect         USART0_RXC_vect
#define U_TXC_vect         USART0_TXC_vect
#define U_UDRE_vect        USART0_UDRE_vect

/*------------ Assign the private generic function name macros to USART1 functions ------------*/

#define U_INIT             uart0_init
#define U_GETC             uart0_getc
#define U_GETS             uart0_gets
#define U_PUTC             uart0_putc
#define U_PUTS             uart0_puts
#define U_PUTS_P           uart0_puts_p
#define U_PUTBYTES         uart0_putbytes
#define U_STUFF_RX         uart0_stuff_rx
#define U_TX_BUF_IS_EMPTY  uart0_tx_buffer_is_empty

/*----------------------------------- The actual code for USART0 ------------------------------*/

#include "RS232.inc"

#endif // UseUART0

/* These functions are only for ATmegas with two or more USARTs */

/***********************************************************************************************/
/*                                        *** UART1 ***                                        */
/***********************************************************************************************/

#ifdef UseUART1

#ifndef UDR1
  #error "This MCU does not have a USART1"
#endif

#if !defined(UART1_RX_FLOWCTRL) || (UART1_RX_FLOWCTRL > UFC_RTSCTS)
  #error "No flow control defined for UART1 receiver - #define UART1_RX_FLOWCTRL = UFC_NONE, UFC_XONXOFF or UFC_RTSCTS"
#endif
#if !defined(UART2_TX_FLOWCTRL) || (UART2_TX_FLOWCTRL > UFC_RTSCTS)
  #error "No (valid) flow control defined for UART2 transmitter - #define UART2_TX_FLOWCTRL = UFC_NONE, UFC_XONXOFF or UFC_RTSCTS"
#endif

#define _RX_FLOWCTRL UART1_RX_FLOWCTRL            // Define flow control options
#define _TX_FLOWCTRL UART1_TX_FLOWCTRL            // Define flow control options

/*----------------------------- Structure to hold the state of UART1 --------------------------*/

static struct{
  volatile byte    TxBuf[UART_TX_BUFFER_SIZE];
  volatile byte    RxBuf[UART_RX_BUFFER_SIZE];
  volatile byte    TxHead;
  volatile byte    TxTail;
  volatile byte    RxHead;
  volatile byte    RxTail;
  volatile byte    LastRxError;
#if(_RX_FLOWCTRL == UFC_XONXOFF)
  volatile byte    RxCount;
  volatile byte    SendX;
#endif
#if(_TX_FLOWCTRL == UFC_XONXOFF)
  volatile boolean CanTx;
#endif
} Uart1;

#define _Uart      Uart1                          // The UART "object"

/*---------------------- Define the internal macros for the USART1 hardware -------------------*/

// Undefine the Generic Definitions

#undef U_Port
#undef U_RxDbit
#undef U_TxDbit
#undef U_XckPort
#undef U_XckBit

#undef U_TxLedPort
#undef U_RxLedPort
#undef U_TxLedBit
#undef U_RxLedBit

// USART1 I/O Ports and Pins

#define U_Port             Uart1Port              // The port that the USART1 RxD pin is on
#define U_RxDbit           Uart1RxDbit            // The port bit that the USART1 RxD pin is on
#define U_TxDbit           Uart1TxDbit            // The port pin that the USART1 TxD pin is on
#define U_XckPort          Uart1XckPort           // The port register that the USART1 XCK pin is on
#define U_XckBit           Uart1XckBit            // The USART1 XCK bit number in the XCK port register

#if defined(Uart1TxLedPort)

#define U_TxLedPort        Uart1TxLedPort         // The transmit indicator LED port
#define U_RxLedPort        Uart1RxLedPort         // The receive indicator LED port
#define U_RxLedBit         Uart1RxLedBit          // The pin used to drive the (green) receive LED
#define U_TxLedBit         Uart1TxLedBit          // The pin used to drive the (red) transmit LED

#endif

/*--------- Use the private generic macros to flesh out the USART1 inline functions -----------*/

#define U_SetBaudrate      Uart1SetBaudrate       // Set the Baud rate registers and the U2X bit
#define U_SetCtrl          Uart1SetCtrl           // Enable or disable USART1 and/or its interrupts
#define U_EnableInts       Uart1EnableInts        // Enable or disable USART1 interrupts
#define U_TxCompleteInt    Uart1TxCompleteInt     // Enable or disable TxC ISR
#define U_RxCompleteInt    Uart1RxCompleteInt     // Enable or disable RxC ISR
#define U_TxBufferEmptyInt Uart1TxBufferEmptyInt  // Enable or disable ISR
#define U_GetTxBufferEmptyInt Uart1GetTxBufferEmptyInt // True if Transmit Complete Interrupt is enabled
#define U_Write            Uart1Write             // Busy write to Tx Buffer reg
#define U_Read             Uart1Read              // Busy read from Rx buffer reg
#define U_RxIsComplete     Uart1RxIsComplete      // boTrue if Receive is complete
#define U_GetRxStatus      Uart1GetRxStatus       // Get the Receive status flags
#define U_ClearIntFlags    Uart1ClearIntFlags     // Clear all interrupt enable flags
#define U_TxBufferIsEmpty  Uart1TxBufferIsEmpty   // boTrue if can send next byte
#define U_TxBufferIsFull   Uart1TxBufferIsFull    // boTrue if cannot send next byte
#define U_TxIsComplete     Uart1TxIsComplete      // boTrue if all bytes sent  
#define U_ClearTxComplete  Uart1ClearTxComplete   // Clear the "Tx Complete" flag

#define U_GetPinRxD        Uart1GetPinRxD         // Get state of the RxD pin
#define U_GetPinTxD        Uart1GetPinTxD         // Get state of the TxD pin
#define U_GetPinXCK        Uart1GetPinXCK         // Get state of the XCX pin

#define U_SetFormat        Uart1SetFormat         // Set the frame format and operating mode
#define U_Enable           Uart1Enable            // Enable/Disable Transmitter and/or Receiver
#define U_FlushRxBuffer    Uart1FlushRxBuffer     // Flush the receive buffer
#define U_IsReceiving()    (!Uart1GetPinRxD())    // Used to check for the next Start bit

/*---------- Assign the private generic interrupt vector macros to USART1 functions -----------*/

#define U_RXC_vect         USART1_RXC_vect
#define U_TXC_vect         USART1_TXC_vect
#define U_UDRE_vect        USART1_UDRE_vect

/*------------ Assign the private generic function name macros to USART1 functions ------------*/

#define U_INIT             uart1_init
#define U_GETC             uart1_getc
#define U_GETS             uart1_gets
#define U_PUTC             uart1_putc
#define U_PUTS             uart1_puts
#define U_PUTS_P           uart1_puts_p
#define U_PUTBYTES         uart1_putbytes
#define U_STUFF_RX         uart1_stuff_rx
#define U_TX_BUF_IS_EMPTY  uart1_tx_buffer_is_empty

/*----------------------------------- The actual code for USART1 ------------------------------*/

#include "RS232.inc"

#endif // UseUart1

/***********************************************************************************************/
/*                                        *** UART2 ***                                        */
/***********************************************************************************************/

#ifdef UseUART2

#ifndef UDR2
#error "This MCU does not have a USART2"
#endif

#if !defined(UART2_RX_FLOWCTRL) || (UART2_RX_FLOWCTRL > UFC_RTSCTS)
#error "No (valid) flow control defined for UART2 receiver - #define UART2_RX_FLOWCTRL = UFC_NONE, UFC_XONXOFF or UFC_RTSCTS"
#endif
#if !defined(UART2_TX_FLOWCTRL) || (UART2_TX_FLOWCTRL > UFC_RTSCTS)
#error "No (valid) flow control defined for UART2 transmitter - #define UART2_TX_FLOWCTRL = UFC_NONE, UFC_XONXOFF or UFC_RTSCTS"
#endif
#define _RX_FLOWCTRL UART2_RX_FLOWCTRL            // Define flow control options
#define _TX_FLOWCTRL UART2_TX_FLOWCTRL            // Define flow control options

/*----------------------------- Structure to hold the state of UART2 --------------------------*/

static struct{
	volatile byte    TxBuf[UART_TX_BUFFER_SIZE];
	volatile byte    RxBuf[UART_RX_BUFFER_SIZE];
	volatile byte    TxHead;
	volatile byte    TxTail;
	volatile byte    RxHead;
	volatile byte    RxTail;
	volatile byte    LastRxError;
	#if(_RX_FLOWCTRL == UFC_XONXOFF)
	volatile byte    RxCount;
	volatile byte    SendX;
	#endif
	#if(_TX_FLOWCTRL == UFC_XONXOFF)
	volatile boolean CanTx;
	#endif
} Uart2;

#define _Uart      Uart2                          // The UART "object"

/*--------------------- Define the internal macros for the USART2 hardware --------------------*/

// Undefine the Generic Definitions

#undef U_Port
#undef U_RxDbit
#undef U_TxDbit
#undef U_XckPort
#undef U_XckBit

#undef U_TxLedPort
#undef U_RxLedPort
#undef U_TxLedBit
#undef U_RxLedBit

// USART2 I/O Ports and Pins

#define U_Port             Uart2Port              // The port that the USART2 RxD pin is on
#define U_RxDbit           Uart2RxDbit            // The port bit that the USART2 RxD pin is on
#define U_TxDbit           Uart2TxDbit            // The port pin that the USART2 TxD pin is on
#define U_XckPort          Uart2XckPort           // The port register that the USART2 XCK pin is on
#define U_XckBit           Uart2XckBit            // The USART2 XCK bit number in the XCK port register

#if defined(Uart2TxLedPort)

#define U_TxLedPort        Uart2TxLedPort         // The transmit indicator LED port
#define U_RxLedPort        Uart2RxLedPort         // The receive indicator LED port
#define U_RxLedBit         Uart2RxLedBit          // The pin used to drive the (green) receive LED
#define U_TxLedBit         Uart2TxLedBit          // The pin used to drive the (red) transmit LED

#endif

/*--------- Use the private generic macros to flesh out the USART2 inline functions -----------*/

#define U_SetBaudrate      Uart2SetBaudrate       // Set the Baud rate registers and the U2X bit
#define U_SetCtrl          Uart2SetCtrl           // Enable or disable USART2 and/or its interrupts
#define U_EnableInts       Uart2EnableInts        // Enable or disable USART2 interrupts
#define U_TxCompleteInt    Uart2TxCompleteInt     // Enable or disable TxC ISR
#define U_RxCompleteInt    Uart2RxCompleteInt     // Enable or disable RxC ISR
#define U_TxBufferEmptyInt Uart2TxBufferEmptyInt  // Enable or disable ISR
#define U_GetTxBufferEmptyInt Uart2GetTxBufferEmptyInt // True if Transmit Complete Interrupt is enabled
//#define U_TxcIntIsEnabled  Uart2TxcIntIsEnabled   // True if Transmit Complete Interrupt is enabled
#define U_Write            Uart2Write             // Busy write to Tx Buffer reg
#define U_Read             Uart2Read              // Busy read from Rx buffer reg
#define U_RxIsComplete     Uart2RxIsComplete      // boTrue if Receive is complete
#define U_GetRxStatus      Uart2GetRxStatus       // Get the Receive status flags
#define U_ClearIntFlags    Uart2ClearIntFlags     // Clear all interrupt enable flags
#define U_TxBufferIsEmpty  Uart2TxBufferIsEmpty   // boTrue if can send next byte
#define U_TxBufferIsFull   Uart2TxBufferIsFull    // boTrue if cannot send next byte
#define U_TxIsComplete     Uart2TxIsComplete      // boTrue if all bytes sent
#define U_ClearTxComplete  Uart2ClearTxComplete   // Clear the "Tx Complete" flag

#define U_GetPinRxD        Uart2GetPinRxD         // Get state of the RxD pin
#define U_GetPinTxD        Uart2GetPinTxD         // Get state of the TxD pin
#define U_GetPinXCK        Uart2GetPinXCK         // Get state of the XCX pin

#define U_SetFormat        Uart2SetFormat         // Set the frame format and operating mode
#define U_Enable           Uart2Enable            // Enable/Disable Transmitter and/or Receiver
#define U_FlushRxBuffer    Uart2FlushRxBuffer     // Flush the receive buffer
#define U_IsReceiving()    (!Uart2GetPinRxD())    // Used to check for the next Start bit

/*---------- Assign the private generic interrupt vector macros to USART1 functions -----------*/

#define U_RXC_vect         USART2_RXC_vect
#define U_TXC_vect         USART2_TXC_vect
#define U_UDRE_vect        USART2_UDRE_vect

/*------------ Assign the private generic function name macros to USART1 functions ------------*/

#define U_INIT             uart2_init
#define U_GETC             uart2_getc
#define U_GETS             uart2_gets
#define U_PUTC             uart2_putc
#define U_PUTS             uart2_puts
#define U_PUTS_P           uart2_puts_p
#define U_PUTBYTES         uart2_putbytes
#define U_STUFF_RX         uart2_stuff_rx
#define U_TX_BUF_IS_EMPTY  uart2_tx_buffer_is_empty

/*----------------------------------- The actual code for USART2 ------------------------------*/

#include "RS232.inc"

#endif // UseUart2

/***********************************************************************************************/
/*                                        *** UART3 ***                                        */
/***********************************************************************************************/

#ifdef UseUart3

#ifndef UDR3
  #error This MCU does not have a USART3
#endif

#if !defined(UART3_RX_FLOWCTRL) || (UART3_RX_FLOWCTRL > UFC_RTSCTS)
  #error "No flow control defined for UART3 receiver - #define UART3_RX_FLOWCTRL = UFC_NONE, UFC_XONXOFF or UFC_RTSCTS"
#endif
#if !defined(UART3_TX_FLOWCTRL) || (UART3_TX_FLOWCTRL > UFC_RTSCTS)
  #error "No flow control defined for UART3 transmitter - #define UART3_TX_FLOWCTRL = UFC_NONE, UFC_XONXOFF or UFC_RTSCTS"
#endif
#define _RX_FLOWCTRL UART3_RX_FLOWCTRL            // Define flow control options
#define _TX_FLOWCTRL UART3_TX_FLOWCTRL            // Define flow control options

/*----------------------------- Structure to hold the state of UART3 --------------------------*/

static struct{
  volatile byte    TxBuf[UART_TX_BUFFER_SIZE];
  volatile byte    RxBuf[UART_RX_BUFFER_SIZE];
  volatile byte    TxHead;
  volatile byte    TxTail;
  volatile byte    RxHead;
  volatile byte    RxTail;
  volatile byte    LastRxError;
#if(_RX_FLOWCTRL == UFC_XONXOFF)
  volatile byte    RxCount;
  volatile byte    SendX;
#endif
#if(_TX_FLOWCTRL == UFC_XONXOFF)
  volatile boolean CanTx;
#endif
} Uart3

#define _Uart      Uart3                          // The UART "object"

/*--------------------- Define the internal macros for the USART3 hardware --------------------*/

// Undefine the Generic Definitions

#undef U_Port
#undef U_RxDbit
#undef U_TxDbit
#undef U_XckPort
#undef U_XckBit

#undef U_TxLedPort
#undef U_RxLedPort
#undef U_TxLedBit
#undef U_RxLedBit

// USART3 I/O Ports and Pins

#define U_Port             Uart3Port              // The port that the USART3 RxD pin is on
#define U_RxDbit           Uart3RxDbit            // The port bit that the USART3 RxD pin is on
#define U_TxDbit           Uart3TxDbit            // The port pin that the USART3 TxD pin is on
#define U_XckPort          Uart3XckPort           // The port register that the USART3 XCK pin is on
#define U_XckBit           Uart3XckBit            // The USART3 XCK bit number in the XCK port register

#if defined(Uart3TxLedPort)

#define U_TxLedPort        Uart3TxLedPort         // The transmit indicator LED port
#define U_RxLedPort        Uart3RxLedPort         // The receive indicator LED port
#define U_RxLedBit         Uart3RxLedBit          // The pin used to drive the (green) receive LED
#define U_TxLedBit         Uart3TxLedBit          // The pin used to drive the (red) transmit LED

#endif

/*--------- Use the private generic macros to flesh out the USART3 inline functions -----------*/

#define U_SetBaudrate      Uart3SetBaudrate       // Set the Baud rate registers and the U2X bit
#define U_SetCtrl          Uart3SetCtrl           // Enable or disable USART3 and/or its interrupts
#define U_EnableInts       Uart3EnableInts        // Enable or disable USART3 interrupts
#define U_TxCompleteInt    Uart3TxCompleteInt     // Enable or disable TxC ISR
#define U_RxCompleteInt    Uart3RxCompleteInt     // Enable or disable RxC ISR
#define U_TxBufferEmptyInt Uart3TxBufferEmptyInt  // Enable or disable ISR
#define U_GetTxBufferEmptyInt Uart3GetTxBufferEmptyInt // True if Transmit Complete Interrupt is enabled
#define U_Write            Uart3Write             // Busy write to Tx Buffer reg
#define U_Read             Uart3Read              // Busy read from Rx buffer reg
#define U_RxIsComplete     Uart3RxIsComplete      // boTrue if Receive is complete
#define U_GetRxStatus      Uart3GetRxStatus       // Get the Receive status flags
#define U_ClearIntFlags    Uart3ClearIntFlags     // Clear all interrupt enable flags
#define U_TxBufferIsEmpty  Uart3TxBufferIsEmpty   // boTrue if can send next byte
#define U_TxBufferIsFull   Uart3TxBufferIsFull    // boTrue if cannot send next byte
#define U_TxIsComplete     Uart3TxIsComplete      // boTrue if all bytes sent  
#define U_ClearTxComplete  Uart3ClearTxComplete   // Clear the "Tx Complete" flag

#define U_GetPinRxD        Uart3GetPinRxD         // Get state of the RxD pin
#define U_GetPinTxD        Uart3GetPinTxD         // Get state of the TxD pin
#define U_GetPinXCK        Uart3GetPinXCK         // Get state of the XCX pin

#define U_SetFormat        Uart3SetFormat         // Set the frame format and operating mode
#define U_Enable           Uart3Enable            // Enable/Disable Transmitter and/or Receiver
#define U_FlushRxBuffer    Uart3FlushRxBuffer     // Flush the receive buffer
#define U_IsReceiving()    (!Uart3GetPinRxD())    // Used to check for the next Start bit

/*---------- Assign the private generic interrupt vector macros to USART3 functions -----------*/

#define U_RXC_vect         USART3_RXC_vect
#define U_TXC_vect         USART3_TXC_vect
#define U_UDRE_vect        USART3_UDRE_vect

/*------------ Assign the private generic function name macros to USART3 functions ------------*/

#define U_INIT             uart3_init
#define U_GETC             uart3_getc
#define U_GETS             uart3_gets
#define U_PUTC             uart3_putc
#define U_PUTS             uart3_puts
#define U_PUTS_P           uart3_puts_p
#define U_PUTBYTES         uart3_putbytes
#define U_STUFF_RX         uart3_stuff_rx
#define U_TX_BUF_IS_EMPTY  uart3_tx_buffer_is_empty

/*----------------------------------- The actual code for USART3 ------------------------------*/

#include "RS232.inc"

#endif // UseUart3

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/

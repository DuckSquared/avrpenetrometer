#ifndef RS232_H
  #define RS323_H

#include "StandardTypes.h"     // Header file containing standard type defs

/************************************************************************
Title:    Interrupt UART library with receive/transmit circular buffers
Author:   Peter Fleury <pfleury@gmx.ch>   http://jump.to/fleury
File:     $Id: uart.h,v 1.7.2.3 2005/03/06 16:10:54 Peter Exp $
Software: AVR-GCC 3.3
Hardware: any AVR with built-in UART, tested on AT90S8515 at 4 Mhz
Usage:    see Doxygen manual
************************************************************************/

/** 
 *  @defgroup pfleury_uart UART Library
 *  @code #include <uart.h> @endcode
 * 
 *  @brief Interrupt UART library using the built-in UART with transmit and receive circular buffers. 
 *
 *  This library can be used to transmit and receive data through the built-in U[S]ART. 
 *
 *  An interrupt is generated when the UART has finished transmitting or
 *  receiving a byte. The interrupt handling routines use circular buffers
 *  for buffering received and transmitted data.
 *
 *  The UART_RX_BUFFER_SIZE and UART_TX_BUFFER_SIZE constants define
 *  the size of the circular buffers in bytes. Note that these constants must be a power of 2.
 *  You may need to adapt this constants to your target and your application by adding 
 *  CDEFS += -DUART_RX_BUFFER_SIZE=nn -DUART_RX_BUFFER_SIZE=nn to your Makefile.
 *
 *  @note Based on Atmel Application Note AVR306
 *  @author Peter Fleury pfleury@gmx.ch  http://jump.to/fleury
 */
 
/**@{*/

#if (__GNUC__ * 100 + __GNUC_MINOR__) < 304
  #error "This library requires AVR-GCC 3.4 or later, update to newer AVR-GCC compiler !"
#endif

#ifndef F_CPU
#  error "RS323.h requires F_CPU to be defined"
#endif

/*** constants and macros */

#define UFC_NONE    0          // Flow control options enumerations
#define UFC_XONXOFF 1
#define UFC_RTSCTS  2
#define UFC_RS485   3          // Half-duplex with a Send/!Receive control line

/** @brief  UART Baudrate Expression
 *  @param  F_CPU  system clock in Hz           
 *  @param  Baudrate baudrate in bps, e.g. 1200, 2400, 9600     
 */

#define UART_BAUD_SELECT(Baudrate) ((F_CPU / (Baudrate * 16l)) - 1)

/* ** high byte error return code of PcUartGetC() */

#define uartParityError    (UPE << 8 )          /* Parity error by UART        */
#define uartOverrunError   (DOR << 8 )          /* Overrun condition by UART   */
#define uartFrameError     (FE  << 8 )          /* Framing Error by UART       */
#define uartBufferOverflow (FE  << 9 )          /* Receive ringbuffer overflow */
#define uartStringOverflow (FE  << 10)          /* gets() string overerflow    */
#define uartNoData         (FE  << 11)          /* No receive data available   */

/*----------------------------- Function Prototypes --------------------------*/

/**
   @brief   Initialize UART and set operating mode and frame format 
   @param   baudrate Specify baudrate using macro UART_BAUD_SELECT()
   @return  none
*/
extern void uart0_init(void);

/**
 *  @brief   Get received byte from ringbuffer
 *
 * Returns in the lower byte the received character and in the 
 * higher byte the last receive error.
 * UART_NO_DATA is returned when no data is available.
 *
 *  @param   void
 *  @return  lower byte:  received byte from ringbuffer
 *  @return  higher byte: last receive status
 *           - \b 0 successfully received data from UART
 *           - \b UART_NO_DATA           
 *             <br>no receive data available
 *           - \b UART_BUFFER_OVERFLOW   
 *             <br>Receive ringbuffer overflow.
 *             We are not reading the receive buffer fast enough, 
 *             one or more received character have been dropped 
 *           - \b UART_OVERRUN_ERROR     
 *             <br>Overrun condition by UART.
 *             A character already present in the UART UDR register was 
 *             not read by the interrupt handler before the next character arrived,
 *             one or more received characters have been dropped.
 *           - \b UART_FRAME_ERROR       
 *             <br>Framing Error by UART
 */

extern word uart0_getc(void);

/** @brief  Get asLF terminated string from USART1 ringbuffer.
 *
 * Reads at most (size - 1) bytes from the UART, until a newline
 * character was encountered, and stores the characters in the buffer
 * pointed to by str. Unless an error was encountered while reading,
 * the string will then be terminated with a Null character.
 *
 * @param  str : Pointer to character array with at least (Max - 1) characters
 * @param  size: Size of the buffer pointed to by S.
 * @return Error status (UART error bits) or zero if received without 
 */

extern byte uart0_gets(char *str, int size);

/**
 *  @brief   Put byte to ringbuffer for transmitting by the UART
 *  @param   data byte to be transmitted
 *  @return  none
 */

extern void uart0_putc(char data);

/**
 *  @brief   Put string to ringbuffer for transmitting via UART
 *
 *  The string is buffered by the uart library in a circular buffer
 *  and one character at a time is transmitted by the UART using interrupts.
 *  Blocks if it can not write the whole string into the circular buffer.
 * 
 *  @param   s string to be transmitted
 *  @return  none
 */

extern void uart0_puts(const char *s );

/**
 * @brief    Put string from program memory to ringbuffer for transmitting via UART.
 *
 * The string is buffered by the uart library in a circular buffer
 * and one character at a time is transmitted by the UART using interrupts.
 * Blocks if it can not write the whole string into the circular buffer.
 *
 * @param    s program memory string to be transmitted
 * @return   none
 * @see      uart_puts_P
 */

extern void uart0_puts_p(const char *s);

/**
 * @brief    Macro to automatically transmmit a string constant in program memory
 */

#define uart0_puts_P(__s) PcUartPutsP(PSTR(__s))

/**
 * @brief    Put array of bytes from program memory to ringbuffer for transmitting via UART.
 *
 * The bytes are buffered by the uart library in a circular buffer
 * and one character at a time is transmitted by the UART using interrupts.
 * Blocks if it can not write the whole string into the circular buffer.
 *
 * @param    data program memory to be transmitted
 * @return   none
 * @see      uart_puts
 */

extern void uart0_putbytes(const char *data, byte count);

/**
 * @brief    Retruns true if all UART0 transmit buffers are empty (nothing pending transmition)
 *
 * The bytes are buffered by the uart library in a circular buffer
*/

extern boolean uart0_tx_buffer_is_empty(void);

/**
 * @brief   Writes an array of bytes to the receive ringbuffer. Used for debuging.
 *
 * The bytes fron *s are stuffed into the receive buffer until either the buffer
 * becomes full or all n bytes have been stuffed. Returns the number of bytes it
 * could not stuff and s points to the next byte to stuff.
 *
 * @prams   s  Pointer to array of bytes to be transmitted
 * @param   n  Number of bytes to suff into Rx buffer from *s. 
 *           have been stuffed or the buffer becomes full
 * return  The number of bytes still to stuff (0 if all n bytes stuffed)
 */

extern byte uart0_stuff_rx(char *s, byte n);

/*----------------------------------- UART1 --------------------------------------*/

/** @brief  Initialize USART1 (only available on selected ATmegas) @see PcUartInit */

extern void uart1_init(void);

/** @brief  Get received byte of USART1 from ringbuffer. (only available on selected ATmega) @see PcUartGetC */

extern word uart1_getc(void);

/** @brief  Get asLF terminated string from USART1 from ringbuffer. (only available on selected ATmega) @see PcUartGetC */

extern byte uart1_gets(char *str, int size);

/** @brief  Put byte to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see PcUartPutC */

extern void uart1_putc(char data);

/** @brief  Put string to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see uart_puts */

extern void uart1_puts(const char *s);

/** @brief  Put string from program memory to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see PcUartPutsP */

extern void uart1_puts_p(const char *s);

/** @brief  Macro to automatically write out a string constant from program memory */

#define uart1_puts_P(__s) uart1_puts_p(PSTR(__s))

/** @brief  Put program memory to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see uart_puts */

extern void uart1_putbytes(const char *data, byte count);

/** @brief  Retruns true if all UART1 transmit buffers are empty (nothing pending transmition) */

extern boolean uart1_tx_buffer_is_empty(void);

/** @brief  Writes an array of bytes to the receive ringbuffer. Used for debugging. */

extern byte uart1_stuff_rx(char *s, byte n);

/*----------------------------------- UART2 --------------------------------------*/

/** @brief  Initialize USART2 (only available on selected ATmegas) @see PcUartInit */

extern void uart2_init(void);

/** @brief  Get received byte of USART2 from ringbuffer. (only available on selected ATmega) @see PcUartGetC */

extern word uart2_getc(void);

/** @brief  Get asLF terminated string from USART2 from ringbuffer. (only available on selected ATmega) @see PcUartGetC */

extern byte uart2_gets(char *str, int size);

/** @brief  Put byte to ringbuffer for transmitting via USART2 (only available on selected ATmega) @see PcUartPutC */

extern void uart2_putc(char data);

/** @brief  Put string to ringbuffer for transmitting via USART2 (only available on selected ATmega) @see uart_puts */

extern void uart2_puts(const char *s);

/** @brief  Put string from program memory to ringbuffer for transmitting via USART2 (only available on selected ATmega) @see PcUartPutsP */

extern void uart2_puts_p(const char *s );

/** @brief  Put program memory to ringbuffer for transmitting via USART2 (only available on selected ATmega) @see uart_puts */

extern void uart2_putbytes(const char *data, byte count);

/** @brief  Retruns true if all UART1 transmit buffers are empty (nothing pending transmition) */

extern boolean uart2_tx_buffer_is_empty(void);

/** @brief  Writes an array of bytes to the receive ringbuffer. Used for debugging. */

extern byte uart2_stuff_rx(char *s, byte n);

/** @brief  Macro to automatically put a string constant into program memory */

#define uart2_puts_P(__s) uart2_puts_p(PSTR(__s))

/*----------------------------------- UART3 --------------------------------------*/

/** @brief  Initialize USART3 (only available on selected ATmegas) @see PcUartInit */

extern void uart3_init(void);

/** @brief  Get received byte of USART3 from ringbuffer. (only available on selected ATmega) @see PcUartGetC */

extern word uart3_getc(void);

/** @brief  Put byte to ringbuffer for transmitting via USART3 (only available on selected ATmega) @see PcUartPutC */

extern void uart3_putc(char data);

/** @brief  Put string to ringbuffer for transmitting via USART3 (only available on selected ATmega) @see uart_puts */

extern void uart3_puts(const char *s);

/** @brief  Put string from program memory to ringbuffer for transmitting via USART3 (only available on selected ATmega) @see PcUartPutsP */

extern void uart3_puts_p(const char *s );

/** @brief  Macro to automatically put a string constant into program memory */

#define uart3_puts_P(__s) uart3_puts_p(PSTR(__s))

/** @brief  Put program memory to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see uart_puts */

extern void uart3_putbytes(const char *data, byte count);

/** @brief  Retruns true if all UART3 transmit buffers are empty (nothing pending transmition) */

extern boolean uart3_tx_buffer_is_empty(void);

/** @brief  Writes an array of bytes to the receive ringbuffer. Used for debugging. */

extern byte uart3_stuff_rx(char *s, byte n);

/**@}*/

#endif // RS323_H

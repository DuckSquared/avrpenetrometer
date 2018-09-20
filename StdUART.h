/************************************************************************
* Title:    USART Hardware Abstraction Layer (HAL)                      *
*                                                                       *
* Purpose:  A Hardware Abstraction Layer for AVR UARTs and USARTS.      *
*           Provides inlined functions for basic set up and control of  *
*           all the hardware U[S]ART's of any ATmega or AT90 with a UART*
*           These functions are as code-efficient as accessing the USART*
*           registers directly with C or assembler when constants are   *
*           passed as function parameters, but this file will save you a*
*           lot of grief when you code needs to run on many various MCUs*
* Author:   Jason Burgon  <jaybur@ntlworld/com>                         *
* Software: AVR-GCC 4.18                                                *
* Hardware: Any AVR with built-in U[S]ART, tested on ATmega88,162,1280  *
* Usage:    #include StdUART.h in your application to reference the     *
*           inline functions defined here                               *
*                                                                       *
*************************************************************************/

#ifndef _StdUART_H_
#define _StdUART_H_ 1

#include <avr/io.h>
#include "StandardTypes.h"
#include "StdUART.def"

#if defined(UARTS) && (UARTS > 0)

#include "StandardTypes.h"
#include "Std_IO_Macros.h"

#define UFC_NONE    0                                                 // Flow control options defines
#define UFC_XONXOFF 1

/*------------------- Enumerated types used in API Functions calls -------------------*/

#ifdef UPM01
  typedef enum{umoAsync, umoSyncUp, umoSyncDown, umoSPI} TUartMode;   // USART operating modes (Asynchronous,
#else                                                                 // Synchronous, Reserved and Master SPI
  typedef enum{umoAsync, umoSyncUp, umoSyncDown} TUartMode;           // No Master SPI mode for older AVR's
#endif
typedef enum{udb5, udb6, udb7, udb8, udb9}          TUartDataBits;    // Number of data bits in a serial frame
typedef enum{upaNone, upaReserved, upaEven, upaOdd} TUartParity;      // Parity bit in a serial frame
typedef enum{ust1, ust2}                            TUartStopBits;    // Number of stop bits in a serial frame
typedef enum{uenRx, uenTx, uenRxTx}                 TUartEnable;      // Parts of the USART to enable/disable

/*------------------------- Internal Generic USART Macros ----------------------------*/

// Sets or clears 1 or more USART control register B bits (mostly device and interrupt enable bits).

#define _UartEnableInts(IntEnBits, EnableIt){if(EnableIt) U_CSRB |= IntEnBits;  else U_CSRB &= ~(IntEnBits);}

// Sets or clears 1 or more USART control register C bits (mostly frame format and operating mode). This 
// macro takes care of setting the USARTS Register Select bit (URSEL) for those USART variants that use 
// the URSEL bit for selecting between the UBRRHn and the UCSRCn registers.

#define _UartSetCtrl(CtrlBits, EnableIt)   {if(EnableIt) U_CSRB |= CtrlBits;   else U_CSRB &= ~(CtrlBits); }

// Enable or disable Transmit Data Register Empty interrupt

#define _UartTxBufferEmptyInt(EnableIt)    {if(EnableIt) U_CSRB |= _BV(UDRIE); else U_CSRB &= ~_BV(UDRIE); }

// Enable or disable Transmit Complete interrupt

#define _UartTxCompleteInt(EnableIt)       {if(EnableIt) U_CSRB |= _BV(TXCIE); else U_CSRB &= ~_BV(TXCIE); }

// Enable or disable Receive Complete interrupt

#define _UartRxCompleteInt(EnableIt)       {if(EnableIt) U_CSRB |= _BV(RXCIE); else U_CSRB &= ~_BV(RXCIE); }

// Sets or clears the "baudrate x 2"  control bit

#define _UartSetBaudX2(SetIt)              {if(SetIt)    U_CSRA |= _BV(U2X);   else U_CSRA &= ~_BV(U2X);   }

// Sets the baudrate register to the value given by "baudrate" to the values given. The given baudrate must
// be less than 32,768 for some AVR's, less than 4096 for others and less than 256 for some very old ones.

#if defined(U_BRR)
  #define _UartSetBaudrate(baudrate)       {U_BRR  = baudrate;                                             }
#elif defined(U_BRRH)
  #define _UartSetBaudrate(baudrate)       {U_BRRL = (baudrate & 0xFF); U_BRRH = (baudrate >> 8);          }
#else
  #define _UartSetBaudrate(baudrate)       {U_BRRL = baudrate;                                             }
#endif

// Unconditionally write data byte to the transmit buffer register

#define _UartWrite(Data) (U_DATA = Data)

// Unconditionally read data byte to the receive buffer register

#define _UartRead() U_DATA

// Non-zero if the "Receive Complete" flag is set

#define _UartRxIsComplete() (U_CSRA & _BV(RXC))

// Read the UCSRAn Status and Control register

#define _UartGetRxStatus() U_CSRA

// Non-zero if the next byte to transmit can be loaded into the USARTn transmit buffer

#define _UartTxBufferIsEmpty() (U_CSRA & _BV(UDRE))

// Non-zero if the next byte to transmit cannot be loaded into the USARTn transmit buffer

#define _UartTxBufferIsFull() !_UartTxBufferIsEmpty()

// Non-zero if the "Transmit Complete" flag is set (USARTn is not transmitting anything)

#define _UartTxComplete() (U_CSRA & _BV(TXC))

// Non-zero if the "Transmit Complete" flag is clear (USARTn is transmitting)

#define _UartIsTransmitting() !_UartTxComplete()

// Clears the "Transmit Complete" flag

#define _UartClearTxComplete() (U_CSRA |= _BV(TXC))

// Return the current state of the RxDn pin.

#define _UartGetPinRxD() GetPin(U_Port, U_RxDbit)

// Return the current state of the TxDn pin.

#define _UartGetPinTxD() GetPin(U_Port, U_TxDbit)

// Return the current state of the XCKn pin.

#define _UartGetPinXCK() GetPin(U_XckPort, U_XckBit)

// Non-zero if the RxD pin is low. Doesn't ensure we are NOT receiving!

#define _UartIsReceiving() (_UartGetPinRxD() == 0)

// Enable or disable the USARTn receiver

#define _UartEnableRx(Enable) if(Enable) U_CSRB |= _BV(RXEN); else U_CSRB &= ~_BV(RXEN)

// Enable or disable the USARTn transmitter

#define _UartEnableTx(Enable) if(Enable) U_CSRB |= _BV(TXEN); else U_CSRB &= ~_BV(TXEN)

// Enable or disable the USARTn transmitter, receiver or both.

#define _UartEnableRxTx(Enable)         \
  if(Enable)                            \
    U_CSRB |=  (_BV(RXEN) | _BV(TXEN)); \
  else                                  \
    U_CSRB &= ~(_BV(RXEN) | _BV(TXEN))

/*--------------------------------------- USART0 -------------------------------------*/

#if defined (UDR0) && (!defined _STD_UART_0)
  #define _STD_UART_0

// Undefine the Generic Definitions

#undef U_DATA
#undef U_BRR
#undef U_BRRH
#undef U_BRRL
#undef U_CSRA 
#undef U_CSRB
#undef U_CSRC
#undef U_RXCIE
#undef U_TXCIE
#undef U_UDRIE

#undef U_Port
#undef U_RxDbit
#undef U_TxDbit
#undef U_XckPort
#undef U_XckBit

// Map the USART0 Register and pin names onto the private generic names

#if defined(UBRR0H)
  #define U_BRRH  UBRR0H
#endif
//#if defined(UBRR0)
  #define U_BRR   UBRR0
//#endif
#define U_DATA  UDR0
#define U_BRRL  UBRR0L
#define U_CSRA  UCSR0A
#define U_CSRB  UCSR0B
#define U_CSRC  UCSR0C
#define U_RXCIE RXCI0E
#define U_TXCIE TXCI0E
#define U_UDRIE UDRI0E

// USART0 I/O Ports and Pins

#define U_Port    Uart0Port           // The port that the USART0 RxD pin is on
#define U_RxDbit  Uart0bitRxD         // The port bit that the USART0 RxD pin is on
#define U_TxDbit  Uart0bitTxD         // The port pin that the USART0 TxD pin is on

#define U_XckPort Uart0PortXCK        // The port register that the USART0 XCK pin is on
#define U_XckBit  Uart0bitXCK         // The USART0 XCK bit number in the XCK port register

// Use the private generic macros to flesh out the USART0 inline functions

TLoHi inline Uart0GetPinRxD(void)                           // Get state of the RxD pin
{
  return(_UartGetPinRxD());        
}

TLoHi inline Uart0GetPinTxD(void)                          // Get state of the TxD pin
{
  return(_UartGetPinTxD());
}

TLoHi inline Uart0GetPinXCK(void)                          // Get state of the XCX pin
{
  return(_UartGetPinXCK());
}

void inline Uart0SetBaudX2(boolean SetIt)                  // Set the "x2" baudrate bit
{
  _UartSetBaudX2(SetIt)
}

void inline Uart0SetCtrl(byte CtrlReg, TEnable Enable)     // Enable or disable USART0 and/or its interrupts
{
  _UartSetCtrl(CtrlReg, Enable);
}

void inline Uart0EnableInts(byte IntEnBits, TEnable Enable)// Enable or disable USART0 interrupts
{
  _UartEnableInts(IntEnBits, Enable);
}

void inline Uart0TxCompleteInt(TEnable EnableIt)           // Enable or disable ISR
{
  _UartTxCompleteInt(EnableIt);
}

void inline Uart0RxCompleteInt(TEnable EnableIt)           // Enable or disable ISR
{
  _UartRxCompleteInt(EnableIt);
}

void inline Uart0TxBufferEmptyInt(TEnable EnableIt)        // Enable or disable ISR
{
  _UartTxBufferEmptyInt(EnableIt); 
}

boolean inline Uart0GetTxBufferEmptyInt(void)             // True if Transmit Buffer Empty Interrupt is enabled
{
  return(U_CSRB & _BV(UDRIE));
}

boolean inline Uart0GetTxCompleteInt(void)                // True if Transmit Complete Interrupt is enabled
{
  return(U_CSRB & _BV(TXCIE));
}

boolean inline Uart0GetRxCompleteInt(void)                // True if Receive Complete Interrupt is enabled
{
  return(U_CSRB & _BV(RXCIE));
}

void inline Uart0Write(byte Data)                         // Write to Transmit Buffer register
{
  _UartWrite(Data);
}

byte inline Uart0Read(void)                               // Read from Receive buffer register
{
  return(_UartRead());
}

boolean inline Uart0RxIsComplete(void)                    // boTrue if Receive byte pending
{
  return(_UartRxIsComplete());
}

boolean inline Uart0IsReceiving(void)                     // boTrue if RxD pin is low
{
  return(_UartIsReceiving());
}

byte inline Uart0GetRxStatus(void)                        // Get the Receive status flags
{
  return(_UartGetRxStatus());
}

boolean inline Uart0TxBufferIsEmpty(void)                 // boTrue if can send next byte
{
  return(_UartTxBufferIsEmpty());
}

boolean inline Uart0TxBufferIsFull(void)                  // boTrue if cannot send next byte
{
  return(_UartTxBufferIsFull());
}

boolean inline Uart0TxIsComplete(void)                    // boTrue if all bytes sent
{
  return(_UartTxComplete());
}

boolean inline Uart0IsTransmitting(void)                  // boTrue if not all bytes sent
{
  return(!_UartTxComplete());
}

void inline Uart0ClearTxComplete(void)                    // Clear the "Tx Complete" flag
{
  U_CSRA |= _BV(TXC);
}

void inline Uart0SetBaudrate(word baudrate, boolean x2)
{
//_UartSetBaudrate(baudrate); !! Doesn't always work!
#if defined(UBRR0)
  UBRR0 = baudrate;
#elif defined(UBRR0H)
  UBRR0L = (baudrate & 0xFF);
  UBRR0H = (baudrate >> 8);
#else
  UBRR0L = baudrate; 
#endif
  _UartSetBaudX2(x2); 
}

void inline Uart0SetFormat(TUartMode   OpMode, TUartDataBits DataBits, TUartParity Parity, 
                           TUartStopBits StopBits)
{
#if defined(AT90_UART) || defined(ATMEGA_UART)

  #if(DataBits < udb8) || ((DataBits == udb9) && (Parity != upaNone)) || (StopBits != ust1) || (OpMode != umoAsync)
    #error UART does not support this frame format
  #endif

  // Set the number of data bits 9 bits is also set for (must be software implemented) for odd or even parity
  // where bit9 is used as the parity bit

  if((DataBits == udb9) || (Parity != upaNone))
    U_CSRB = _BV(CHR9);
  else 
    U_CSRB = 0;

#else

  byte TempB;
  byte TempC;

  // Set the number of data bits

  if(DataBits == udb9)
  {
    TempC = _BV(UCSZ0) | _BV(UCSZ1);
    TempB = _BV(UCSZ2);
  } 
  else 
  {
    TempC = (DataBits - udb5) << UCSZ0;
    TempB = 0;
  }

  // Set the operating mode (Async, SyncUp, SyncDown), parity and stop bits 

  TempC |= (((OpMode == umoSPI) << UMSEL1) | ((OpMode > umoAsync) << UMSEL0) | 
            (Parity << UPM0) | (StopBits << USBS) | ((OpMode == umoSyncDown) << UCPOL));

#ifdef URSEL 
  U_CSRC = TempC | _BV(URSEL);
#else
  U_CSRC = TempC;
#endif
  U_CSRB = (U_CSRB & ~_BV(UCSZ2)) | TempB;                 // Clear UCSZ2 for all bar udb9
#endif
}

void inline Uart0Enable(TUartEnable RxTx, TEnable Enable)
{
  switch(RxTx)
  {
    case uenRx:
      _UartEnableRx(Enable);
      break;
    case uenTx:
      _UartEnableTx(Enable);
      break;
    case uenRxTx:
      _UartEnableRxTx(Enable);
  }
}

byte inline Uart0FlushRxBuffer(void)
{
  volatile byte Dummy;

  while (U_CSRA & _BV(RXC))
    Dummy = U_DATA;
  return(Dummy);
}

word inline Uart0GetByte(void)
{
  while(!_UartRxIsComplete())                             // Busy wait until data received in USART Rx buffer
    ;
  return((word)(U_CSRA & (_BV(FE) | _BV(DOR) | _BV(UPE))) << 8) | U_DATA; // High byte = Status Flags, Low byte = data
}

void inline Uart0PutByte(byte Data)
{
  while(_UartTxBufferIsFull())                            // Busy wait until room in USART Tx buffer
    ;
  U_DATA = Data;
}
#endif

//-------------------------------- USART1 ---------------------------------

#if defined(UDR1) && (!defined _STD_UART_1)
  #define _STD_UART_1

// Undefine the Generic Definitions

#undef U_DATA
#undef U_BRR
#undef U_BRRH
#undef U_BRRL
#undef U_CSRA 
#undef U_CSRB
#undef U_CSRC
#undef U_RXCIE
#undef U_TXCIE
#undef U_UDRIE

#undef U_Port
#undef U_RxDbit
#undef U_TxDbit
#undef U_XckPort
#undef U_XckBit

// Map the USART1 register and pin names onto the private generic names

//#ifdef UBRR1
  #define U_BRR   UBRR1
//#endif
#define U_DATA    UDR1
#define U_BRRH    UBRR1H
#define U_BRRL    UBRR1L
#define U_CSRA    UCSR1A
#define U_CSRB    UCSR1B
#define U_CSRC    UCSR1C
#define U_RXCIE   RXCI1E
#define U_TXCIE   TXCI1E
#define U_UDRIE   UDRI1E

// USART1 I/O Ports and Pins

#define U_Port    Uart1Port           // The port that the USART1 RxD pin is on
#define U_RxDbit  Uart1bitRxD         // The port bit that the USART1 RxD pin is on
#define U_TxDbit  Uart1bitTxD         // The port pin that the USART1 TxD pin is on

#define U_XckPort Uart1PortXCK        // The port register that the USART0 XCK pin is on
#define U_XckBit  Uart1bitXCK         // The USART1 XCK bit number in the XCK port register

// Use the private generic macros to flesh out the USART1 inline functions

TLoHi inline Uart1GetPinRxD(void)                           // Get state of the RxD pin
{
  return(_UartGetPinRxD());        
}

TLoHi inline Uart1GetPinTxD(void)                          // Get state of the TxD pin
{
  return(_UartGetPinTxD());
}

TLoHi inline Uart1GetPinXCK(void)                          // Get state of the XCX pin
{
  return(_UartGetPinXCK());
}

void inline Uart1SetBaudX2(boolean SetIt)                  // Set the "x2" baudrate bit
{
  _UartSetBaudX2(SetIt)
}

void inline Uart1SetCtrl(byte CtrlReg, TEnable Enable)     // Enable or disable USART1 and/or its interrupts
{
  _UartSetCtrl(CtrlReg, Enable);
}

void inline Uart1EnableInts(byte IntEnBits, TEnable Enable)// Enable or disable USART1 interrupts
{
  _UartEnableInts(IntEnBits, Enable);
}

void inline Uart1TxCompleteInt(TEnable EnableIt)           // Enable or disable ISR
{
  _UartTxCompleteInt(EnableIt);
}

void inline Uart1RxCompleteInt(TEnable EnableIt)           // Enable or disable ISR
{
  _UartRxCompleteInt(EnableIt);
}

void inline Uart1TxBufferEmptyInt(TEnable EnableIt)        // Enable or disable ISR
{
  _UartTxBufferEmptyInt(EnableIt); 
}

boolean inline Uart1GetTxBufferEmptyInt(void)              // True if Transmit Buffer Empty Interrupt is enabled
{
  return(U_CSRB & _BV(UDRIE));
}

boolean inline Uart1GetTxCompleteInt(void)                 // True if Transmit Complete Interrupt is enabled
{
  return(U_CSRB & _BV(TXCIE));
}

boolean inline Uart1GetRxCompleteInt(void)                 // True if Receive Complete Interrupt is enabled
{
  return(U_CSRB & _BV(RXCIE));
}

void inline Uart1Write(byte Data)                          // Write to Transmit Buffer register
{
  _UartWrite(Data);
}

byte inline Uart1Read(void)                                // Read from Receive buffer register
{
  return(_UartRead());
}

boolean inline Uart1RxIsComplete(void)                     // boTrue if Receive byte pending
{
  return(_UartRxIsComplete());
}

boolean inline Uart1IsReceiving(void)                      // boTrue if RxD pin is low
{
  return(_UartIsReceiving());
}

byte inline Uart1GetRxStatus(void)                         // Get the Receive status flags
{
  return(_UartGetRxStatus());
}

boolean inline Uart1TxBufferIsEmpty(void)                  // boTrue if can send next byte
{
  return(_UartTxBufferIsEmpty());
}

boolean inline Uart1TxBufferIsFull(void)                   // boTrue if cannot send next byte
{
  return(_UartTxBufferIsFull());
}

boolean inline Uart1TxIsComplete(void)                     // boTrue if all bytes sent
{
  return(_UartTxComplete());
}

boolean inline Uart1IsTransmitting(void)                   // boTrue if not all bytes sent
{
  return(!_UartTxComplete());
}

void inline Uart1ClearTxComplete(void)                     // Clear the "Tx Complete" flag
{
  U_CSRA |= _BV(TXC);
}

void inline Uart1SetBaudrate(word baudrate, boolean x2)
{
//_UartSetBaudrate(baudrate); !! Doesn't always work!
#if defined(UBRR1)
  UBRR1 = baudrate;
#elif defined(UBRR1H)
  UBRR1L = (baudrate & 0xFF);
  UBRR1H = (baudrate >> 8);
#else
  UBRR1L = baudrate; 
#endif
  _UartSetBaudX2(x2); 
}

void inline Uart1SetFormat(TUartMode   OpMode, TUartDataBits DataBits, TUartParity Parity, 
                           TUartStopBits StopBits)
{
#if defined(AT90_UART) || defined(ATMEGA_UART)

  #if(DataBits < udb8) || ((DataBits == udb9) && (Parity != upaNone)) || (StopBits != ust1) || (OpMode != umoAsync)
    #error UART does not support this frame format
  #endif

  // Set the number of data bits. 9 bits is also set for odd or even parity
  // where bit9 is used as the parity bit (must be software implemented).

  if((DataBits == udb9) || (Parity != upaNone))
    U_CSRB = _BV(CHR9);
  else
    U_CSRB = 0;

#else

  byte TempB;
  byte TempC;

  // Set the number of data bits

  if(DataBits == udb9)
  {
    TempC = _BV(UCSZ0) | _BV(UCSZ1);
    TempB = _BV(UCSZ2);
  } 
  else 
  {
    TempC = (DataBits - udb5) << UCSZ0;
    TempB = 0;
  }

  // Set the operating mode (SyncUp, SyncDown or Async), parity and stop bits 

  TempC |= (((OpMode > umoAsync) << UMSEL0) | (Parity << UPM0) | (StopBits << USBS) | ((OpMode == umoSyncDown) << UCPOL));

#ifdef URSEL 
  U_CSRC = TempC | _BV(URSEL);
#else
  U_CSRC = TempC;
#endif
  U_CSRB = (U_CSRB & ~_BV(UCSZ2)) | TempB;                 // Clear UCSZ2 for all bar udb9
#endif
}

void inline Uart1Enable(TUartEnable RxTx, TEnable Enable)
{
  switch(RxTx)
  {
    case uenRx:
      _UartEnableRx(Enable);
      break;
    case uenTx:
      _UartEnableTx(Enable);
      break;
    case uenRxTx:
      _UartEnableRxTx(Enable);
  }
}

byte inline Uart1FlushRxBuffer(void)
{
  volatile byte Dummy;

  while (U_CSRA & _BV(RXC))
    Dummy = U_DATA;
  return(Dummy);                                           // Prevents a compiler warning
}

word inline Uart1GetByte(void)
{
  while(!_UartRxIsComplete())                              // Busy wait until data received in USART Rx buffer
    ;
  return((word)(U_CSRA & (_BV(FE) | _BV(DOR) | _BV(UPE))) << 8) | U_DATA; // High byte = Status Flags, Low byte = data
}

void inline Uart1PutByte(byte Data)
{
  while(_UartTxBufferIsFull())                             // Busy wait until room in USART Tx buffer
    ;
  U_DATA = Data;
}

#endif // _STD_UART_1

//---------------------------------- USART2 -------------------------------

#if defined(UDR2) && (!defined _STD_UART_2)
  #define _STD_UART_2

// Undefine the Generic Definitions

#undef U_DATA
#undef U_BRR
#undef U_BRRH
#undef U_BRRL
#undef U_CSRA 
#undef U_CSRB
#undef U_CSRC
#undef U_RXCIE
#undef U_TXCIE
#undef U_UDRIE

#undef U_Port
#undef U_RxDbit
#undef U_TxDbit
#undef U_XckPort
#undef U_XckBit

// Map the USART2 register and pin names onto the private generic names

#ifdef UBRR2
  #define U_BRR   UBRR2
#endif
#define U_DATA    UDR2
#define U_BRRH    UBRR2H
#define U_BRRL    UBRR2L
#define U_CSRA    UCSR2A
#define U_CSRB    UCSR2B
#define U_CSRC    UCSR2C
#define U_RXCIE   RXCI2E
#define U_TXCIE   TXCI2E
#define U_UDRIE   UDRI2E

// USART2 I/O Ports and Pins

#define U_Port    Uart2Port           // The port that the USART2 RxD pin is on
#define U_RxDbit  Uart2bitRxD         // The port bit that the USART2 RxD pin is on
#define U_TxDbit  Uart2bitTxD         // The port pin that the USART2 TxD pin is on

#define U_XckPort Uart2PortXCK        // The port register that the USART0 XCK pin is on
#define U_XckBit  Uart2bitXCK         // The USART0 XCK bit number in the XCK port register

// Use the private generic macros to flesh out the USART2 inline functions

TLoHi inline Uart2GetPinRxD(void)                           // Get state of the RxD pin
{
  return(_UartGetPinRxD());        
}

TLoHi inline Uart2GetPinTxD(void)                          // Get state of the TxD pin
{
  return(_UartGetPinTxD());
}

TLoHi inline Uart2GetPinXCK(void)                          // Get state of the XCX pin
{
  return(_UartGetPinXCK());
}

void inline Uart2SetBaudX2(boolean SetIt)                  // Set the "x2" baudrate bit
{
  _UartSetBaudX2(SetIt)
}

void inline Uart2SetCtrl(byte CtrlReg, TEnable Enable)     // Enable or disable USART2 and/or its interrupts
{
  _UartSetCtrl(CtrlReg, Enable);
}

void inline Uart2EnableInts(byte IntEnBits, TEnable Enable)// Enable or disable USART2 interrupts
{
  _UartEnableInts(IntEnBits, Enable);
}

void inline Uart2TxCompleteInt(TEnable EnableIt)           // Enable or disable ISR
{
  _UartTxCompleteInt(EnableIt);
}

void inline Uart2RxCompleteInt(TEnable EnableIt)           // Enable or disable ISR
{
  _UartRxCompleteInt(EnableIt);
}

void inline Uart2TxBufferEmptyInt(TEnable EnableIt)        // Enable or disable ISR
{
  _UartTxBufferEmptyInt(EnableIt); 
}

boolean inline Uart2GetTxBufferEmptyInt(void)              // True if Transmit Buffer Empty Interrupt is enabled
{
  return(U_CSRB & _BV(UDRIE));
}

boolean inline Uart2GetTxCompleteInt(void)                 // True if Transmit Complete Interrupt is enabled
{
  return(U_CSRB & _BV(TXCIE));
}

boolean inline Uart2GetRxCompleteInt(void)                 // True if Receive Complete Interrupt is enabled
{
  return(U_CSRB & _BV(RXCIE));
}

void inline Uart2Write(byte Data)                          // Write to Transmit Buffer register
{
  _UartWrite(Data);
}

byte inline Uart2Read(void)                                // Read from Receive buffer register
{
  return(_UartRead());
}

boolean inline Uart2RxIsComplete(void)                     // boTrue if Receive byte pending
{
  return(_UartRxIsComplete());
}

boolean inline Uart2IsReceiving(void)                     // boTrue if RxD pin is low
{
  return(_UartIsReceiving());
}

byte inline Uart2GetRxStatus(void)                        // Get the Receive status flags
{
  return(_UartGetRxStatus());
}

boolean inline Uart2TxBufferIsEmpty(void)                 // boTrue if can send next byte
{
  return(_UartTxBufferIsEmpty());
}

boolean inline Uart2TxBufferIsFull(void)                  // boTrue if cannot send next byte
{
  return(_UartTxBufferIsFull());
}

boolean inline Uart2TxIsComplete(void)                    // boTrue if all bytes sent
{
  return(_UartTxComplete());
}

boolean inline Uart2IsTransmitting(void)                  // boTrue if not all bytes sent
{
  return(!_UartTxComplete());
}

void inline Uart2ClearTxComplete(void)                    // Clear the "Tx Complete" flag
{
  U_CSRA |= _BV(TXC);
}

void inline Uart2SetBaudrate(word baudrate, boolean x2)
{
//_UartSetBaudrate(baudrate); !! Doesn't always work!
#if defined(UBRR2)
  UBRR2 = baudrate;
#elif defined(UBRR2H)
  UBRR2L = (baudrate & 0xFF);
  UBRR2H = (baudrate >> 8);
#else
  UBRR2L = baudrate; 
#endif
  _UartSetBaudX2(x2); 
}

void inline Uart2SetFormat(TUartMode   OpMode, TUartDataBits DataBits, TUartParity Parity, 
                           TUartStopBits StopBits)
{
#if defined(AT90_UART) || defined(ATMEGA_UART)

  #if(DataBits < udb8) || ((DataBits == udb9) && (Parity != upaNone)) || (StopBits != ust1) || (OpMode != umoAsync)
    #error UART does not support this frame format
  #endif

  // Set the number of data bits 9 bits is also set for (must be software implemented) for odd or even parity
  // where bit9 is used as the parity bit

  if((DataBits == udb9) || (Parity != upaNone))
    U_CSRB = _BV(CHR9);
  else 
    U_CSRB = 0;

#else

  byte TempB;
  byte TempC;

  // Set the number of data bits

  if(DataBits == udb9)
  {
    TempC = _BV(UCSZ0) | _BV(UCSZ1);
    TempB = _BV(UCSZ2);
  } 
  else 
  {
    TempC = (DataBits - udb5) << UCSZ0;
    TempB = 0;
  }

  // Set the operating mode (SyncUp, SyncDown or Async), parity and stop bits 

  TempC |= (((OpMode > umoAsync) << UMSEL0) | (Parity << UPM0) | (StopBits << USBS) | ((OpMode == umoSyncDown) << UCPOL));

#ifdef URSEL 
  U_CSRC = TempC | _BV(URSEL);
#else
  U_CSRC = TempC;
#endif
  U_CSRB = (U_CSRB & ~_BV(UCSZ2)) | TempB;                 // Clear UCSZ2 for all bar udb9
#endif
}

void inline Uart2Enable(TUartEnable RxTx, TEnable Enable)
{
	switch(RxTx)
	{
		case uenRx:
		_UartEnableRx(Enable);
		break;
		case uenTx:
		_UartEnableTx(Enable);
		break;
		case uenRxTx:
		_UartEnableRxTx(Enable);
	}
}

byte inline Uart2FlushRxBuffer(void)
{
  volatile byte Dummy;

  while (U_CSRA & _BV(RXC))
  Dummy = U_DATA;
  return(Dummy);                                           // Prevents a compiler warning
}

word inline Uart2GetByte(void)
{
  while(!_UartRxIsComplete())                              // Busy wait until data received in USART Rx buffer
    ;
  return((word)(U_CSRA & (_BV(FE) | _BV(DOR) | _BV(UPE))) << 8) | U_DATA; // High byte = Status Flags, Low byte = data
}

void inline Uart2PutByte(byte Data)
{
  while(_UartTxBufferIsFull())                            // Busy wait until room in USART Tx buffer
    ;
  U_DATA = Data;
}

#endif // _STD_UART_2

//---------------------------------- USART3 --------------------------------

#if defined(UDR3) && (!defined _STD_UART_3)
  #define _STD_UART_3

// Undefine the Generic Definitions

#undef U_DATA
#undef U_BRR
#undef U_BRRH
#undef U_BRRL
#undef U_CSRA 
#undef U_CSRB
#undef U_CSRC
#undef U_RXCIE
#undef U_TXCIE
#undef U_UDRIE

#undef U_Port
#undef U_RxDbit
#undef U_TxDbit
#undef U_XckPort
#undef U_XckBit

// Map the USART3 register and pin names onto the private generic names

#ifdef UBRR3
  #define U_BRR   UBRR3
#endif
#define U_DATA    UDR3
#define U_BRRH    UBRR3H
#define U_BRRL    UBRR3L
#define U_CSRA    UCSR3A
#define U_CSRB    UCSR3B
#define U_CSRC    UCSR3C
#define U_RXCIE   RXCI3E
#define U_TXCIE   TXCI3E
#define U_UDRIE   UDRI3E

// USART3 I/O Ports and Pins

#define U_Port    Uart3Port           // The port that the USART3 RxD pin is on
#define U_RxDbit  Uart3bitRxD         // The port bit that the USART3 RxD pin is on
#define U_TxDbit  Uart3bitTxD         // The port pin that the USART3 TxD pin is on

#define U_XckPort Uart3PortXCK        // The port register that the USART0 XCK pin is on
#define U_XckBit  Uart3bitXCK         // The USART0 XCK bit number in the XCK port register

// Use the private generic macros to flesh out the USART3 inline functions

TLoHi inline Uart3GetPinRxD(void)                           // Get state of the RxD pin
{
  return(_UartGetPinRxD());        
}

TLoHi inline Uart3GetPinTxD(void)                          // Get state of the TxD pin
{
  return(_UartGetPinTxD());
}

TLoHi inline Uart3GetPinXCK(void)                          // Get state of the XCX pin
{
  return(_UartGetPinXCK());
}

void inline Uart3SetBaudX2(boolean SetIt)                  // Set the "x2" baudrate bit
{
  _UartSetBaudX2(SetIt)
}

void inline Uart3SetCtrl(byte CtrlReg, TEnable Enable)     // Enable or disable USART3 and/or its interrupts
{
  _UartSetCtrl(CtrlReg, Enable);
}

void inline Uart3EnableInts(byte IntEnBits, TEnable Enable)// Enable or disable USART3 interrupts
{
  _UartEnableInts(IntEnBits, Enable);
}

void inline Uart3TxCompleteInt(TEnable EnableIt)           // Enable or disable ISR
{
  _UartTxCompleteInt(EnableIt);
}

void inline Uart3RxCompleteInt(TEnable EnableIt)           // Enable or disable ISR
{
  _UartRxCompleteInt(EnableIt);
}

void inline Uart3TxBufferEmptyInt(TEnable EnableIt)        // Enable or disable ISR
{
  _UartTxBufferEmptyInt(EnableIt); 
}

boolean inline Uart3GetTxBufferEmptyInt(void)             // True if Transmit Buffer Empty Interrupt is enabled
{
  return(U_CSRB & _BV(UDRIE));
}

boolean inline Uart3GetTxCompleteInt(void)                // True if Transmit Complete Interrupt is enabled
{
  return(U_CSRB & _BV(TXCIE));
}

boolean inline Uart3GetRxCompleteInt(void)                // True if Receive Complete Interrupt is enabled
{
  return(U_CSRB & _BV(RXCIE));
}

void inline Uart3Write(byte Data)                         // Write to Transmit Buffer register
{
  _UartWrite(Data);
}

byte inline Uart3Read(void)                               // Read from Receive buffer register
{
  return(_UartRead());
}

boolean inline Uart3RxIsComplete(void)                    // boTrue if Receive byte pending
{
  return(_UartRxIsComplete());
}

boolean inline Uart3IsReceiving(void)                     // boTrue if RxD pin is low
{
  return(_UartIsReceiving());
}

byte inline Uart3GetRxStatus(void)                        // Get the Receive status flags
{
  return(_UartGetRxStatus());
}

boolean inline Uart3TxBufferIsEmpty(void)                 // boTrue if can send next byte
{
  return(_UartTxBufferIsEmpty());
}

boolean inline Uart3TxBufferIsFull(void)                  // boTrue if cannot send next byte
{
  return(_UartTxBufferIsFull());
}

boolean inline Uart3TxIsComplete(void)                    // boTrue if all bytes sent
{
  return(_UartTxComplete());
}

boolean inline Uart3IsTransmitting(void)                  // boTrue if not all bytes sent
{
  return(!_UartTxComplete());
}

void inline Uart3ClearTxComplete(void)                    // Clear the "Tx Complete" flag
{
  U_CSRA |= _BV(TXC);
}

void inline Uart3SetBaudrate(word baudrate, boolean x2)
{
//_UartSetBaudrate(baudrate); !! Doesn't always work!
#if defined(UBRR3)
  UBRR3 = baudrate;
#elif defined(UBRR3H)
  UBRR3L = (baudrate & 0xFF);
  UBRR3H = (baudrate >> 8);
#else
  UBRR3L = baudrate; 
#endif
  _UartSetBaudX2(x2); 
}

void inline Uart3SetFormat(TUartMode   OpMode, TUartDataBits DataBits, TUartParity Parity, 
                           TUartStopBits StopBits)
{
#if defined(AT90_UART) || defined(ATMEGA_UART)

  #if(DataBits < udb8) || ((DataBits == udb9) && (Parity != upaNone)) || (StopBits != ust1) || (OpMode != umoAsync)
    #error UART does not support this frame format
  #endif

  // Set the number of data bits 9 bits is also set for (must be software implemented) for odd or even parity
  // where bit9 is used as the parity bit

  if((DataBits == udb9) || (Parity != upaNone))
    U_CSRB = _BV(CHR9);
  else 
    U_CSRB = 0;

#else

  byte TempB;
  byte TempC;

  // Set the number of data bits

  if(DataBits == udb9)
  {
    TempC = _BV(UCSZ0) | _BV(UCSZ1);
    TempB = _BV(UCSZ2);
  } 
  else 
  {
    TempC = (DataBits - udb5) << UCSZ0;
    TempB = 0;
  }

  // Set the operating mode (SyncUp, SyncDown or Async), parity and stop bits 

  TempC |= (((OpMode > umoAsync) << UMSEL0) | (Parity << UPM0) | (StopBits << USBS) | ((OpMode == umoSyncDown) << UCPOL));

#ifdef URSEL 
  U_CSRC = TempC | _BV(URSEL);
#else
  U_CSRC = TempC;
#endif
  U_CSRB = (U_CSRB & ~_BV(UCSZ2)) | TempB;                 // Clear UCSZ2 for all bar udb9
#endif
}

void inline Uart3Enable(TUartEnable RxTx, TEnable Enable)
{
  switch(RxTx)
  {
    case uenRx:
      _UartEnableRx(Enable);
      break;
    case uenTx:
      _UartEnableTx(Enable);
      break;
    case uenRxTx:
      _UartEnableRxTx(Enable);
  }
}

byte inline Uart3FlushRxBuffer(void)
{
  volatile byte Dummy;
  
  while (U_CSRA & _BV(RXC))
    Dummy = U_DATA;
  return(Dummy);                                         // Prevents a compiler warning
}

word inline Uart3GetByte(void)
{
  while(!_UartRxIsComplete())                             // Busy wait until data received in USART Rx buffer
    ;
  return((word)(U_CSRA & (_BV(FE) | _BV(DOR) | _BV(UPE))) << 8) | U_DATA; // High byte = Status Flags, Low byte = data
}

void inline Uart3PutByte(byte Data)
{
  while(_UartTxBufferIsFull())                            // Busy wait until room in USART Tx buffer
    ;
  U_DATA = Data;
}

#endif // _STD_UART_3

//--------------- Undefine all the private generic macros -----------------

#undef _UartTxCompleteInt
#undef _UartRxCompleteInt
#undef _UartTxBufferEmptyInt
#undef _UartSetCtrl
#undef _UartWrite
#undef _UartRead
#undef _UartRxIsComplete
#undef _UartGetRxStatus
#undef _UartClearIntFlags
#undef _UartTxBufferIsEmpty
#undef _UartTxBufferIsFull
#undef _UartTxComplete
#undef _UartSetBaudrate
#undef _UartEnableRx
#undef _UartEnableRxTx
#undef _UartEnableInts 
#undef _UartClearTxComplete
#undef _UartEnableRx
#undef _UartEnableTx
#undef _UartEnableRxTx

#undef _UartGetPinRxD
#undef _UartGetPinTxD
#undef _UartGetPinXCK

#endif // UARTS
#endif // _StdUART_H_

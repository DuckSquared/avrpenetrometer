/*************************************************************************

Title:    RS232.c private include file.

DESCRIPTION:
    #include'd for each USART used. Allows this same source code to be
    used for all the USART[s] controlled by RS232.c
USAGE:
    Automatically #include'd by RS232.c to provide the actual code. 

*************************************************************************/

/*********************** UARTn Tx/Rx LED Macros *************************/

#if defined(U_TxLedPort)                       // Defined only if UART uses Rx/Tx LED's

  // Switch both Rx and Tx LEDs off

  #define U_SetLedsOff()                    \
    SetPin(U_RxLedPort, U_RxLedBit, swOff); \
    SetPin(U_TxLedPort, U_TxLedBit, swOff)

  // Set Rx & Tx LED i/o pins to output low

  #define U_InitLeds()                     \
    U_SetLedsOff();                        \
    SetPinDir(U_RxLedPort, U_RxLedBit, 1); \
    SetPinDir(U_TxLedPort, U_TxLedBit, 1)

  #define U_SetRxLed(State) SetPin(U_RxLedPort, U_RxLedBit, State)
  #define U_SetTxLed(State) SetPin(U_TxLedPort, U_TxLedBit, State)

#else

  #define U_SetLedsOff()    /* null macro */
  #define U_InitLeds()      /* null macro */
  #define U_SetRxLed(State) /* null macro */
  #define U_SetTxLed(State) /* null macro */

#endif

#if(_RX_FLOWCTRL == UFC_RTSCTS)
  #define U_InitPinRTS()                        \
    SetPin(U_ModemPort, U_bitRTS, swOn);        \
    SetPinDir(U_ModemPort , U_bitRTS, dirOutput)
  #define U_SetPinRTS(State) SetPin(U_ModemPort, U_bitRTS, State)
#else
  #define U_InitPinRTS() /**/
  #define U_SetPinRTS()  /**/
#endif

#if(_TX_FLOWCTRL == UFC_RTSCTS)
  #define U_InitPinCTS()                       \
    SetPinDir(U_ModemPort, U_bitCTS, dirInput) \
    SetPinPullup(U_ModemPort, U_bitCTS, swOn)
  #define U_GetPinCTS() GetPin(U_ModemPort, U_bitCTS)
#else
  #define U_InitPinCTS() /**/
  #define U_GetPinCTS()  /**/
#endif

/*************************************************************************
Function: UARTn Receive Complete interrupt
Purpose:  called when the UARTn has received a character
**************************************************************************/

ISR(U_RXC_vect)
{
  byte TmpHead;
  byte Data;
  byte LastRxError;
 
  LastRxError = U_GetRxStatus() & (_BV(FE) | _BV(DOR));// Read UART status register
  Data        = U_Read();                              // and the UART data register
     
  // Check the current state of the RxD pin. This indicates that the next frame in a 
  // contiguous stream of frames is currently being received (de-serialised), so only
  // switch on the Receive LED if RxD is low. This assumes that the baudrate and 
  // latencies are low enough for this ISR at this point in its execution to still be
  // in the next frames start bit. While not bomb-proof, this method of controlling
  // the RxD LED is the only practical method that does not involve a pin-change ISR
  // or hardware timer.

  U_SetRxLed(U_IsReceiving());                         // Switch on/off the Receive LED
  //U_SetRxLed(1);

#if(_TX_FLOWCTRL == UFC_XONXOFF)
  
  // Check for an Xon or Xoff control character sent by the other machine.
  // This implements our Xon/Xoff transmit flow control
  
  if(LastRxError == 0)
    switch(Data)
    {
      case asciiXon:
        _Uart.CanTx = boTrue;                          // Used by PcUartPutC() 
        if(_Uart.TxHead != _Uart.TxTail)               // Is the Tx buffer empty?
          U_TxBufferEmptyInt(enEnable);                // Re-enable the UDRE interrupt
        break;
      case asciiXoff:
        _Uart.CanTx = boFalse;
      default:
        ;
    }
#endif

  TmpHead = (_Uart.RxHead + 1) & UART_RX_BUFFER_MASK;  // Calculate Rx buffer index  
  if(TmpHead == _Uart.RxTail)
    LastRxError = uartBufferOverflow >> 8;             // Error: Receive buffer overflow 
  else
  {
    _Uart.RxHead = TmpHead;                            // Store the new index
    _Uart.RxBuf[TmpHead] = Data;                       // Store received data in buffer
  }
  _Uart.LastRxError = LastRxError;

#if(_RX_FLOWCTRL != UFC_NONE)
 
  // Check the receive buffers level to see if it has reached the Xoff threashold.
  
  if(++_Uart.RxCount == UART_XOFF_LEVEL)               // Have we reached the RX Xoff threashold level?
  {
  #if(_RX_FLOWCTRL == UFC_XONXOFF)

    // Send an Xoff as soon as possible. This implements our Xon/Xoff receive flow control

    _Uart.SendX = asciiXoff;                           // Set the XonXoff pending to Xoff
    U_TxBufferEmptyInt(enEnable);                      // Enable UDRE interrupt (stuffs an Xoff into stream)
    
  #else
    // Take our RTS line low This implements our RTS/CTS receive flow control

    U_SetPinRTS(swOff);                                // Take RTS low (receive buffer nearly full)
  #endif
  }
#endif
}

/*************************************************************************

Function: USARTn Data Register Empty Interrupt Service Routine for RS232.c 
Purpose:  Invoked when the USARTn is ready to transmit the next byte,

DESCRIPTION:
    An interrupt is generated when the USART has transfered a byte from its
    transmit buffer register to its transmit shift register. This allows 
    the next byte to be transmitted to be loaded in the now empty Tx buffer.
    A circular buffer is used for buffering the transmit bytes, and this
    ISR is used to fetch the next byte from the software buffer and store it 
    in the USARTs hardware transmit buffer register.

*************************************************************************/

ISR(U_UDRE_vect)
{
byte TmpTail;

  U_SetTxLed(swOn);                                    // Switch on the transmit LED and/or enable the RS422/485 transmitter

#if(_RX_FLOWCTRL == UFC_XONXOFF)
  
  if(_Uart.SendX != asciiNul)                          // Is an Xon or Xoff pending?
  {
    U_Write(_Uart.SendX);                              // Transmit the Xon or Xoff character
    _Uart.SendX = asciiNul;                            // then clear the "XonXoff pending" flag
    return;
  }
  if((_Uart.TxHead != _Uart.TxTail) && (_Uart.CanTx))  // Tx buffer not empty & not Xoff?

#elif(_TX_FLOWCTRL == UFC_RTSCTS)

  if((_Uart.TxHead != _Uart.TxTail) && (!U_GetPinCTS)) // Tx buffer not empty & ClearToSend?

#else

  if(_Uart.TxHead != _Uart.TxTail)                     // Tx buffer not empty?

#endif
  {
    TmpTail = (_Uart.TxTail + 1) & UART_TX_BUFFER_MASK;// Calculate and store 
    _Uart.TxTail = TmpTail;                            // the new buffer index 
    U_Write(_Uart.TxBuf[TmpTail]);                     // Get next byte from buffer and write it to USART
  }
  else
    U_TxBufferEmptyInt(enDisable);                     // Tx buffer is now empty or !CanTx so disable UDRE interrupt
}

/*************************************************************************
Function: USARTn Transmit Complete interrupt (transmitter -> idle)
Purpose:  Used to switch off the transmitters indicator LED (port pin)
**************************************************************************/

#if defined(U_TxLedPort) || defined(_HALF_DUPLEX)      // Defined only if USARTn uses Rx/Tx LED's or half-duplex

ISR(U_TXC_vect, ISR_NAKED)
{
  U_SetTxLed(swOff);                                   // Does not affect SREG
  reti();
}

#endif

/*************************************************************************
Function: USARTn CTS Pin Change interrupt (transmitter)
Purpose:  Used to re-start transmitions when the CTS pin becomes active.
          Defined only when RTS/CTS flow control is used on the transmitter
**************************************************************************/

#if(_TX_FLOWCTRL == UFC_RTSCTS)

ISR(U_CTS_vect)
{
  if((_Uart.TxHead != _Uart.TxTail) && (_UartGetCTS))  // Tx buffer !empty & ClearToSend?
    U_TxBufferEmptyInt(enEnable);                      // Tx buffer not empty and CTS so ensable UDRE interrupt
}

#endif

/*************************************************************************
Function: USARTn_init()
Purpose:  initialize USARTn and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/

void U_INIT(void)
{
  byte Ctrl;

  U_InitLeds();                                 // Initialise the Tx/Rx LED indicator i/o pins
  U_InitPinRTS();                               // Initialise the CTS and RTS flow control pins
  U_InitPinCTS();

  _Uart.TxHead = 0;
  _Uart.TxTail = 0;
  _Uart.RxHead = 0;
  _Uart.RxTail = 0;

  U_SetFormat(umoAsync, udb8, upaNone, ust1);   // Mode = asynchonous, Frame Format = 8-0-1

#if(_RX_FLOWCTRL == UFC_XONXOFF)
  _Uart.RxCount = 0;
  _Uart.SendX   = asciiXon;                     // Send an Xon Control signal
  Ctrl = _BV(UDRIE);
#else
  Ctrl = 0;
#endif    

#if(_TX_FLOWCTRL == UFC_XONXOFF)
  _Uart.CanTx = boTrue;
#endif
#if defined(U_TxLedPort)
  Ctrl |= _BV(TXCIE);
#endif

  Ctrl |= (_BV(RXEN) | _BV(TXEN) | _BV(RXCIE)); // Enable Txer,Rxer, Rx_Complete & TxBuffer_Empty ints
  U_SetCtrl(Ctrl, enEnable);

#if(_TX_FLOWCTRL == UFC_RTSCTS)
  U_CTS_IntEnable();                            // Enable CTS -> Active interrupt (user-defined)
#endif
}

/*************************************************************************
Function: uart[n]_getc()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/

word U_GETC (void)
{    
  byte TmpTail;
  byte Data;
#if(_RX_FLOWCTRL == UFC_XONXOFF)
  byte StatusReg;
#endif  

  if (_Uart.RxHead == _Uart.RxTail) 
    return(uartNoData);                                // No data available
    
  // Check to see if the receive buffers level has dropped to the Xon threashold
  // This simple algorithm will send an Xon every time the Tx buffers drops to the
  // UART_XON_LEVEL, even when it never exceeded the UART_XOFF_LEVEL.

#if(_RX_FLOWCTRL == UFC_XONXOFF)

  StatusReg = SREG;                                    // Preserve the global interrupt flag
  SetGlobalInterrupts(enDisable);
  if(--_Uart.RxCount == UART_XON_LEVEL)
  {
    _Uart.SendX = asciiXon;                            // Set the Xon/Xoff variable to Xon
    U_TxBufferEmptyInt(enEnable);                      // Enable UDRE interrupt
  }
  SREG = StatusReg;                                    // Restore the global interrupt flag

#elif(_RX_FLOWCTRL == UFC_RTSCTS)

  if(--_Uart.RxCount == UART_XON_LEVEL)
    U_SetPinRTS(swOn);                                 // Set "Can Receive" Signal on

#endif  
  
  TmpTail = (_Uart.RxTail + 1) & UART_RX_BUFFER_MASK;  // Calculate and store buffer index
  _Uart.RxTail = TmpTail;
  Data = _Uart.RxBuf[TmpTail];                         // Get data from receive buffer
  return((_Uart.LastRxError << 8) + Data);             // Hi byte = Status, Lo byte = Data
}

/*************************************************************************
Function: uart[n]_gets(char* str, int size)
Purpose:  Reads at most (size - 1) bytes from the UART, until a newline 
          character is encountered, and stores the characters in the buffer 
          pointed to by str. Unless an error was encountered while reading, 
          the string will be terminated with an asNUL character.
Input:    str : Pointer to character array with at least (Max - 1) characters
          size: Size of the buffer pointed to by S.
Returns:  error status
**************************************************************************/

byte U_GETS (char *str, int size)
{    
  word Data;

  while(size > 0)
  {
    Data = U_GETC ();
    switch(Data >> 8)                               // Ok, Error or No Data
    {
      case 0:                                       // Valid data byte
        *str++ = Data & 0xFF;                       // Store next character++
        if((Data & 0xFF) == asLF)                   // Check for end-of-line
        {
          *str = 0;                                 // Terminate string will a Null
          return(0);                                // No errors, string terminated
        }
        size--;
        break;
      case uartNoData >> 8:                         // No receive char pending
        APP_UART_GETC_WAIT;                         // Allow application processing
        break;
      default:                                      // Receiver error (overrun, parity, etc)
        return(Data >> 8);                          // Receive error
    }
  }
  return(uartStringOverflow >> 8);                  // Not enough space in str
}

/*************************************************************************
Function: uart[n]_putc()
Purpose:  write byte to ringbuffer for transmitting via USARTn
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/

void U_PUTC (char data)
{
  byte TmpHead;

  TmpHead = (_Uart.TxHead + 1) & UART_TX_BUFFER_MASK;
  while(TmpHead == _Uart.TxTail)                     // Wait for free space in buffer
    APP_UART_PUTC_WAIT;                              // But allow application processing
  _Uart.TxBuf[TmpHead] = data;
  _Uart.TxHead = TmpHead;

#if(_TX_FLOWCTRL == UFC_XONXOFF)

  if(_Uart.CanTx)                                    // Okay to transmit?

#elif(_TX_FLOWCTRL == UFC_RTSCTS)

  if(!U_GetPinCTS())                                 // Okay to transmit (Clear To Send)?

#endif
    U_TxBufferEmptyInt(enEnable);                    // Enable the UDRE interrupt
}

/*************************************************************************
Function: uart[n]_puts()
Purpose:  transmit string to USARTn
Input:    string to be transmitted (null terminated)
Returns:  none          
**************************************************************************/

void U_PUTS (const char *s)
{
  while (*s) 
    U_PUTC(*s++);
}

/*************************************************************************
Function: uart[n]_puts_p()
Purpose:  transmit string from program memory to USARTn
Input:    program memory string to be transmitted (null terminated)
Returns:  none
**************************************************************************/

void U_PUTS_P (const char *progmem_s)
{
  register byte c;
    
  while((c = pgm_read_byte(progmem_s++)))
    U_PUTC(c);
}

/*************************************************************************
Function: uart[n]_putbytes()
Purpose:  transmit string to USARTn
Input:    p: pointer to bytes to be transmitted
          n: Number of bytes to be transmitted
Returns:  none          
**************************************************************************/

void U_PUTBYTES (const char *p, byte count)
{
  while(count--) 
    U_PUTC(*p++);
}

/*************************************************************************
Function: uart[n]_IsTransmitting()
Purpose:  Determines if USART[n] is currently transmitting
Input:    none
Returns:  True if UART has data being or pending transmition          
**************************************************************************/

boolean U_TX_BUF_IS_EMPTY (void)
{
#if(_RX_FLOWCTRL == UFC_XONXOFF)
  return((_Uart.TxHead == _Uart.TxTail) && !U_GetTxBufferEmptyInt());
#else
  return((_Uart.TxHead == _Uart.TxTail) && !U_GetTxBufferEmptyInt());
#endif
}

/*************************************************************************
Function: uart[n]_stuff_rx()
Purpose:  writes an array of bytes to the receive ringbuffer.
          Used for debugging .
Input:    s  Pointer to array of bytes to be transmitted
          n: Number of bytes to suff into Rx buffer. return when a bytes 
             have been stuffed or the buffer becomes full
Returns:  The number of bytes still to stuff (0 if all n bytes stuffed          
**************************************************************************/

byte U_STUFF_RX (char *s, byte n)
{
  byte TmpHead;

  TmpHead = _Uart.RxHead;
  while(n > 0)
  {
    TmpHead = (TmpHead + 1) & UART_RX_BUFFER_MASK;
    while(TmpHead == _Uart.RxTail)
      APP_UART_PUTC_WAIT;                              // But allow application processing
    _Uart.RxBuf[TmpHead] = *s++;                       // Store next byte, inc buffer pointer
    n--;
   }
   _Uart.RxHead = TmpHead;
  return(n);
}

// Undefine the Generic Definitions

#undef _Uart 
#undef _RX_FLOWCTRL
#undef _TX_FLOWCTRL

#undef _HALF_DUPLEX

#undef U_SetLedsOff
#undef U_InitLeds
#undef U_SetRxLed
#undef U_SetTxLed

#undef U_DATA
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
#undef U_XckPin

#undef U_TxLedPort
#undef U_RxLedPort
#undef U_TxLedDDR
#undef U_RxLedDDR
#undef U_RxLedBit
#undef U_TxLedBit

#undef U_GetPinCTS
#undef U_GetPinRTS
#undef U_SetPinRTS
#undef U_CTS_vect

#undef U_SetBaudrate
#undef U_SetCtrl
#undef U_EnableInts
#undef U_TxCompleteInt
#undef U_RxCompleteInt
#undef U_TxBufferEmptyInt
#undef U_GetTxBufferEmptyInt
#undef U_Write
#undef U_Read
#undef U_RxIsComplete
#undef U_GetRxStatus
#undef U_ClearIntFlags
#undef U_TxBufferIsEmpty
#undef U_TxBufferIsFull
#undef U_TxIsComplete
#undef U_ClearTxComplete

#undef U_GetPinRxD
#undef U_GetPinTxD
#undef U_GetPinXCK

#undef U_SetFormat
#undef U_Enable
#undef U_FlushRxBuffer
#undef U_IsReceiving
#undef U_Read
#undef U_Write

#undef U_RXC_vect
#undef U_TXC_vect
#undef U_UDRE_vect

#undef U_INIT
#undef U_GETC
#undef U_PUTC
#undef U_PUTS
#undef U_PUTS_P
#undef U_PUTBYTES
#undef U_GETS
#undef U_PUTS
#undef U_STUFF_RX
#undef U_TX_BUF_IS_EMPTY

/*   Copyright (c) 2009, Jason G. Burgon */
/*   All rights reserved.                */

#ifndef _AVR_STD_IO_MACROS_H
#define _AVR_STD_IO_MACROS_H 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include "StandardTypes.h"

typedef enum {boFalse, boTrue} boolean;       // A half-decent boolean type

// MCU Control

#define SetGlobalInterrupts(State) if(State) sei(); else cli()

// Return non-zero if global interrupts are enabled

#define GlobalIntsEnabled() (SREG && _BV(SREG_I))

// General Purpose I/O (GPIO)

typedef enum{stOff, stOn} TState;
typedef enum{Lo, Hi} TLoHi;
typedef enum{swOff, swOn} TSwitch;
typedef enum{enDisable, enEnable} TEnable;

typedef enum {dirInput, dirOutput} TPinDir;

// Single bit I/O Port macros

#define SFR(Port, Offset) _SFR_IO8(_SFR_IO_ADDR(Port) + Offset)

// All the XxxPin macros are used to set, clear or read a single bit of a digital
// I/O port. In all cases you must pass PORTx as the Port argument and NOT PINx
// or DDRx associated with that I/O port.
// 
// GetPin returns the boolean state of the associated PINxn register bit while
// GetOutPin returns the boolean state of the actual PORTxn bit. SetInPin sets or
// clears a PINn bit.
//
// IN ALL CASES PASS THE PORTx VALUE IN Port!

#define SetPin(Port, Pin, State) {if(State) Port |= _BV(Pin); else Port &= ~_BV(Pin);}

// Return the boolean state of the given Pin in the GPIO input register
// associated with the given Port register. Returns boTrue (1) or boFalse (0)

//#define GetPin(Port, Pin) (SFR(Port,-2) & (1 << Pin))      // Returns a bool, not a boolean
#define GetPin(Port, Pin) ((SFR(Port,-2) & (1 << Pin)) != 0) // Returns a boolean when required, but then very bloated

// Return the boolean state of the given bit in the PORT output register.
// Returns boTrue (1) or boFalse (0), regardless of bit position in register

#define GetOutPin(Port, Pin) ((Port & (1 << Pin)) != 0)      // Returns a boolean when required, but very bloated

#define SetPinPullUp(Port, Pin, State) SetPin(Port, Pin, State)  // Writing to PORTxn enables pull-ip when DDRxn is 0

#define SetPinDir(Port, Pin, Dir) SetPin(SFR(Port, -1), Pin, Dir)

// Return the boolean state of the given pin in the DDR register associated with the Port
// register.Returns boTrue (1) or boFalse (0), regardless of bit position in register.

#define GetPinDir(Port, Pin) ((SFR(Port,-1) & (1 << Pin)) != 0) // Returns a boolean when required, but very bloated

// Toggle the state of an output pin. Later AVR's can do this by writing
// a "1" to the corresponding PINx I/O register. 

#if defined _PINTOGGLE
  #define TogglePin(Port, Pin) (SetInPin(Port, Pin, 1))
#else
  #define TogglePin(Port, Pin) SetPin(Port, Pin, !GetOutPin(Port, Pin))
#endif

#define SetInPin(Port, Pin, State) SetPin(SFR(Port, -2), Pin, State)

// The macros work on the whole PORT/PIN/DDR register and can set/get/clear multiple
// I/O register bits at the same time. For this reason the bit(s) are specified as a bit-mask

#define SetPort(Port, PinMask, State) {if(State) (Port |=  PinMask); else (Port &= ~PinMask);}

// GetPort returns the state of the input PIN register associated with the PORT and
// masked by the PinMask bit field

#define GetPort(Port, PinMask) (SFR(Port, -2) & PinMask)

// SetPortDir sets (o/p) or clears (i/p) the DDR register bits specified in PinMask bit field

#define SetPortDir(Port, PinMask, Dir) SetPort(SFR(Port,-1), PinMask, Dir)

// GetPortDir returns the value of Port's Data Direction Register

#define GetPortDir(Port) (SFR(Port, -1))

// ReadPort returns the value of Port's input PIN Register

#define ReadPort(Port) (SFR(Port, -2))

// Special Registers single bit read/write

#define SetReg(Reg, BitNum, State) SetPin(Reg, BitNum, State)
#define GetReg(Reg, BitNum)        GetOutPin(Port, Pin)

// External Interrupt Control (INT0, INT1, INT2)

typedef enum {einINT0 = 6, einINT1 = 7, einINT2 = 5} TExtIntNum; // GICR interrupt enable bit numbers

#define ExtIntEnable(EinNum, State) if (State) EIMSK |= _BV(EinNum); else EIMSK &= ~(_BV(EinNum))

typedef enum{eisLo, eisChange, eisHiToLo, eisLoToHi} TExtIntSense;

#define SetExtIntSense(LsbPos, Sense)                       \
  switch (Sense) {                                          \
    case eisLo:                                             \
      EICRA &= ~(0x3 << LsbPos);                            \
      break;                                                \
    case eisChange:                                         \
      EICRA = (EICRA & ~(0x3 << LsbPos)) | (0x1 << LsbPos); \
      break;                                                \
    case eisHiToLo:                                         \
      EICRA = (EICRA & ~(0x1 << LsbPos)) | (0x2 << LsbPos); \
      break;                                                \
    case eisLoToHi:                                         \
      EICRA |= (0x3 << LsbPos);                             \
  }                                                         \

#define ExtIntClearFlag(ExtInt) EIFR |= _BV(ExtInt)

// Get the current value of the stack pointer

#ifdef SPH
  #define GetStackPtr() _SFR_IO16(_SFR_IO_ADDR(SPL))
#else
  #define GetStackPtr() _SFR(SPL)
#endif

#endif // _AVR_STD_IO_MACROS_H

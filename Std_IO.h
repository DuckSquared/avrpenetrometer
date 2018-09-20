/************************************************************************
*                                                                       *
* Title:    Standardised AVR hardware definitions except UARTS.         *
* Purpose:  Defines a device-independant set of nmenonics and macros for*
*           most of the common hardware devices on a particular chip.   *
* Author:   Jason Burgon  <jaybur@ntlworld/com>                         *
* Software: AVR-GCC 4.18                                                *
* Hardware: Most AVR Tiny and Mega devices.                             *
* Usage:    #include Std_IO.h can be used with or without Std_IO.c      *
*                                                                       *
*************************************************************************/

#ifndef Std_IO_H
#define Std_IO_H 1

#include <avr/io.h>

// System and Watchdog hardware

//#ifdef MCUCR
//  #define MCUSR MCUCR                   // MCUCR -> MCUSR
//#endif

#if defined(MCUCSR) && !defined(MCUSR)
  #define MCUSR MCUCSR                    // MCUCSR -> MCUSR
#endif

#if defined(WDTCR) && !defined(WDTCSR)    // WDTCR -> WDTCSR
  #define WDTCSR WDTCR
#endif

#if defined(WDTOE) && !defined(WDCE)
  #define WDCE WDTOE                      // WDTOE -> WDCE 
#endif

#if defined(PRR) && !defined(PRR0)
  #define PRR0 PRR                        // PRR   -> PRR0 
#endif

// Timer/Counter 1

#if defined(TCCR1) & !defined(TCCR1A)
  #define TCCR1A TCCR1                    // TCCR1 -> TCCR1A
    #define WGM12 CTC1                    // CTC1  -> WGM12 (TCCR1.CTC1  -> TCCR1.WGM12)
    #define WGM11 PWM1A                   // PWM1A -> WGM11 (TCCR1.PWM1A -> TCCR1.WGM11)
    #define WGM13 8                       // (1 << 8) -> 0 with an 8-bit register

  #if defined(GTCCR) & !defined(TCCR1B)
    #define TCCR1B GTCCR                  // GTCCR -> TCCR1B
  #endif
  #ifdef PWM1B
    #define WGM10  PWM1B
  #endif
#endif

// Timer/Counter 2

#if defined(OCR2)
  #define OCR2A  OCR2
  #define TCCR2A TCCR2
  #define TCCR2B TCCR2
  #define COM2A0 COM20
  #define COM2A1 COM21
  #define OCIE2A OCIE2
  #define TIMER2_COMPA_vect TIMER2_COMP_vect
#endif

#if defined(SFIOR) && !defined(GTCCR)
  #define GTCCR SFIOR
#elif defined(PSRASY) & !defined(PSR2)
  #define PSR2 PSRASY
#endif

#if defined(OCF2) & !defined(OCF2A)
  #define OCF2A OCF2
#endif

// Create standardized aliases for the combined TIMSK register found on smaller AVR's
// Note that this means that the TIMSK[n] bit-flags cannot be homogenized so they
// don't refer to their particular timer/counter. Some AVR's with more than 3 timers
// use a separate TIMSK[n] register for each timer/counter while others have a 
// TIMSK for TC0..3 and ETIMSK for TC3. This makes it a bloody mess.

#if defined(TIMSK) && !defined(TIMSK0)
  #define TIMSK0 TIMSK
#endif
#if defined(TIMSK) && !defined(TIMSK1)
  #define TIMSK1 TIMSK
#endif
#if defined(TIMSK) && !defined(TIMSK2)
  #define TIMSK2 TIMSK
#endif
#if defined(ETIMSK)
  #define TIMSK3 ETIMSK
#endif

#if defined(TIFR)
  #if defined(TOV0) && !defined(TIFR0)
    #define TIFR0 TIFR
  #endif
  #if defined(TOV1) && !defined(TIFR1)
    #define TIFR1 TIFR
  #endif
  #if defined(TOV2) && !defined(TIFR2)
    #define TIFR2 TIFR
  #endif
  #if defined(TOV3) && !defined(TIFR3)
    #define TIFR3 TIFR
  #endif
#endif

// Standardize the interrupt-based register names to EIMSK, EIFR and EICRA

#if defined(GICR) && !defined(EIMSK)
  #define EIMSK  GICR                     // GICR  -> EIMSK (ExtInt INTn bits)
  #define EIFR   GIFR                     // GIFR  -> EIFR
  #define EICRA  MCUCR                    // MCUCR -> EICRA
#endif
  
// Create standardized aliases for the Pin Change and 
// the External Interrupt Registers and register bits

#if defined(GICR) && !defined(PCICR)
  #define PCICR GICR                      // GICR -> PCICR (PinChange PCIEn bits)
#endif

#if defined(IVSEL) && !defined(MCUCR)
  #define MCUCR GICR                      // GICR -> MCUCR (Int Vec Table IVSEL, IVCE bits)
#endif

#if defined(GIFR) && !defined(PCIFR)
  #define PCIFR GIFR                      // GIFR -> PCIFR (PinChange Flag Register PCIFn bits)
  #if defined(PCIF) && !defined(PCIF0)
    #define PCIF0 PCIF                    // PCIF -> PCIF0 (GIFR.PCIF -> PCIFR.PCIF0)
  #endif
#endif

#if defined(GIFR) && !defined(EIFR)       // GIFR -> EIFR (External Interrupt Flags Reg)
  #define EIFR GIFR
#endif

#if defined(PCMSK) && !defined(PCMSK0)
  #define PCMSK0 PCMSK                    // PCMSK -> PCMSK0
#endif

#if defined(GIMSK) && !defined(PCICR)
  #define PCICR GIMSK                     // GIMSK -> PCICR (Pin Change Int Enable PCIEn bits)
  #if defined(PCIE) 
    #define PCIE0 PCIE                    // PCIE  -> PCIE0 (GIMSK.PCIE -> PCIE0.PCIE)
  #endif
#endif

#if !defined(EICRA)
  #define EICRA  MCUCR                    // MCUCR -> EICRA (Ext Int Sense ISCnn bits )
#endif

#endif

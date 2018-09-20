#ifndef CNC_CMD_CODES_H
#define CNC_CMD_CODES_H

#include "AsciiCtrl.h"

// PC-sent Command Codes

#define cmdInit        '@'    // asEnq  // Establish communications link
#define cmdDone        '~'    // asEoF  // Disconnect and power-down, make safe
#define cmdSaveParams  '^'    // asSoH  // Commit current parameters to EEPROM

// The Emergency Stop command is used to go in and out of E-Stop mode.
// [0]    cmdEStop
// [1]    ' '
// [2]    Value        - Either ASCII '0' (Clear E-Stop) or '1' (Set EStop)
// [n+1]  End-of-Line1 - asCR (ASCII carriage return #13)
// [n+2]  End-of-Line2 - asLF (ASCII line feed #10)

// cmdEStop is sent by us to notify the PC that that the Tester has gone
// into or out of Emergency (E-Stop) mode. cmdEStop(false) is also sent by
// the PC to (try) and bring us out of E-Stop mode

#define cmdSetEStop      'e' // asEsc   // Emergency stop

// Axis-specific commands. All these commands have the following structure:
// [0]      CommandCode  - Lowercase ASCII command code letter (as below)
// [1]      AxisID       - Case-insensitive axis identifier('X', 'Y', 'Z' or 'A')
// [2]..[n] Value        - Parameter value as an ASCII encoded decimal number
// [n+1]    End-of-Line1 - asCR (ASCII carriage return #13)
// [n+2]    End-of-Line2 - asLF (ASCII line feed #10)

//#define cmdMoveBy      'm'  // Move axis by the given number of steps
#define cmdSetAccel      'a'  // Set acceleration (steps/sec/sec)
#define cmdSetHomeIsPlus 'c'  // Set which end of the travel the homing switch is
#define cmdSetDecel      'd'  // Set Deceleration (steps/sec/sec)
#define cmdGoTo          'g'  // Move axis to a new position (in steps)
#define cmdSetDirInvert  'i'  // Invert the Direction output pin if set
#define cmdSetEnInvert   'j'
#define cmdSetHomeSpeed  'k'  // Set axis homing speed (steps/sec)
#define cmdSetLimitMin   'l'  // Set lower travel limit (steps)
#define cmdSetHomeInvert 'o'
#define cmdSetEnable     'p'  // Switch the "Motor Enable" (Power) signal on or off
#define cmdSetStepsPerM  'q'  // Set the number of steps per meter or steps per rev
#define cmdReposition    'r'  // Set Position (absolute steps) without moving
#define cmdSetSpeed      's'  // Set working speed (feed rate) in steps/sec
#define cmdSetTopSpeed   't'  // Set axis maximum allowed speed (steps/sec)
#define cmdSetLimitMax   'u'  // Set upper travel limit (steps)
#define cmdSimLimitSw    'v'  // Simulate limit switch state
#define cmdSetPulseW     'w'  // Set "Step" pulse width (in micro-seconds)
#define cmdRefHome       'z'  // Perform a reference homing cycle - needs a home/limit switch

// SetParams and GetParams send all their parameters as little-endian
// binary numbers and are not terminated with an EoL (or anything else).

//#define cmdGetParams   'P'  // Get all parameters (PulseWidth..LimitMax)

// Parameter "getters" have the same structure as above except that
// the Value field is absent.

#define cmdGetAccel      'A'  // Get the currently active rate of acceleration in steps/sec/sec
#define cmdGetBaseFreq   'B'  // Return the motion controllers timer Base frequency in Hz (read only)
#define cmdGetHomeIsPlus 'C'
#define cmdGetDecel      'D'  // Get the currently active rate of deceleration in steps/sec/sec
#define cmdGetEStop      'E'  // Return the current EStop state
#define cmdGetFault      'F'  // Get the motor driver "Fault" signal state (read only)
#define cmdGetTargetPos  'G'  // Get the current target position
#define cmdGetHomeState  'H'  // Get the current homing/limit switch state
#define cmdGetDirInvert  'I'  // Dir(ection) pin is inverted if 1 is returned
#define cmdGetEnInvert   'J'
#define cmdGetHomeSpeed  'K'  // Get the currently active homing speed (homing Feedrate)
#define cmdGetPosMin     'L'  // Get the SoftLimits minimum position
#define cmdGetSpeedMax   'M'
#define cmdGetCurrentPos 'N'  // Get the current target position
#define cmdGetHomeInvert 'O'
#define cmdGetEnable     'P'  // Get the current "Motor Enable" (Power) signal state
#define cmdGetStepsPerX  'Q'  // Get the number of steps per unit length or steps per rev
#define cmdGetSpeed      'S'  // Get the currently active top speed (active Feedrate)
#define cmdGetTopSpeed   'T'  // Get the absolute max allowed speed (feedrate limit)
#define cmdGetPosMax     'U'  // Get the SoftLimits maximum position
#define cmdGetPulseW     'W'  // Get the current "Step" pulse width in microseconds
#define cmdGetAccelMax   'Y'
#define cmdIsRefHomed    'Z'  // A "Reference Home" cycle has been performed

// All parameter setter commands sent from the PC generate a response message
// of the same structure. A valid command is denoted by the CmdCode being the
// uppercase equivalent of the lowercase command that caused it. Parameter
// getter commands are already uppercase.

#define rspGetSpeed      'S' // Get current speed (feed rate) in steps/sec
#define rspGetTopSpeed   'T' // Get maximum allowed speed (steps/sec)
#define rspGetHomeSpeed  'B' // Get axis homing speed (steps/sec)
#define rspGetAccel      'A' // Get acceleration (steps/sec/sec)
#define rspGetDecel      'D' // Get Deceleration (steps/sec/sec)
#define rspGetPosMin     'L' // Get lower travel limit (steps)
#define rspGetPosMax     'U' // Get upper travel limit (steps)
#define rspGetPulseW     'W' // Get "Step" pulse width (in micro-seconds)
#define rspGetDirInvert  'I' // Get Direction output pin Inverted setting (true/false)
#define rspGetEnInvert   'J'
#define rspSetStepsPerM  'Q' // Get the number of steps per meter (per revolution for cyclic axes)
#define rspGetTargetPos  'N' // Get current target position
#define rspGetHomeIsPlus 'C' // Get which end of axis is "Home"
#define rspHomeState     'H' // Current state of homing/limit switches
#define rspGetAccelMax   'C'
#define rspGetSpeedMax   'M'
 
// Unsolicited messages generated by us

// These position information messages are generated periodically whenever
// the associated axis is moving. The Axis position is sent as a signed
// 32-bit little-endian binary value, [0] = msgPosN  [1..4] = Value

#define msgPosX      asDC1   // X-axis positional feedback
#define msgPosY      asDC2   // Y-axis positional feedback
#define msgPosZ      asDC3   // Z-axis positional feedback
#define msgPosA      asDC4   // A-axis positional feedback

#define msgAxisStart asSTx   // Axis started moving
#define msgAxisStop  asETx   // Axis stopped moving

// A "NewMove" message is generated by us at the start of an axis move.
//[0]    msgNewMove
//[1..3] CurPos      - Current (start) position 
//[4..7] TargetPos   - New target (destination) position

//#define msgNewMove 'N'

#endif

/*
 * avrpenetrometer.h
 *
 * Created: 21/06/2018 01:12:35
 *  Author: Jake Bird
 */

#define ERR_NONE 0
#define ERR_ESTOP 1
#define ERR_UNRECOGNISED_INSTRUCTION 2
#define ERR_BUSY 3
#define ERR_STATE 4
#define ERR_PARAMETER 5
#define ERR_LIMIT_EXCEEDED 6
#define ERR_HARDWARE_FAULT 7
#define ERR_HARDWARE_FAILURE 8
#define ERR_NO_COMMS 9
#define ERR_UNKNOWN 10 // ':'

#define avrErrMotor 'm'
#define avrErrLFD 'l'
#define avrErrForce 'f'
#define avrErrForceDelta 'd'
#define avrErrDCell 'c'
#define avrErrCnc 'b'
#define avrErrAvr 'a'

#define cncErrConstrained '/'
#define cncErrNone '0'
#define cncErrAxis '1'
#define cncErrParameter '2'
#define cncErrCommand '3'
#define cncErrEeprom '4'
#define cncErrHardware '5'
#define cncErrState '6'
#define cncErrComms '7'

#define STEPS_PER_DMM 32
#define STEPS_PER_MM 320

#define STEPS_MAX 100
#define STEPS_MIN 1
#define TOLER_MAX 255
#define TOLER_MIN 0
#define FORCE_MAX 10000
#define FORCE_MIN -10000
#define ROBOT_TIMEOUT 3000
#define CNC_TIMEOUT 100
#define DCELL_TIMEOUT 100

#define STATION_NUMBER 1

#define cncEoL '\n'
#define cncEoLString "\n"

#define cncInitString			"@"
#define cncDoneString			"~"
#define cncSaveParamsString		"^"
#define cncSetEStopString		"e"
#define cncGoToString			"g"
#define cncRefHomeString		"z"
#define cncSetTopSpeedString	"t"
#define cncSetSpeedString		"s"
#define cncSetHomeSpeedString	"k"
#define cncSetAccelString		"a"
#define cncSetDecelString		"d"
#define cncSetLimitMinString	"l"
#define cncSetLimitMaxString	"u"
#define cncSetPulseWString		"w"
#define cncSetDirInvertString	"i"
#define cncSetEnInvertString	"j"
#define cncSetHomeInvertString  "o"
#define cncSetHomeIsPlusString	"c"
#define cncSetStepsPerMString	"q"
#define cncRepositionString		"r"
#define cncSetEnableString		"p"
#define cncSimLimitSwString		"v"
#define cncGetTargetPosString	"G"
#define cncGetCurrentPosString	"N"
#define cncGetHomeStateString	"H"
#define cncGetTopSpeedString	"T"
#define cncGetSpeedString		"S"
#define cncGetHomeSpeedString	"K"
#define cncGetAccelString		"A"
#define cncGetDecelString		"D"
#define cncGetPosMinString		"L"
#define cncGetPosMaxString		"U"
#define cncGetPulseWString		"W"
#define cncGetBaseFreqString	"B"
#define cncGetStepsPerXString	"Q"
#define cncGetDirInvertString	"I"
#define cncGetEnInvertString    "J"
#define cncGetHomeInvertString  "O"
#define cncGetHomeIsPlusString  "C"
#define cncGetEStopString		"E"
#define cncGetEnableString		"P"
#define cncGetFaultString		"F"
#define cncGetAccelMaxString	"Y"
#define cncGetSpeedMaxString	"M"
#define cncIsRefHomedString		"Z"

#define cncInitCmd cncInitString "CNC" cncEoLString
#define cncDoneCmd cncDoneString " " cncEoLString
#define cncSaveParamsCmd cncSaveParamsString " " cncEoLString
#define cncSetEStopCmd cncSetEStopString "1" cncEoLString
#define cncClearEStopCmd cncSetEStopString "0" cncEoLString
#define cncRefHomeCmd cncRefHomeString "X" cncEoLString
#define cncGetTargetPosCmd cncGetTargetPosString "X" cncEoLString
#define cncGetHomeStateCmd cncGetHomeStateString "X" cncEoLString
#define cncGetTopSpeedCmd cncGetTopSpeedString "X" cncEoLString
#define cncGetSpeedCmd cncGetSpeedString "X" cncEoLString
#define cncGetHomeSpeedCmd cncGetHomeSpeedString "X" cncEoLString
#define cncGetAccelCmd cncGetAccelString "X" cncEoLString
#define cncGetDecelCmd cncGetDecelString "X" cncEoLString
#define cncGetPosMinCmd cncGetPosMinString "X" cncEoLString
#define cncGetPosMaxCmd cncGetPosMaxString "X" cncEoLString
#define cncGetPulseWCmd cncGetPulseWString "X" cncEoLString
#define cncGetBaseFreqCmd cncGetBaseFreqString "X" cncEoLString
#define cncGetStepsPerXCmd cncGetStepsPerXString "X" cncEoLString
#define cncGetDirInvertCmd cncGetDirInvertString "X" cncEoLString
#define cncGetEnInvertCmd cncGetEnInvertString "X" cncEoLString
#define cncGetEStopCmd cncGetEStopString " " cncEoLString
#define cncGetEnableCmd cncGetEnableString "X" cncEoLString
#define cncGetFaultCmd cncGetFaultString "X" cncEoLString
#define cncIsRefHomedCmd cncIsRefHomedString "X" cncEoLString
#define cncGetAccelMaxCmd cncGetAccelMaxString "X" cncEoLString
#define cncGetSpeedMaxCmd cncGetSpeedMaxString "X" cncEoLString

#define cncHsMidZone 0
#define cncHsMinZone 1
#define cncHsMaxZone 2
#define cncHsMovingOff 3
#define cncHsIsHoming 4
#define cncHsHomed 5
#define cncHsMinLimit 6
#define cncHsMaxLimit 7
#define cncHsSensor1 8
#define cncHsSensor2 9
#define cncHsSensor3 10
#define cncHsSensor4 11

#define avrEoL '\n'
#define avrEoLString "\n"

#define avrNone ' '
#define avrInit '@'
#define avrDone '~'
#define avrSave '^'
#define avrLog '#'
#define avrDoProbe '!'
#define avrData '*'
#define avrDataSeparator ','
#define avrDoRefHome 'z'

#define avrSetEStop 'e'
#define avrSetGroundLevel 'g'
#define avrSetTopSpeed 't'
#define avrSetSpeed 's'
#define avrSetHomeSpeed 'k'
#define avrSetAccel 'a'
#define avrSetDecel 'd'
#define avrSetProbeDepth 'l'
#define avrSetForce 'w'
#define avrSetStepsPerX 'q'
#define avrSetLFDTolerance 'r'
#define avrSetEnable 'p'
#define avrSetMaxForce 'm'
#define avrSetMinForce 'n'
#define avrSetMaxForceDelta 'x'
#define avrSetMinForceDelta 'y'
#define avrSetForceDeltaAbs 'v'
#define avrSetSafeDisconnect 'o'
#define avrSetHomeState 'h'
#define avrSetLFDState 'u'
#define avrSetProbeState 'j'
#define avrSetError 'f'
#define avrCncPassthrough 'b'
#define avrDCellPassthrough 'c'

#define avrIsRefHomed 'Z'
#define avrGetEStop 'E'
#define avrGetGroundLevel 'G'
#define avrGetTopSpeed 'T'
#define avrGetSpeed 'S'
#define avrGetHomeSpeed 'K'
#define avrGetAccel 'A'
#define avrGetDecel 'D'
#define avrGetProbeDepth 'L'
#define avrGetForce 'W'
#define avrGetStepsPerX 'Q'
#define avrGetLFDTolerance 'R'
#define avrGetEnable 'P'
#define avrGetMaxForce 'M'
#define avrGetMinForce 'N'
#define avrGetMaxForceDelta 'X'
#define avrGetMinForceDelta 'Y'
#define avrGetForceDeltaAbs 'V'
#define avrGetSafeDisconnect 'O'
#define avrGetHomeState 'H'
#define avrGetLFDState 'U'
#define avrGetProbeState 'J'
#define avrGetError 'F'

#define avrNoneString " "
#define avrInitString "@"
#define avrDoneString "~"
#define avrSaveString "^"
#define avrLogString "#"
#define avrDoProbeString "!"
#define avrDataString "*"
#define avrDataSeparatorString ","
#define avrDoRefHomeString "z"

#define avrSetEStopString "e"
#define avrSetGroundLevelString "g"
#define avrSetTopSpeedString "t"
#define avrSetSpeedString "s"
#define avrSetHomeSpeedString "k"
#define avrSetAccelString "a"
#define avrSetDecelString "d"
#define avrSetProbeDepthString "l"
#define avrSetForceString "w"
#define avrSetStepsPerXString "q"
#define avrSetLFDToleranceString "r"
#define avrSetEnableString "p"
#define avrSetMaxForceString "m"
#define avrSetMinForceString "n"
#define avrSetMaxForceDeltaString "x"
#define avrSetMinForceDeltaString "y"
#define avrSetForceDeltaAbsString "v"
#define avrSetSafeDisconnectString "o"
#define avrSetHomeStateString "h"
#define avrSetLFDStateString "u"
#define avrSetErrorString "f"
#define avrCncPassthroughString "b"
#define avrDCellPassthroughString "c"

#define avrIsRefHomedString "Z"
#define avrGetEStopString "E"
#define avrGetGroundLevelString "G"
#define avrGetTopSpeedString "T"
#define avrGetSpeedString "S"
#define avrGetHomeSpeedString "K"
#define avrGetAccelString "A"
#define avrGetDecelString "D"
#define avrGetProbeDepthString "L"
#define avrGetForceString "W"
#define avrGetStepsPerXString "Q"
#define avrGetLFDToleranceString "R"
#define avrGetEnableString "P"
#define avrGetMaxForceString "M"
#define avrGetMinForceString "N"
#define avrGetMaxForceDeltaString "X"
#define avrGetMinForceDeltaString "Y"
#define avrGetForceDeltaAbsString "V"
#define avrGetSafeDisconnectString "O"
#define avrGetHomeStateString "H"
#define avrGetLFDStateString "U"
#define avrGetErrorString "F"

#define avrPing avrNoneString avrEoLString

#define cnc_getc		uart1_getc
#define cnc_gets		uart1_gets
#define cnc_putbytes	uart1_putbytes
#define cnc_putc		uart1_putc
#define cnc_puts		uart1_puts

#define dcell_getc		uart2_getc
#define dcell_gets		uart2_gets
#define dcell_putbytes	uart2_putbytes
#define dcell_putc		uart2_putc
#define dcell_puts		uart2_puts

#define robot_getc		uart0_getc
#define robot_gets		uart0_gets
#define robot_putbytes	uart0_putbytes
#define robot_putc		uart0_putc
#define robot_puts		uart0_puts

#define port_IsMoving	PORTJ
#define pin_IsMoving	PJ0

#define port_LFD		PORTJ
#define pin_LFD			PJ1

#define port_DoSample	PORTD
#define pin_DoSample	PD0

#define port_Estop		PORTD
#define pin_Estop		PD1

#define ISR_DoSample	INT0_vect
#define ISR_Estop		INT1_vect
#define ISR_Tick		TIMER1_OVF_vect

/*#define TICK_HZ 100UL
#define T1_TICKS (F_CPU / TICK_HZ)
#if (T1_TICKS <= 65536)
	#define T1_PRESCALE ((1 << CS12) | (1 << CS11) | (0 << CS10) | (1 << WGM13) | (1 << WGM12))
	#define T1_COUNT T1_TICKS
#else
	#define T1_PRESCALE ((0 << CS12) | (0 << CS11) | (1 << CS10) | (1 << WGM13) | (1 << WGM12))
	#define T1_COUNT (T1_TICKS / 8)
#endif*/
#define T1_MODE_DISABLE ((1 << WGM11) | (1 << WGM10) | (0 << COM1A0) | (0 << COM1A1))

#define T1_COUNT 20000
#define T1_PRESCALE ((0 << CS12) | (0 << CS11) | (1 << CS10) | (1 << WGM13) | (1 << WGM12))

byte TxWait(void);
dword GetTick(void);
boolean IsMoving(void);

void DCellTransmit(void);
void DCellReadChar(void);
void DCellSetChecksum(void);
byte DCellVerifyChecksum(void);
void DCellReadPacket(void);
void DCellReadCommand(word startRegister);
void DCellWriteCommand(word startRegister, word lowerRegister, word upperRegister);
void DCellFlush(void);
void CreateForcePacket(void);
void DCellGetForce(void);
void DCellGetStationNumber(void);
void DCellListen(void);
void DCellPassthrough(void);

long ConvertDMMtoSteps(long inValue);
long ConvertStepstoDMM(char* inString);
long ConvertMMtoSteps(long inValue);
long ConvertStepstoMM(char* inString);
void ConvertForceToInt(void);

void CncTransmit(char* line);
void CncReadChar(void);
void CncSend(char cmd, long parameter);
void CncReadLine(void);
void CncFlush(void);
void CncInit(void);
void CncDone(void);
void CncSaveParams(void);
void CncSetEStop(void);
void CncClearEStop(void);
void CncGoTo(long newPosition);
void CncRefHome(void);
void CncSetTopSpeed(long newTopSpeed);
void CncSetSpeed(long newSpeed);
void CncSetHomeSpeed(long newHomeSpeed);
void CncSetAccel(long newAcceleration);
void CncSetDecel(long newDeceleration);
void CncSetLimitMin(long newLimitMin);
void CncSetLimitMax(long newLimitMax);
void CncSetPulseW(long newPulseW);
void CncSetDirInvert(long newDirInvert);
void CncSetEnInvert(long newEnInvert);
void CncSetHomeIsPlus(long newHomeIsPlus);
void CncSetStepsPerM(long newStepsPerM);
void CncReposition(long newPosition);
void CncSetEnable(long newEnable);
void CncSimLimitSw(long newSimLimitSw);
void CncGetTargetPos(void);
void CncGetHomeState(void);
void CncGetTopSpeed(void);
void CncGetSpeed(void);
void CncGetHomeSpeed(void);
void CncGetAccel(void);
void CncGetDecel(void);
void CncGetPosMin(void);
void CncGetPosMax(void);
void CncGetPulseW(void);
void CncGetBaseFreq(void);
void CncGetStepsPerX(void);
void CncGetDirInvert(void);
void CncGetEnInvert(void);
void CncGetEStop(void);
void CncGetEnable(void);
void CncGetFault(void);
void CncGetAccelMax(void);
void CncGetSpeedMax(void);
void CncIsRefHomed(void);
void CncListen(void);
void CncPassthrough(void);

void RobotTransmit(char* line);
void RobotReadChar(void);
void RobotSend(char cmd, word parameter);
void RobotForceData(void);
void Init(void);
void Done(void);
void Save(void);
void Log(void);
void DoProbe(byte probeDir);
void DoRefHome(void);
void GetEStop(void);
void SetEStop(word newEstop);
void GetError(void);
void GetErrorEstop(void);
void ThrowError(byte newErrorNum, char newErrorParam);
void ClearError(void);
void GetParamAvr(void);
void GetParamCnc(void);
void SetParamAvr(int16_t newParam);
void SetParamCnc(word newParam);
void GetForce(void);
void RobotListen(void);

void DoSafeRefHome(byte newErrorNum, char newErrorParam);
void DoEStop(byte newErrorNum, char newErrorParam);
void ISRInit(void);
void TimerInit(void);

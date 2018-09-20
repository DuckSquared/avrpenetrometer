/*
 * avrpenetrometer.c
 *
 * Created: 02/05/2018 11:22:33
 * Author : Jake Bird
 */

#include <avr/io.h>
#include <util/delay.h>

#include "stdlib.h"
#include "string.h"

#include "AsciiCtrl.h"
#include "CncCmdCodes.h"
#include "DCell.h"
#include "RS232_Opts.h"
#include "StandardTypes.h"
#include "Std_IO.h"
#include "Std_IO_Macros.h"
#include "StdUART.h"
#include "RS232.h"

#include "avrpenetrometer.h"

byte Index = 0;
char Line[64];

byte toRobotIndex = 0; // into Robot from AVR
volatile dword toRobotTime = 0; // time of last message sent to Robot
byte fromRobotIndex = 0; // from Robot to AVR
byte fromRobotReady = 0; // a full line has been transmitted and is ready to be acted on
volatile byte waitingForRobot = 0;
dword fromRobotTime = 0; // time of last complete message from Robot
byte toCncIndex = 0; // into CnC from AVR
dword toCncTime = 0; // time of last message sent to Cnc
byte fromCncIndex = 0; // from Cnc to AVR
byte fromCncReady = 0;
volatile byte waitingForCnc = 0;
dword fromCncTime = 0; // time of last complete message from Cnc
byte toDCellIndex = 0; // into DCell from AVR
volatile dword toDCellTime = 0; // time of last message sent to DCell
byte fromDCellIndex = 0; // from DCell to AVR
byte fromDCellReady = 0;
volatile byte waitingForDCell = 0;
dword fromDCellTime = 0; // time of last complete message from DCell

char toRobot[64];
char fromRobot[64];
char toCnc[64];
char fromCnc[64];
char forcePacket[8];
char toDCell[16];
char fromDCell[16];
byte toDCellLength = 8;
byte fromDCellLength = 8;

volatile byte isHoming = 0;
volatile byte isProbing = 0;
volatile byte startedMoving = 0;
byte logging = 0;
volatile byte estop = 0;
volatile byte newestop = 0;
volatile byte LFDcount = 12;
volatile byte LFDtolerance = 12;
volatile byte LFDstate = 0;
volatile byte probeState = 1;

volatile dword robotWatchdog = ROBOT_TIMEOUT;
volatile dword robotWatchdogState = 0;

byte probeDir = 0;
byte readSample = 0;
volatile byte errorNum = 0;
volatile char errorParam = ' ';
long groundLevel = 2500;
long maxDepth = 5000;
long sampleCount = 0;
volatile dword tick = 0;
int lastForce = 0;
int currentForce = 0;
int maxForce = 100;
int minForce = -100;
int maxForceDelta = 100;
int minForceDelta = -100;
byte forceDeltaAbs = 1;
byte safeDisconnect = 0;

byte motorEnable = 0;
byte motorEstop = 0;
byte motorFault = 0;
byte homeState = 0;
byte isRefHomed = 0;
long acceleration = 0;
long accMax = 0;
long deceleration = 0;
long speedMax = 0;
long homeSpeed = 0;
long posMin = 0;
long posMax = 0;
long stepsPerX = 0;
long speed = 0;
long topSpeed = 0;
long conValue = 0;

char currentTask = ' ';
char expectedRsp = ' ';
long currentParameter = 0;
byte currentStep = 0;

// Wait until finished transmitting before we start receiving

byte TxWait(void)
{
	word c;

	do
	{
		while(GetPin(Uart1TxLedPort, Uart1TxLedBit)) // Wait until finished transmitting
		{
			c = uart0_getc();
			if((c != uartNoData) && (Index < sizeof(Line)))
			{
				Line[Index] = (c & 0xFF);
				Index++;
			}
		}
		_delay_ms(2);
	}
	while(GetPin(Uart1TxLedPort, Uart1TxLedBit)); // Wait until finished transmitting
	return(Index);
}

dword GetTick(void)
{
	dword lastTick = tick;
	while (lastTick != tick)
		lastTick = tick;
	return lastTick;
}

boolean IsMoving(void)
{
	return GetPin(port_IsMoving, pin_IsMoving);
}

// *** DCell related methods

// Sends the array toDCell to DCell
void DCellTransmit(void)
{
	dcell_putbytes(toDCell, toDCellLength);
	while(GetOutPin(Uart2TxLedPort, Uart2TxLedBit) == 1)                   // Wait for whole line to be Tx'd
	;
	toDCellTime = GetTick();
	waitingForDCell = 1;
}

// Reads bytes from DCell until either the buffer is empty or a complete packet has been received, stores in fromDCell
void DCellReadChar(void)
{
	if (!fromDCellReady)
	{
		word lastCharacter = 65535;
		do
		{
			word lastCharacter = dcell_getc();
			if (lastCharacter < 256)
			{
				fromDCell[fromDCellIndex++] = lastCharacter;
				if (fromDCellIndex == fromDCellLength)
				{
					fromDCellReady = 1;
					fromDCellIndex = 0;
				}
				if (fromDCellIndex == 2)
				{
					switch (lastCharacter)
					{
					case 0x03:
						fromDCellLength = 9;
						break;
					case 0x10:
						fromDCellLength = 8;
						break;
					case 0x83:
						fromDCellLength = 5;
						break;
					case 0x90:
						fromDCellLength = 5;
						break;
					default:
						DoEStop(ERR_NO_COMMS, avrErrDCell);
						break;
					}
				}
			}
		} while (lastCharacter < 256 && !fromDCellReady);
		if (fromDCellReady)
		{
			fromDCellTime = GetTick();
			waitingForDCell = 0;
		}
		else if (waitingForDCell && GetTick() - toDCellTime > DCELL_TIMEOUT)
		{
			if (logging)
				robot_puts("# DCell timeout\n");
			DoEStop(ERR_NO_COMMS, avrErrDCell);
			waitingForDCell = 0;
		}
	}
}

// Sets the checksum of a packet stored in toDCell
void DCellSetChecksum(void)
{
	word u16CRC = 0xFFFF;
	for (byte i = 0; i < toDCellLength - 2; i++)
	{
		u16CRC ^= toDCell[i];
		for (byte j = 0; j < 8; j++)
		if (u16CRC & 1)
		u16CRC = (u16CRC >> 1) ^ 0xA001;
		else
		u16CRC = (u16CRC >> 1);
	}
	toDCell[toDCellLength - 2] = (u16CRC & 0xFF);
	toDCell[toDCellLength - 1] = (u16CRC >> 8);
}

// Verifies the checksum of a packet stored in fromDCell (returns 1 if valid and 0 if invalid)
byte DCellVerifyChecksum(void)
{
	word u16CRC = 0xFFFF;
	for (byte i = 0; i < fromDCellLength - 2; i++)
	{
		u16CRC ^= fromDCell[i];
		for (byte j = 0; j < 8; j++)
			if (u16CRC & 1)
				u16CRC = (u16CRC >> 1) ^ 0xA001;
			else
				u16CRC = (u16CRC >> 1);
	}
	return (fromDCell[fromDCellLength - 2] == (u16CRC & 0xFF) && fromDCell[fromDCellLength - 1] == (u16CRC >> 8));
}

// Reads a complete packet from DCell, stores in fromDCell - BLOCKING
void DCellReadPacket(void)
{
	while (waitingForDCell)
		DCellListen();
}

// Sends a request to read a specified register - BLOCKING
void DCellReadCommand(word startRegister)
{
	toDCell[0] = STATION_NUMBER;
	toDCell[1] = 3;
	toDCell[2] = startRegister >> 8;
	toDCell[3] = startRegister & 0xFF;
	toDCell[4] = 0;
	toDCell[5] = 2;
	toDCellLength = 8;
	DCellSetChecksum();
	DCellTransmit();
	DCellReadPacket();
}

// Sends a request to write to a specified register - BLOCKING
void DCellWriteCommand(word startRegister, word lowerRegister, word upperRegister)
{
	toDCell[0] = STATION_NUMBER;
	toDCell[1] = 0x10;
	toDCell[2] = startRegister >> 8;
	toDCell[3] = startRegister & 0xFF;
	toDCell[4] = 0;
	toDCell[5] = 2;
	toDCell[6] = 4;
	toDCell[7] = lowerRegister >> 8;
	toDCell[8] = lowerRegister & 0xFF;
	toDCell[9] = upperRegister >> 8;
	toDCell[10] = upperRegister & 0xFF;
	toDCellLength = 13;
	DCellSetChecksum();
	DCellTransmit();
	DCellReadPacket();
}

// Removes unexpected data from buffers
void DCellFlush(void)
{
	Uart2FlushRxBuffer();
}

// Creates the packet necessary to request a force reading - this only needs to be done once
void CreateForcePacket(void)
{
	toDCell[0] = STATION_NUMBER;
	toDCell[1] = 3;
	toDCell[2] = 0;
	toDCell[3] = MD_CRAW;
	toDCell[4] = 0;
	toDCell[5] = 2;
	toDCellLength = 8;
	DCellSetChecksum();
	for (byte i = 0; i < 8; i++)
		forcePacket[i] = toDCell[i];
}

// Requests a force reading - BLOCKING
void DCellGetForce(void)
{
	for (byte i = 0; i < 8; i++)
		toDCell[i] = forcePacket[i];
	toDCellLength = 8;
	DCellTransmit();
	DCellReadPacket();
}

// Requests the station number - BLOCKING
void DCellGetStationNumber(void)
{
	DCellReadCommand(MD_STN);
}

// Read from DCell and react as necessary
void DCellListen(void)
{
	DCellReadChar();
	if (fromDCellReady)
	{
		fromDCellReady = 0;
		if (DCellVerifyChecksum())
		{
			if (readSample)
			{
				readSample = 0;
				ConvertForceToInt();
				if (currentForce > maxForce || currentForce < minForce)
					DoEStop(ERR_LIMIT_EXCEEDED, avrErrForce);
				else if ((forceDeltaAbs && (abs(currentForce) - abs(lastForce) > maxForceDelta || abs(currentForce) - abs(lastForce) < minForceDelta)) || (!forceDeltaAbs && (currentForce - lastForce > maxForceDelta || currentForce - lastForce < minForceDelta)))
					DoEStop(ERR_LIMIT_EXCEEDED, avrErrForceDelta);
				else if (!isHoming)
					RobotForceData();
			}
		}
		else
			DoEStop(ERR_NO_COMMS, avrErrDCell);
	}
}

// Turns into a passthrough for communicating with DCell
void DCellPassthrough(void)
{
	waitingForRobot = 0;
	if (logging)
		robot_puts("# In DCellPassthrough\n");
	word lastCharacter = 65535;
	byte flag = 1;
	while(flag)
	{
		byte i = 0;
		while (i < toDCellLength)
		{
			do
			{
				lastCharacter = robot_getc();
			} while (lastCharacter > 255);
			toDCell[i] = lastCharacter;
			if (i == 1)
			{
				switch (lastCharacter)
				{
				case 0x03:
					toDCellLength = 8;
					break;
				case 0x10:
					toDCellLength = 13;
					break;
				case 0x2A:
					flag = 0;
					toDCellLength = 5;
					break;
				case 0x83:
					toDCellLength = 5;
					break;
				case 0x90:
					toDCellLength = 5;
					break;
				default:
					DoEStop(ERR_NO_COMMS, avrErrAvr);
					break;
				}
			}
			i++;
		}
		dcell_putbytes(toDCell, toDCellLength);
		while(GetOutPin(Uart2TxLedPort, Uart2TxLedBit) == 1);
		if (flag)
		{
			DCellReadPacket();
			robot_putbytes(fromDCell, fromDCellLength);
		}
	}
}

// *** Unit conversions

long ConvertDMMtoSteps(long inValue)
{
	return inValue * STEPS_PER_DMM;
}

long ConvertStepstoDMM(char* inString)
{
	conValue = atol(inString);
	conValue = conValue / STEPS_PER_DMM;
	return conValue;
}

long ConvertMMtoSteps(long inValue)
{
	return inValue * STEPS_PER_MM;
}

long ConvertStepstoMM(char* inString)
{
	conValue = atol(inString);
	conValue = conValue / STEPS_PER_MM;
	return conValue;
}

// Converts the force reading received from DCell from float to int
// This is necessary as we need to act on the force as quickly as possible
void ConvertForceToInt(void)
{
	// packet    [5]      [6]     [3]     [4]
	// force  SEEEEEEE EMMMMMMM MMMMMMM MMMMMMM
	lastForce = currentForce;
	currentForce = fromDCell[6]; // 00000000 EMMMMMMM
	currentForce = ((currentForce << 8) | fromDCell[3]) & 0x7FFE; // 0MMMMMMM MMMMMMM0
	currentForce = currentForce >> 1; // 00MMMMMM MMMMMMMM
	currentForce = currentForce | 0x4000; // 01MMMMMM MMMMMMMM - this number is actual mantissa << 13
	
	word exponent = fromDCell[5]; // 00000000 SEEEEEEE
	exponent = ((exponent << 8) | fromDCell[6]) & 0x7F80; // 0EEEEEEE E0000000
	exponent = exponent >> 7; // 00000000 EEEEEEEE - number of times to << (first E is inverted sign bit); should be <= 13
	if (exponent & 0x80) // if the exponent is positive
	{
		exponent = exponent & 0x7F; // remove sign bit
		if (exponent > 13)
			currentForce = 32767; // exponent is too large, overflow
		else if (exponent < 13) // if exponent == 13, do nothing
		{
			currentForce = currentForce >> (12 - exponent); // valid exponent
			if (currentForce & 1) // if fractional part >= 0.5
				currentForce++; // round up
			currentForce = currentForce >> 1;
		}
	}
	else
		currentForce = 0; // exponent is negative, force is -1N < 0 < 1N (also valid)
	if (fromDCell[5] & 0x80) // if the number is negative
		currentForce = -currentForce; // negate
}

// *** CNC related methods

// Sends a line to Cnc
void CncTransmit(char* line)
{
	if (logging)
	{		
		robot_puts("# Sent ");
		robot_puts(line);
	}
	cnc_puts(line);
	toCncTime = GetTick();
	waitingForCnc = 1;
}

// Reads bytes from Cnc until either the buffer is empty or a complete packet has been received, stores in fromCnc
void CncReadChar(void)
{
	if (!fromCncReady)
	{
		word lastCharacter = 65535;
		do 
		{
			word lastCharacter = cnc_getc();
			if (lastCharacter < 256)
			{
				if (logging)
					robot_putc(lastCharacter);
				fromCnc[fromCncIndex++] = lastCharacter;
				if (lastCharacter == cncEoL)
				{
					fromCnc[fromCncIndex] = 0;
					fromCncReady = 1;
					fromCncIndex = 0;
				}
			}
		} while (lastCharacter < 256 && !fromCncReady);
		if (fromCncReady)
		{
			fromCncTime = GetTick();
			waitingForCnc = 0;
		}
		else if (waitingForCnc && GetTick() - toCncTime > CNC_TIMEOUT)
		{
			if (logging)
				robot_puts("# Cnc timeout\n");
			DoEStop(ERR_NO_COMMS, avrErrCnc);
			waitingForCnc = 0;
		}
	}
}

// Takes a command and parameter and sends to Cnc; sets the expected response to ensure reply is valid
void CncSend(char cmd, long parameter)
{
	toCnc[0] = cmd;
	toCnc[1] = 'X';
	ltoa(parameter, &toCnc[2], 10);
	strcat(toCnc, cncEoLString);
	CncTransmit(toCnc);
	if (cmd < 'a')
		expectedRsp = cmd;
	else
		expectedRsp = cmd - 32;
}

// Reads a complete packet from Cnc, stores in fromCnc - BLOCKING
void CncReadLine(void)
{
	while (waitingForCnc)
		CncListen();
}

// Removes unexpected data from buffers
void CncFlush()
{
	for (int i = 0; i < 3; i++)
	{
		CncTransmit(cncEoLString);
		_delay_ms(2);
	}
	Uart1FlushRxBuffer();
}

// Sends the appropriate commands to Cnc; sets the expected response to ensure reply is valid
void CncInit(void)
{
	CncTransmit(cncInitCmd);
	expectedRsp = cmdInit;
}

void CncDone(void)
{
	CncTransmit(cncDoneCmd);
	expectedRsp = cmdDone;
}

void CncSaveParams(void)
{
	CncTransmit(cncSaveParamsCmd);
	expectedRsp = cmdSaveParams;
}

void CncSetEStop(void)
{
	CncTransmit(cncSetEStopCmd);
	expectedRsp = cmdGetEStop;
}

void CncClearEStop(void)
{
	CncTransmit(cncClearEStopCmd);
	expectedRsp = cmdGetEStop;
}

void CncGoTo(long newPosition)
{
	CncSend(cmdGoTo, newPosition);
}

void CncRefHome(void)
{
	CncTransmit(cncRefHomeCmd);
	expectedRsp = cmdIsRefHomed;
}

void CncSetTopSpeed(long newTopSpeed)
{
	CncSend(cmdSetTopSpeed, newTopSpeed);
}

void CncSetSpeed(long newSpeed)
{
	CncSend(cmdSetSpeed, newSpeed);
}

void CncSetHomeSpeed(long newHomeSpeed)
{
	CncSend(cmdSetHomeSpeed, newHomeSpeed);
}

void CncSetAccel(long newAcceleration)
{
	CncSend(cmdSetAccel, newAcceleration);
}

void CncSetDecel(long newDeceleration)
{
	CncSend(cmdSetDecel, newDeceleration);
}

void CncSetLimitMin(long newLimitMin)
{
	CncSend(cmdSetLimitMin, newLimitMin);
}

void CncSetLimitMax(long newLimitMax)
{
	CncSend(cmdSetLimitMax, newLimitMax);
}

void CncSetPulseW(long newPulseW)
{
	CncSend(cmdSetPulseW, newPulseW);
}

void CncSetDirInvert(long newDirInvert)
{
	CncSend(cmdSetDirInvert, newDirInvert);
}

void CncSetEnInvert(long newEnInvert)
{
	CncSend(cmdSetEnInvert, newEnInvert);
}

void CncSetHomeIsPlus(long newHomeIsPlus)
{
	CncSend(cmdSetHomeIsPlus, newHomeIsPlus);
}

void CncSetStepsPerM(long newStepsPerM)
{
	CncSend(cmdSetStepsPerM, newStepsPerM);
}

void CncReposition(long newPosition)
{
	CncSend(cmdReposition, newPosition);
}

void CncSetEnable(long newEnable)
{
	CncSend(cmdSetEnable, newEnable);
}

void CncSimLimitSw(long newSimLimitSw)
{
	CncSend(cmdSimLimitSw, newSimLimitSw);
}

void CncGetTargetPos(void)
{
	CncTransmit(cncGetTargetPosCmd);
}

void CncGetHomeState(void)
{
	CncTransmit(cncGetHomeStateCmd);
	expectedRsp = cmdGetHomeState;
}

void CncGetTopSpeed(void)
{
	CncTransmit(cncGetTopSpeedCmd);
	expectedRsp = cmdGetTopSpeed;
}

void CncGetSpeed(void)
{
	CncTransmit(cncGetSpeedCmd);
	expectedRsp = cmdGetSpeed;
}

void CncGetHomeSpeed(void)
{
	CncTransmit(cncGetHomeSpeedCmd);
	expectedRsp = cmdGetHomeSpeed;
}

void CncGetAccel(void)
{
	CncTransmit(cncGetAccelCmd);
	expectedRsp = cmdGetAccel;
}

void CncGetDecel(void)
{
	CncTransmit(cncGetDecelCmd);
	expectedRsp = cmdGetDecel;
}

void CncGetPosMin(void)
{
	CncTransmit(cncGetPosMinCmd);
	expectedRsp = cmdGetPosMin;
}

void CncGetPosMax(void)
{
	CncTransmit(cncGetPosMaxCmd);
	expectedRsp = cmdGetPosMax;
}

void CncGetPulseW(void)
{
	CncTransmit(cncGetPulseWCmd);
	expectedRsp = cmdGetPulseW;
}

void CncGetBaseFreq(void)
{
	CncTransmit(cncGetBaseFreqCmd);
	expectedRsp = cmdGetBaseFreq;
}

void CncGetStepsPerX(void)
{
	CncTransmit(cncGetStepsPerXCmd);
	expectedRsp = cmdGetStepsPerX;
}

void CncGetDirInvert(void)
{
	CncTransmit(cncGetDirInvertCmd);
	expectedRsp = cmdGetDirInvert;
}

void CncGetEnInvert(void)
{
	CncTransmit(cncGetEnInvertCmd);
	expectedRsp = cmdGetEnInvert;
}

void CncGetEStop(void)
{
	CncTransmit(cncGetEStopCmd);
	expectedRsp = cmdGetEStop;
}

void CncGetEnable(void)
{
	CncTransmit(cncGetEnableCmd);
	expectedRsp = cmdGetEnable;
}

void CncGetFault(void)
{
	CncTransmit(cncGetFaultCmd);
	expectedRsp = cmdGetFault;
}

void CncGetAccelMax(void)
{
	CncTransmit(cncGetAccelMaxCmd);
	expectedRsp = cmdGetAccelMax;
}

void CncGetSpeedMax(void)
{
	CncTransmit(cncGetSpeedMaxCmd);
	expectedRsp = cmdGetSpeedMax;
}

void CncIsRefHomed(void)
{
	CncTransmit(cncIsRefHomedCmd);
	expectedRsp = cmdIsRefHomed;
}

// Read from Cnc and react as necessary
void CncListen(void)
{
	CncReadChar();
	if (fromCncReady)
	{
		if (fromCnc[0] != expectedRsp)
		{
			switch (fromCnc[0])
			{
			case cmdGetEStop:
				motorEstop = atol(&fromCnc[2]);
				if (motorEstop && currentTask != avrInit)
				{
					estop = motorEstop;
					GetErrorEstop();
				}
				break;
			case cncErrConstrained:
				break;
			case cncErrNone:
				break;
			case cncErrAxis:
				DoEStop(ERR_UNKNOWN, avrErrCnc);
				break;
			case cncErrParameter:
				ThrowError(ERR_PARAMETER, currentTask);
				break;
			case cncErrCommand:
				DoEStop(ERR_UNKNOWN, avrErrCnc);
				break;
			case cncErrEeprom:
				DoEStop(ERR_HARDWARE_FAILURE, avrErrCnc);
				break;
			case cncErrHardware:
				DoEStop(ERR_HARDWARE_FAULT, avrErrCnc);
				break;
			case cncErrState:
				ThrowError(ERR_STATE, currentTask);
				break;
			case cncErrComms:
				DoEStop(ERR_NO_COMMS, avrErrCnc);
				break;
			default:
				fromCncReady = 0;
				break;	
			}
		}
	}
}

// Turns into a passthrough for communicating with Cnc
void CncPassthrough(void)
{
	waitingForRobot = 0;
	if (logging)
		robot_puts("# In CncPassthrough\n");
	word lastCharacter = 65535;
	byte flag = 5;
	while(flag)
	{
		lastCharacter = cnc_getc();
		if (lastCharacter < 256)
			robot_putc(lastCharacter);
		lastCharacter = robot_getc();
		if (lastCharacter < 256)
		{
			cnc_putc(lastCharacter);
			if (lastCharacter == '*')
				flag--;
			else
				flag = 5;
		}
	}
	currentTask = ' ';
	cnc_putc(cncEoL);
	_delay_ms(2);
	CncFlush();
}

// *** AVR-rest of robot interaction methods

// Sends a line to robot
void RobotTransmit(char* line)
{
	robot_puts(line);
}

// Reads bytes from robot until either the buffer is empty or a complete packet has been received, stores in fromRobot
void RobotReadChar(void)
{
	if (!fromRobotReady)
	{
		word lastCharacter = 65535;
		do 
		{
			lastCharacter = robot_getc();
			if (lastCharacter < 256)
			{
				robotWatchdog = ROBOT_TIMEOUT;
				fromRobot[fromRobotIndex++] = lastCharacter;
				if (lastCharacter == avrEoL)
				{
					fromRobot[fromRobotIndex] = 0;
					fromRobotReady = 1;
					fromRobotIndex = 0;
				}
			}
		} while (lastCharacter < 256 && !fromRobotReady);
		if (robotWatchdogState)
		{
			robotWatchdogState = 0;
			if (logging)
				robot_puts("# Robot timeout\n");
			DoSafeRefHome(ERR_NO_COMMS, avrErrAvr);
			waitingForRobot = 0;
		}
	}
}

// Takes a command and parameter and sends to robot
void RobotSend(char cmd, word parameter)
{
	toRobot[0] = cmd;
	itoa(parameter, &toRobot[1], 10);
	strcat(toRobot, avrEoLString);
	RobotTransmit(toRobot);
}

// Sends force data to robot; this is done repeatedly while probing,
// and these are the only unsolicited messages sent to the robot
void RobotForceData(void)
{
	RobotTransmit(avrDataString);
	itoa(sampleCount, &toRobot[0], 10);
	RobotTransmit(toRobot);
	RobotTransmit(avrDataSeparatorString);
	itoa(currentForce, &toRobot[0], 10);
	RobotTransmit(toRobot);
	RobotTransmit(avrEoLString);
}

// Performs the requested tasks
void Init(void)
{
	if (currentStep == 0 || fromCncReady)
	{
		fromCncReady = 0;
		switch (currentStep)
		{
		case 0:
			// initialise variables
			LFDcount = LFDtolerance;
			sampleCount = 0;
			errorNum = 0;
			CreateForcePacket();
			
			// check communications with DCell
			DCellGetStationNumber();
			
			// check communications with Cnc
			CncFlush();
			CncInit();
			break;
		case 1:
			// start synchronising parameters with Cnc
			CncGetHomeState();
			break;
		case 2:
			homeState = atol(&fromCnc[2]);
			CncGetTopSpeed();
			break;
		case 3:
			topSpeed = ConvertStepstoMM(&fromCnc[2]);
			CncGetSpeed();
			break;
		case 4:
			speed = ConvertStepstoMM(&fromCnc[2]);
			CncGetHomeSpeed();
			break;
		case 5:
			homeSpeed = ConvertStepstoMM(&fromCnc[2]);
			CncGetAccel();
			break;
		case 6:
			acceleration = ConvertStepstoMM(&fromCnc[2]);
			CncGetDecel();
			break;
		case 7:
			deceleration = ConvertStepstoMM(&fromCnc[2]);
			CncGetPosMin();
			break;
		case 8:
			posMin = ConvertStepstoDMM(&fromCnc[2]);
			CncGetPosMax();
			break;
		case 9:
			posMax = ConvertStepstoDMM(&fromCnc[2]);
			CncGetStepsPerX();
			break;
		case 10:
			stepsPerX = ConvertStepstoDMM(&fromCnc[2]);
			CncGetFault();
			break;
		case 11:
			motorFault = atol(&fromCnc[2]);
			CncGetEnable();
			break;
		case 12:
			motorEnable = atol(&fromCnc[2]);
			CncGetAccelMax();
			break;
		case 13:
			accMax = ConvertStepstoMM(&fromCnc[2]);
			CncGetSpeedMax();
			break;
		case 14:
			speedMax = ConvertStepstoMM(&fromCnc[2]);
			CncIsRefHomed();
			break;
		case 15:
			isRefHomed = atol(&fromCnc[2]);
			if (isRefHomed == 1)
				probeState = 0;
			CncGetEStop();
			break;
		case 16:
			motorEstop = atol(&fromCnc[2]);
			estop = motorEstop;
			RobotSend(avrInit, 1);
			currentTask = ' ';
			break;
		}
		currentStep++;
	}
}

void Done(void)
{
	currentTask = ' ';
}

void Save(void)
{
	if (currentStep == 0)
		CncSaveParams();
	else
	{
		RobotSend(avrSave, 1);
		currentTask = ' ';
	}
}

void Log(void)
{
	logging = 1 - logging;
	RobotSend(avrLog, logging);
	currentTask = ' ';
}

void DoProbe(byte newProbeDir)
{
	if (!isProbing)
	{
		isHoming = 0;
		startedMoving = 0;
		isProbing = 1;
		probeState = 1;
		DCellGetForce();
		ConvertForceToInt();
		lastForce = currentForce;
		CncGetTargetPos();
		sampleCount = ConvertStepstoDMM(&fromCnc[2]) - groundLevel;
		if (newProbeDir != 0)
			newProbeDir = 1;
		probeDir = newProbeDir;
		if (probeDir)
		{
			sampleCount -= stepsPerX;
			CncSend(cmdGoTo, ConvertDMMtoSteps(groundLevel + maxDepth));
		}
		else
		{
			sampleCount += stepsPerX;
			CncSend(cmdGoTo, ConvertDMMtoSteps(0));
		}
		currentStep = 1;
	}
	else if (startedMoving && !IsMoving())
	{
		startedMoving = 0;
		isProbing = 0;
		probeState = probeDir;
		while (readSample); // wait for last sample to finish being read and sent before confirming end of probing
		RobotSend(avrDoProbe, probeDir);
		currentTask = ' ';
	}
}

void DoRefHome(void)
{
	isHoming = 1;
	if (currentStep == 0 || fromCncReady)
	{
		fromCncReady = 0;
		switch (currentStep)
		{
		case 0:
			CncTransmit(cncRefHomeCmd);
			expectedRsp = cmdIsRefHomed;
			waitingForCnc = 0;
			break;
		case 1:
			CncIsRefHomed();
			break;
		case 2:
			isRefHomed = atol(&fromCnc[2]);
			CncGetHomeState();
			break;
		case 3:
			homeState = atol(&fromCnc[2]);
			isHoming = 0;
			if (isRefHomed == 1)
				probeState = 0;
			RobotSend(avrIsRefHomed, isRefHomed);
			currentTask = ' ';
			break;
		}
		currentStep++;
	}
}

void GetEStop(void)
{
	RobotSend(avrGetEStop, estop);
	currentTask = ' ';
}

void SetEStop(word newEstop)
{
	if (currentStep == 0 || fromCncReady)
	{
		fromCncReady = 0;
		switch (currentStep)
		{
		case 0:
			estop = newEstop;
			if (estop != 0)
			{
				estop = 1;
				DoEStop(ERR_ESTOP, avrSetEStop);
				GetEStop();
			}
			else
			{
				SetPinDir(port_Estop, pin_Estop, dirInput);
				CncClearEStop();
				currentStep++;
			}
			break;
		case 1:
			SetPinDir(port_Estop, pin_Estop, dirInput);
			motorEstop = atol(&fromCnc[2]);
			if (motorEstop)
			{
				estop = 1;
				if (logging)
					robot_puts("# Cnc EStop is still active\n");
			}
			else if (!GetPin(port_Estop, pin_Estop))
			{
				estop = 1;
				if (logging)
					robot_puts("# EStop pin is still active\n");
			}
			GetEStop();
			break;
		}
	}
}

void GetError(void)
{
	toRobot[0] = avrGetError;
	toRobot[1] = errorNum + '0';
	toRobot[2] = errorParam;
	toRobot[3] = 0;
	strcat(toRobot, avrEoLString);
	RobotTransmit(toRobot);
	currentTask = ' ';
}

void GetErrorEstop(void)
{
	if (errorNum > 0)
		GetError();
	else
		GetEStop();
}

void ThrowError(byte newErrorNum, char newErrorParam)
{
	errorNum = newErrorNum;
	errorParam = newErrorParam;
	GetError();
}

void ClearError(void)
{
	ThrowError(ERR_NONE, ' ');
}

void GetParamAvr(void)
{
	switch (currentTask)
	{
		case avrGetGroundLevel:
			RobotSend(avrGetGroundLevel, groundLevel);
			break;
		case avrGetTopSpeed:
			RobotSend(avrGetTopSpeed, topSpeed);
			break;
		case avrGetSpeed:
			RobotSend(avrGetSpeed, speed);
			break;
		case avrGetHomeSpeed:
			RobotSend(avrGetHomeSpeed, homeSpeed);
			break;
		case avrGetAccel:
			RobotSend(avrGetAccel, acceleration);
			break;
		case avrGetDecel:
			RobotSend(avrGetDecel, deceleration);
			break;
		case avrGetProbeDepth:
			RobotSend(avrGetProbeDepth, maxDepth);
			break;
		case avrGetStepsPerX:
			RobotSend(avrGetStepsPerX, stepsPerX);
			break;
		case avrGetLFDTolerance:
			RobotSend(avrGetLFDTolerance, LFDtolerance);
			break;
		case avrGetMaxForce:
			RobotSend(avrGetMaxForce, maxForce);
			break;
		case avrGetMinForce:
			RobotSend(avrGetMinForce, minForce);
			break;
		case avrGetMaxForceDelta:
			RobotSend(avrGetMaxForceDelta, maxForceDelta);
			break;
		case avrGetMinForceDelta:
			RobotSend(avrGetMinForceDelta, minForceDelta);
			break;
		case avrGetForceDeltaAbs:
			RobotSend(avrGetForceDeltaAbs, forceDeltaAbs);
			break;
		case avrGetSafeDisconnect:
			RobotSend(avrGetSafeDisconnect, safeDisconnect);
			break;
		case avrGetLFDState:
			RobotSend(avrGetLFDState, LFDstate);
			break;
		case avrGetProbeState:
			RobotSend(avrGetProbeState, probeState);
			break;
	}
	currentTask = ' ';
}

void GetParamCnc(void)
{
	switch (currentStep)
	{
	case 0:
		switch (currentTask)
		{
		case avrGetHomeState:
			CncGetHomeState();
			break;			
		case avrGetEnable:
			CncGetEnable();
			break;
		case avrIsRefHomed:
			CncIsRefHomed();
			break;
		}
		currentStep++;
		break;
	case 1:
		if (fromCncReady)
		{
			fromCncReady = 0;
			switch (currentTask)
			{
			case avrGetHomeState:
				homeState = atol(&fromCnc[2]);
				RobotSend(avrGetHomeState, homeState);
				break;
			case avrGetEnable:
				motorEnable = atol(&fromCnc[2]);
				RobotSend(avrGetEnable, motorEnable);
				break;
			case avrIsRefHomed:
				isRefHomed = atol(&fromCnc[2]);
				RobotSend(avrIsRefHomed, isRefHomed);
				break;
			}
			currentTask = ' ';
			break;
		}
	}
}

void SetParamAvr(int16_t newParam)
{
	switch (currentTask)
	{		
	case avrSetGroundLevel:
		if (maxDepth + newParam < posMin || maxDepth + newParam > posMax)
			ThrowError(ERR_PARAMETER, avrSetGroundLevel);
		else
		{
			groundLevel = newParam;
			RobotSend(avrGetGroundLevel, groundLevel);
		}
		break;
	case avrSetProbeDepth:
		if (newParam + groundLevel < posMin || newParam + groundLevel > posMax)
			ThrowError(ERR_PARAMETER, avrSetProbeDepth);
		else
		{
			maxDepth = newParam;
			RobotSend(avrGetProbeDepth, maxDepth);
		}
		break;
	case avrSetLFDTolerance:
		if (newParam > TOLER_MAX || newParam < TOLER_MIN)
			ThrowError(ERR_PARAMETER, avrSetLFDTolerance);
		else
		{
			LFDtolerance = newParam;
			LFDcount = LFDtolerance;
			RobotSend(avrGetLFDTolerance, LFDtolerance);
		}
		break;
	case avrSetMaxForce:
		if (newParam > FORCE_MAX || newParam < minForce)
			ThrowError(ERR_PARAMETER, avrSetMaxForce);
		else
		{
			maxForce = newParam;
			RobotSend(avrGetMaxForce, maxForce);
		}
		break;
	case avrSetMinForce:
		if (newParam > maxForce || newParam < FORCE_MIN)
			ThrowError(ERR_PARAMETER, avrSetMinForce);
		else
		{
			minForce = newParam;
			RobotSend(avrGetMinForce, minForce);
		}
		break;
	case avrSetMaxForceDelta:
		if (newParam > FORCE_MAX || newParam < minForce)
			ThrowError(ERR_PARAMETER, avrSetMaxForceDelta);
		else
		{
			maxForceDelta = newParam;
			RobotSend(avrGetMaxForceDelta, maxForceDelta);
		}
		break;
	case avrSetMinForceDelta:
		if (newParam > maxForce || newParam < FORCE_MIN)
			ThrowError(ERR_PARAMETER, avrSetMinForceDelta);
		else
		{
			minForceDelta = newParam;
			RobotSend(avrGetMinForceDelta, minForceDelta);
		}
		break;
	case avrSetForceDeltaAbs:
		if (newParam != 0)
			newParam = 1;
		forceDeltaAbs = newParam;
		RobotSend(avrGetForceDeltaAbs, forceDeltaAbs);
		break;
	case avrSetSafeDisconnect:
		if (newParam != 0)
			newParam = 1;
		safeDisconnect = newParam;
		RobotSend(avrGetSafeDisconnect, safeDisconnect);
		break;
	}
	currentTask = ' ';
}

void SetParamCnc(word newParam)
{
	switch (currentStep)
	{
	case 0:
		switch (currentTask)
		{
		case avrSetTopSpeed:
			if (newParam > speedMax || newParam < 0)
				ThrowError(ERR_PARAMETER, avrSetTopSpeed);
			else
				CncSetTopSpeed(ConvertMMtoSteps(newParam));
			break;
		case avrSetSpeed:
			if (newParam > topSpeed || newParam < 0)
				ThrowError(ERR_PARAMETER, avrSetSpeed);
			else
				CncSetSpeed(ConvertMMtoSteps(newParam));
			break;
		case avrSetHomeSpeed:
			if (newParam > topSpeed || newParam < 0)
				ThrowError(ERR_PARAMETER, avrSetHomeSpeed);
			else
				CncSetHomeSpeed(ConvertMMtoSteps(newParam));
			break;
		case avrSetAccel:
			if (newParam > accMax || newParam < 0)
				ThrowError(ERR_PARAMETER, avrSetAccel);
			else
				CncSetAccel(ConvertMMtoSteps(newParam));
			break;
		case avrSetDecel:
			if (newParam > accMax || newParam < 0)
				ThrowError(ERR_PARAMETER, avrSetDecel);
			else
				CncSetDecel(ConvertMMtoSteps(newParam));
			break;
		case avrSetStepsPerX:
			if (newParam > STEPS_MAX || newParam < STEPS_MIN)
				ThrowError(ERR_PARAMETER, avrSetStepsPerX);
			else
				CncSetStepsPerM(ConvertDMMtoSteps(newParam));
			break;
		case avrSetEnable:
			if (newParam != 0)
				newParam = 1;
			CncSetEnable(newParam);
			break;
		}
		currentStep++;
		break;
	case 1:
		if (fromCncReady)
		{
			fromCncReady = 0;
			switch (currentTask)
			{
			case avrSetTopSpeed:
				topSpeed = newParam;
				RobotSend(avrGetTopSpeed, topSpeed);
				break;
			case avrSetSpeed:
				speed = newParam;
				RobotSend(avrGetSpeed, speed);
				break;
			case avrSetHomeSpeed:
				homeSpeed = newParam;
				RobotSend(avrGetHomeSpeed, homeSpeed);
				break;
			case avrSetAccel:
				acceleration = newParam;
				RobotSend(avrGetAccel, acceleration);
				break;
			case avrSetDecel:
				deceleration = newParam;
				RobotSend(avrGetDecel, deceleration);
				break;
			case avrSetStepsPerX:
				stepsPerX = newParam;
				RobotSend(avrGetStepsPerX, stepsPerX);
				break;
			case avrSetEnable:
				if (newParam != 0)
					newParam = 1;
				motorEnable = newParam;
				RobotSend(avrGetEnable, motorEnable);
				break;
			}
			currentTask = ' ';
		}
	}
}

void GetForce(void)
{
	if (IsMoving())
	{
		int tempForce = currentForce;
		while (tempForce != currentForce)
			tempForce = currentForce;
		RobotSend(avrGetForce, tempForce);
	}
	else
	{
		DCellGetForce();
		_delay_ms(2);
		DCellGetForce();
		ConvertForceToInt();
		RobotSend(avrGetForce, currentForce);
	}
	currentTask = ' ';
}

// Read from robot and react as necessary
void RobotListen(void)
{
	RobotReadChar();
	if (fromRobotReady)
	{
		fromRobotReady = 0;
		if (currentTask == avrNone || fromRobot[0] == avrSetEStop || fromRobot[0] == avrNone)
		{
			if (fromRobot[0] == avrNone)
				RobotTransmit(avrPing);
			else if (logging)
			{
				robot_puts("# Setting task to ");
				robot_putc(fromRobot[0]);
				robot_putc(avrEoL);
			}
			currentTask = fromRobot[0];
			currentParameter = atol(&fromRobot[1]);
			currentStep = 0;
		}
		else
			ThrowError(ERR_BUSY, fromRobot[0]);
	}
	switch(currentTask)
	{
	case avrNone:
		currentStep = 0;
		fromCncReady = 0;
		break;
	case avrInit:
		Init();
		break;
	case avrSetEStop:
		SetEStop(currentParameter);
		break;
	case avrGetEStop:
		GetEStop();
		break;
	case avrSetError:
		ClearError();
		break;
	case avrGetError:
		GetError();
		break;
	case avrSetEnable:
		SetParamCnc(currentParameter);
		break;
	case avrGetEnable:
		GetParamCnc();
		break;
	case avrCncPassthrough:
		CncPassthrough();
		break;
	case avrDCellPassthrough:
		DCellPassthrough();
		break;
	default:
		if (errorNum || estop)
			GetErrorEstop();
		break;
	}
	if ((errorNum == 0 && estop == 0 && !IsMoving()) || currentStep != 0)
	{
		switch (currentTask)
		{
		case avrNone:
		case avrInit:
		case avrSetEStop:
		case avrGetEStop:
		case avrSetError:
		case avrGetError:
		case avrSetEnable:
		case avrGetEnable:
		case avrCncPassthrough:
		case avrDCellPassthrough:
			break;
		case avrDone:
			Done();
			break;
		case avrSave:
			Save();
			break;
		case avrLog:
			Log();
			break;
		case avrDoProbe:
			DoProbe(currentParameter);
			break;
		case avrDoRefHome:
			DoRefHome();
			break;
		case avrSetGroundLevel:
		case avrSetProbeDepth:
		case avrSetLFDTolerance:
		case avrSetMaxForce:
		case avrSetMinForce:
		case avrSetMaxForceDelta:
		case avrSetMinForceDelta:
		case avrSetForceDeltaAbs:
		case avrSetSafeDisconnect:
			SetParamAvr(currentParameter);
			break;
		case avrSetTopSpeed:
		case avrSetSpeed:
		case avrSetHomeSpeed:
		case avrSetAccel:
		case avrSetDecel:
		case avrSetStepsPerX:
		case avrSetHomeState:
			SetParamCnc(currentParameter);
			break;
		case avrGetGroundLevel:
		case avrGetTopSpeed:
		case avrGetSpeed:
		case avrGetHomeSpeed:
		case avrGetAccel:
		case avrGetDecel:
		case avrGetProbeDepth:
		case avrGetStepsPerX:
		case avrGetLFDTolerance:
		case avrGetMaxForce:
		case avrGetMinForce:
		case avrGetMaxForceDelta:
		case avrGetMinForceDelta:
		case avrGetForceDeltaAbs:
		case avrGetSafeDisconnect:
		case avrGetLFDState:
		case avrGetProbeState:
			GetParamAvr();
			break;
		case avrGetHomeState:
		case avrIsRefHomed:
			GetParamCnc();
			break;
		case avrGetForce:
			GetForce();
			break;
		default:
			ThrowError(ERR_UNRECOGNISED_INSTRUCTION, currentTask);
			break;
		}
	}
}

// *** Internal operations

void DoSafeRefHome(byte newErrorNum, char newErrorParam)
{
	if (safeDisconnect)
		DoRefHome();
	DoEStop(newErrorNum, newErrorParam);
}

void DoEStop(byte newErrorNum, char newErrorParam)
{
	errorNum = newErrorNum;
	errorParam = newErrorParam;
	estop = 1;
	SetPinDir(port_Estop, pin_Estop, dirOutput);
	if (logging)
		robot_puts("# EStop thrown\n");
}

void ISRInit(void)
{
	EICRA = (1 << ISC11) | (0 << ISC10) | (1 << ISC01) | (1 << ISC00); // setup INT0 (DoSample) for rising edge and INT1 (Estop) for falling edge
	EIMSK = (1 << INT0) | (1 << INT1);
}

void TimerInit(void)
{
	OCR1A = T1_COUNT - 1;
	TCCR1A = T1_MODE_DISABLE;
	TCCR1B = T1_PRESCALE;
	TIMSK1 = (1 << TOIE1);
}

ISR(ISR_Tick)
{
	tick++;
	if (robotWatchdog > 0)
	{
		robotWatchdog--;
		if (robotWatchdog == 0)
			robotWatchdogState = 1;
	}
	if (!GetPin(port_LFD, pin_LFD))
	{
		if (LFDcount > 0)
			LFDcount--;
	}
	else
	{
		LFDcount = LFDtolerance;
		LFDstate = 0;
	}
}

ISR(ISR_DoSample)
{
	if (currentTask != avrCncPassthrough)
	{
		startedMoving = 1;
		if (probeDir)
			sampleCount += stepsPerX;
		else
			sampleCount -= stepsPerX;
		if (readSample)
		{
			if (logging)
				robot_puts("# DCell slow sample\n");
			DoEStop(ERR_NO_COMMS, avrErrDCell);
		}
		else
		{
			readSample = 1;
			for (byte i = 0; i < 8; i++)
				toDCell[i] = forcePacket[i];
			toDCellLength = 8;
			DCellTransmit();
		}
	}
}

ISR(ISR_Estop)
{
	estop = 1;
	newestop = 1;
	isHoming = 0;
	isProbing = 0;
	waitingForRobot = 0;
	waitingForCnc = 0;
	waitingForDCell = 0;
	if (logging)
		robot_puts("# ISR Estop thrown\n");
}

int main(void)
{
	#define BAUD 57600
	#include <util/setbaud.h>                        // Pre-calculate the baudrate register values
	Uart0SetBaudrate(UBRR_VALUE, USE_2X);
	Uart1SetBaudrate(UBRR_VALUE, USE_2X);            // Set the baudrate
	Uart2SetBaudrate(UBRR_VALUE, USE_2X);
	uart0_init();
	uart1_init();
	uart2_init();
	Uart0SetFormat(umoAsync, udb8, upaNone, ust1);
	Uart1SetFormat(umoAsync, udb8, upaNone, ust1);   // Asynchronous mode, 8 data bits, no parity, 1 stop bit
	Uart2SetFormat(umoAsync, udb8, upaNone, ust1);
	Uart0FlushRxBuffer();
	Uart1FlushRxBuffer();
	Uart2FlushRxBuffer();
	SetPinDir(DEBUG_PORT, DEBUG_PIN, dirOutput);
	SetPinDir(port_Estop, pin_Estop, dirInput);
	SetPin(port_Estop, pin_Estop, Lo);
	SetPinDir(port_LFD, pin_LFD, dirInput);
	SetPinPullUp(port_LFD, pin_LFD, swOn);
	ISRInit();
	TimerInit();
	sei();
	
	SetPin(DEBUG_PORT, DEBUG_PIN, 0);
	if (logging)
		robot_puts("# Hello!\n");
	while(1)
	{
		if (LFDtolerance > 0 && LFDcount == 0 && LFDstate == 0)
		{
			LFDstate = 1;
			DoEStop(ERR_LIMIT_EXCEEDED, avrErrLFD);
		}
		if (newestop)
		{
			newestop = 0;
			if (errorNum == 0)
			{
				CncGetFault();
				CncReadLine();
				motorFault = atol(&fromCnc[2]);
				if (motorFault)
				{
					errorNum = ERR_HARDWARE_FAULT;
					errorParam = avrErrMotor;
				}
				else
				{
					CncGetHomeState();
					CncReadLine();
					homeState = atol(&fromCnc[2]);
					switch (homeState)
					{
					case cncHsMinLimit:
					case cncHsMaxLimit:
						errorNum = ERR_LIMIT_EXCEEDED;
						errorParam = avrErrMotor;
						break;
					default:
						errorNum = ERR_ESTOP;
						errorParam = avrSetEStop;
						break;
					}
				}
			}
			GetErrorEstop();
		}
		RobotListen();
		CncListen();
		DCellListen();
	}
}
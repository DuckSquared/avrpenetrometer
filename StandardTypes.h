/**********************************************************************************/
/*                                                                                */
/*     Standard types and "normalized" port names for various Atmel AVR MCU's     */
/*   Keep this header file WinAVR assembler compatible! (no Macros or typedefs)   */
/*                                                                                */
/**********************************************************************************/ 

#ifndef _AVR_STANDARD_TYPES_H
#define _AVR_STANDARD_TYPES_H 1

// General purpose types

#define nil       NULL                        // A stupid C-ism

#define shortint  int8_t                      // A sensible name for (-128..127)
#define pshortint shortint*
#define byte      unsigned char               // A char is not a byte!
#define pbyte     byte*
#define word      uint16_t                    // 16-bit unsigned integer
#define pword     word*
#define dword     uint32_t                    // 32-bit unsigned integer
#define pdword    dword*
#define qword     uint64_t                    // 64-bit unsigned integer
#define pqword    qword*

// Special ASCII characters

#define asciiNul  0
#define asciiXon  17
#define asciiXoff 19
#define asciiDel  127

#endif // _AVR_STANDARD_TYPES_H

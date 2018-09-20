#ifndef ASCII_CTRL_H
  #define ASCII_CTRL_H

// ASCII codes 

#define asNul   0     // Null
#define asSoH   1     // Start of Heading
#define asSTx   2     // Start of Text
#define asETx   3     // End of Text
#define asEoT   4     // End of Transmition
#define asEnq   5     // Enquiry
#define asAck   6     // Acknowledge
#define asBel   7     // Bell
#define asBS    8     // Back Space
#define asTab   9     // Horizontal tabulation
#define asLF    10    // Line Feed
#define asVT    11    // Vertical Tabulation
#define asFF    12    // Form Feed
#define asCR    13    // Carriage Return
#define asSO    14    // Shift Out
#define asSI    15    // Shift In
#define asDLE   16    // Data Link Escape
#define asDC1   17    // Device Control 1
#define asDC2   18    // Device Control 2
#define asDC3   19    // Device Control 3
#define asDC4   20    // Device Control 4
#define asNAk   21    // Not Acknowledge
#define asSyn   22    // Synchronous Idle
#define asETB   23    // End of Transmittion Block
#define asCan   24    // Cancel
#define asEM    25    // End of Medium
#define asSub   26    // Substitute character
#define asEsc   27    // Excape seq. introducer
#define asFS    28    // File Separator
#define asGS    29    // Group Separator
#define asRS    30    // Record Separator
#define asUS    31    // Unit Separator
#define asSpace 32    // Space character
#define asDel   127   // Backspace-Delete

#define asXon   asDC1 // Xon
#define asXoff  asDC3 // Xoff
#define asEoF   asSub // End of File

#endif

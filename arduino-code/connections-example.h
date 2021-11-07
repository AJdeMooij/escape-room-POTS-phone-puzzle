/***************************************************************************************
 * Pin connections
 * 
 * You should not have to change this if the wiring is the same.
 * 
 * Only change values; keep names the same
 **************************************************************************************/
// Columns
#define KEYBOARD_INPUT_A 2
#define KEYBOARD_INPUT_B 3
#define KEYBOARD_INPUT_C 4

// Rows
#define KEYBOARD_INPUT_D A0
#define KEYBOARD_INPUT_E A1

#ifdef ESCAPE_ROOM_TIME
  #define KEYBOARD_INPUT_F A2
  #define KEYBOARD_INPUT_G A3
#endif

#ifdef PORTABLE_GAME
  #define KEYBOARD_INPUT_F A3
  #define KEYBOARD_INPUT_G A2
#endif

// MP3 player
#define mp3TX 5
#define mp3RX 6

// Hook
#define hook 7

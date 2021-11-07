/* 
 * Different wiring schemes exist for different versions of the phone.  
 * Exactly one of the two following needs to be uncommented
 */
#define ESCAPE_ROOM_TIME // Uncomment this line for the wiring of the phone that is not portable
// #define PORTABLE_GAME // Uncomment this line for the portable phone wiring

/*
 * Comment the following line when ready
 * for production. Makes the code run faster!
 */
#define DEBUG

/***************************************************************************************
 * Constant values changing the behavior of the phone system
 **************************************************************************************/
// How long is sound played after button press (milliseconds)
const int min_button_playback_duration = 300;   

// How long do we pause the entire system after sending command to MP3 player?
// (Increase this value if starting/stopping MP3 files sometimes does not register)
const int delay_after_mp3_send = 30;

// Playback volume of sound files
int volume = 15;

// Time to wait after registring button press (avoids registring one button press twice)
const int button_debounce_time = 10;

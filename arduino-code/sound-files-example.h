/***************************************************************************************
 * MP3 Files
 * 
 * If you replace files, but keep the numbering the same, you should not have to 
 * change anything here. 
 * If not, take into account other parts of the code may have to be updated as well.
 * 
 * The first 10 files should represent the DTMF tones for the buttons 0 to 9
 **************************************************************************************/
// Helper sounds
#define connection_lost 11          // 425Hz sound/no-sound switching every .5 seconds
#define ringing_tone 12             // 425Hz sound/no-sound switching every second
#define dial_tone 13                // Constant 425Hz tone, used to signal ready to dial
#define system_ready_signal 14      // Constant LOUD 440Hz tone

// Voice messages that can be triggered
#define number_unknown 15           // Computerized phone central operator saying number is unknown
#define em911 16                    // Fake 911 operator (English) (TODO: Replace with voice recording)
#define em112 17                    // Fake 112 operator (Dutch)   (TODO: Replace with voice recording)
#define escape_room_time 18         // Escape Room Time promo
#define puzzle_solved 19            // El Professor message (TODO: Replace with voice recording)
#define almost_solved_pickup 20     // Computerized anonymous crime tip line introduction 
#define please_hold_msg 21          // A message asking to please hold
#define hold_music_original 22      // La Casa de Papel Original Sound Track
#define hold_music_instrumental 23  // La Casa de Papel Instrumental Sound Track
#define update_volume_robot 24      // A robot voice giving instructions on how to update the volume
#define volume_updated 25           // A robot voice notifying the user of the updated volume

// In advert directory (use player.playAdvertisement(<file>)
#define please_hold_msg_advert 1   // All workers busy message (same file as above)

// In the case the waiting music, interrupted by "all workers busy" message is played,
// these values determine the behavior
int red_herring_mp3_length = 7000;        // The anonymous tipline mp3 (almost solved msg) is 7 seconds long
int please_hold_msg_length = 2000;        // This mp3 file is two seconds long
int waiting_music_interrupt_time = 20000; // After every how many milliseconds to play
                                          // all workers busy track

/**
 * These values are related to updating the volume after starting the phone
 */
// Length of sound files used in changing volume
const int spoken_number_length = 1500;
const int volume_updated_length = 1000;

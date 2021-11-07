/**
 * Library to control an old phone for an escape room puzzle.
 * 
 * It required an Arduino to be wired to the keyboard of the phone and to a
 * DFPlayer Mini MP3 player module that itself is connected to the speaker
 * in the horn of the phone.
 * 
 * The code allows specifying different phone numbers and associate them
 * with MP3 files to play if dialed.
 * 
 * 
 * Created by Jan de Mooij and Charlotte de Gooijer, Oktober 2021
 *    github.com/AJdeMooij / github.com/charlotte923
 *  
 * License:   Mozilla Public License 2.0
 * 
 */
#include "SoftwareSerial.h"
#include <DFPlayerMini_Fast.h>
#include "variables.h"
#include "sound-files.h"
#include "connections.h"

// Signals from detectButton
#define NO_CHANGE 255
#define NO_KEY 254
#define ANY_KEY 253

// The volume is at most two digits (max volume is 30), so we can store both digits
// seperately
int firstDigit = -1;
int secondDigit = -1;

#include "phone-numbers.h"

/***************************************************************************************
 * These lines point to list indices in the above array of phone
 * numbers. You should not have to change these values
 **************************************************************************************/
const int MP3_FILE_INDEX = 11;
const int MP3_DURATION_INDEX = 12;
const int RINGING_DURATION_INDEX = 13;
const int RINGING_DURATION_STDERR_INDEX = 14;

/***************************************************************************************
 * Button layout.
 * 
 * Written for standard 3*4 matrix keyboard. If phone contains buttons A-D in a fourth
 * column (that you want to use) make sure to update this.
 **************************************************************************************/
const byte rows = 4;
const byte cols = 3;

const int rowPins[rows] = {KEYBOARD_INPUT_D, KEYBOARD_INPUT_E, KEYBOARD_INPUT_F, KEYBOARD_INPUT_G};
const int colPins[cols] = {KEYBOARD_INPUT_A, KEYBOARD_INPUT_B, KEYBOARD_INPUT_C};

// Button map, useful for finding what button was pressed
const byte keys[rows][cols] = {
  {1,  2,  3},
  {4,  5,  6},
  {7,  8,  9},
  {'*',0, '#'}
};

// This is where we store the button state
byte matrix[rows][cols];

// This is where we store what number has been dialed so far
int phone_number[MAX_NUMBER_LENGTH];
int phone_index = 0;      // Next digit is written at this index to phone_number
int button_sound_index = 0;     // Keeps track of what button sounds are already played
unsigned long last_play_button_sound_change = -1; // Keeps track of when last button sound was played
bool is_playing_button_sound = false;
bool is_playing_dial_tone = false;

// The phone is programmed as a finite state machine (FSA). These are the states
enum STATE {
  ON_HOOK,                // Horn is placed on device
  DIALING,                // Awaiting for user to enter a complete phone number
  IN_CALL,                // User is listening to voice message being played
  WAITING_DIAL_FINISHED,  // Valid phone number registered but still playing button sounds
  CONNECTION_LOST         // Phone call has ended
};

// Assume we always start with the horn placed on the device
STATE state = ON_HOOK;

/*****************************
 * Allow debug environment
 ****************************/
#ifdef DEBUG
#define debug(x) Serial.print(x);
#define debugln(x) Serial.println(x);
#else
#define debug(x)
#define debugln(x)
#endif

// Initializing the MP3 player
SoftwareSerial mp3Serial(mp3TX, mp3RX); // Our TX goes to MP3s RX, and vice versa
DFPlayerMini_Fast player;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  #ifdef DEBUG
    Serial.begin(115200);
    debugln(F("Starting La Casa de Papel Phone System"));
    debugln(F("Initializing DFPlayer... (May take 3~5 seconds)"));
  #endif

  // We use a non-connected analog pin for our random number generator
  randomSeed(analogRead(A4));

  setupMp3();
  for(int r = 0; r < rows; r++) {
    pinMode(rowPins[r], INPUT_PULLUP);
  }
  for(int c = 0; c < cols; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }
  pinMode(hook, INPUT_PULLUP);

  onSetupComplete();
}

/**
 * Tries to setup the MP3 player, but will fail if there is an
 * error in the wiring. In that case, the system will not continue
 * to boot, and instead show an aggresively blinking led indicating
 * a wiring fault.
 */
void setupMp3() {
  // Different serial connections, debug(ln) can still be 
  // used for writing to Serial monitor
  mp3Serial.begin(9600);
  mp3Serial.listen();

  if(!player.begin(mp3Serial)) {
    debugln(F("Unable to open connection with MP3 Module"));
    debugln(F("Please test connections and check SD card"));
   
    while (true) {
      // Show error light indicating problem with MP3 player
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
}

/**
 * After succesful setup, play a loud tone to indicate system is
 * live and working, and then decrease volume again to operating
 * levels
 */
void onSetupComplete() {
  //Play sound indicating we started and are live
  player.volume(30);
  player.flush();
  delay(500);
  player.play(system_ready_signal);
  player.flush();
  unsigned long start_startup_tone = millis();
  while(digitalRead(hook) == LOW && millis() - start_startup_tone < 1500) {
    // Do nothing
  }
  
  while(player.currentVolume() != volume) {
    player.volume(volume);
    player.flush();
    delay(200);
  }
  
  reset();
  
  // Low led indicates setup is complete
  digitalWrite(LED_BUILTIN, LOW);

  debugln(F("System ready. Let's play!"));
}

/**
 * The loop checks the current state of the phone system,
 * and runs the logic corresponding to that state
 */
void loop()
{
  int hookValue = digitalRead(hook);
  
  if (state != ON_HOOK && hookValue == LOW) {
    
    // User hung up
    reset();
    
  } else if (state == ON_HOOK && hookValue == HIGH) {
    
    // User picked up horn. Start listening for phone number
    debugln(F("Horn picked up"));
    player.play(dial_tone);
    is_playing_dial_tone = true;
    state = DIALING;

  } else if (state == DIALING) {

    // We are waiting for a complete phone number to be entered
    checkForNumberPressed();
    
  } else if (state == CONNECTION_LOST) {
    
    // Play connection lost tone until player hangs up
    player.loop(connection_lost);
    player.flush();
    delay(delay_after_mp3_send);
    delayAllowingHangup(-1);
    
  } else if (state == WAITING_DIAL_FINISHED) {
    
    if (playButtonSound()) {
      delay(300);
      state = IN_CALL;
      int validNumber = isNumberValid();
      debug(F("Finished playing dialing sounds. Calling number "));debugln(validNumber);
      dialNumber(correct_digits[validNumber]);
    }
    
  }
}


/**
 * Check if the user has pressed a new number on the keypad and handles
 * the logic for checking phone numbers if that is the case
 */
void checkForNumberPressed() {
  byte button = detectButton();
  if ( button < NO_KEY ) { // We no longer care about key? Idk
    if ( button < 10 ) {

      if (is_playing_dial_tone) {
        // Stop playing the tone we played when the user picked up the horn
        player.stop();
        player.flush();
        is_playing_dial_tone = false;
        delay(delay_after_mp3_send);
        delay(100);
      }
      
      int buttonNum = (int) button;
      phone_number[phone_index] = button;
      phone_index++;
      
      isNumberValid();
      
    } else {
      // TODO, button >= 10?
      debug(F("User managed to press a button not on the keypad: "));
      debugln(button);
    }
  }

  playButtonSound();
}


/**
 * Plays the next button sound in the queue, or cancels playing the previous button sound
 * if the required time has elapsed.
 * 
 * Returns: true if the currently dialed phone number is a valid number and all sounds for
 *          the digits in that phone number have finished playing
 */
bool playButtonSound() {
    if (!is_playing_button_sound && phone_number[button_sound_index] >= 0 && button_sound_index <= phone_index && millis() - last_play_button_sound_change > 150) {
      debug(F("Playing sound for button "));debugln(phone_number[button_sound_index]);
      player.play(phone_number[button_sound_index]);
      player.flush();
      delay(delay_after_mp3_send);
      last_play_button_sound_change = millis();
      button_sound_index++;
      is_playing_button_sound = true;
    } else if (is_playing_button_sound && millis() - last_play_button_sound_change > min_button_playback_duration) {
      player.stop();
      player.flush();
      delay(delay_after_mp3_send);
      last_play_button_sound_change = millis();
      is_playing_button_sound = false;
    }
    
    return state == WAITING_DIAL_FINISHED && !is_playing_button_sound && button_sound_index == phone_index;
}

/**
 * Check if the currently dialed sequence is a valid phone number,
 * i.e., if the sequence is present in the list of valid phone numbers
 * 
 * Returns the index of a valid phone number in the list of all phone
 * numbers, or -1 if no valid number has been detected
 */
int isNumberValid() {
  #ifdef DEBUG
  printPhoneNumber();
  #endif
  
  for(int i = 0; i < N_NUMBERS; i++) {
    if( 
        correct_digits[i][0] == phone_index && 
        comparePhoneNumber(correct_digits[i], correct_digits[i][0])
    ) {
      debug(F("Found a number to call: "))debugln(i);
      state = WAITING_DIAL_FINISHED;
      return i;
    }
  }
  
  return -1;
}


/**
 * Compares the currently dialed number against a specific phone number
 * in the list of all valid phone numbers
 * 
 * int correct_number[]:  A specific array from the list of all valid 
 *                        numbers
 * int arr_size:          The size of the actual phone number within
 *                        the passed array
 *                        
 * returns:               True if the actually dialed number corresponds
 *                        to the phone number in the passed array
 */
bool comparePhoneNumber(long correct_number[], long arr_size) {
  for(int i = 0; i < arr_size; i++) {
    if(correct_number[i+1] != phone_number[i] && correct_number[i+1] != ANY_KEY) {
      return false;
    }
  }
  return true;
}


/**
 * Starts dialing the given phone number.
 * This method first plays the dialing sound for a given
 * period of time, then starts playing the MP3 file associated
 * with this number, and finally switches the state to CONNECTION_LOST
 * 
 * correct_number_array[]:  A specific array from the list of all valid
 *                          numbers representing the phone number to
 *                          dial.
 */
void dialNumber(long correct_number_array[]) {
  long mp3ToPlay = correct_number_array[MP3_FILE_INDEX];

  if ( mp3ToPlay == almost_solved_pickup ) {
    debugln(F("Switching to state for almost correct puzzle, which is a special case"));
    playRedHerring(correct_number_array);
    return;
  } else if ( mp3ToPlay == update_volume_robot ) {
    debugln(F("Switching to update volume state, which is a special case"));
    enterUpdateVolumeMode(correct_number_array);
    return;
  }
  
  long mp3Duration = correct_number_array[MP3_DURATION_INDEX];
  long actualRingDuration = getRingDuration(correct_number_array);

  // Log some stuff to see what's going on
  debug(F("Playing MP3 file ")); debug(mp3ToPlay); debug(F(" (")); debug(mp3Duration); 
  debug(F("ms) after ringing for ")); debug(actualRingDuration); debugln(F("ms"));
  
  player.loop(ringing_tone);
  player.flush();
  delay(100);
  if(!delayAllowingHangup(actualRingDuration)) {
    return;
  }
  debugln(F("Other end picked up!"));
  player.play(mp3ToPlay);
  player.flush();
  delay(50);
  if(!delayAllowingHangup(mp3Duration + 1000)) {
    return;
  }
  debugln(F("Changing state to connection lost"));
  state = CONNECTION_LOST;
}

/**
 * Get a random duration between the bounds specified in the passed array 
 * for the passed phone number to dial.
 * 
 * Returns integer representing milliseconds.
 */
long getRingDuration(long correct_number_array[]) {
  long ringDuration = correct_number_array[RINGING_DURATION_INDEX];
  long ringDurationStd = correct_number_array[RINGING_DURATION_STDERR_INDEX];
  return random(
    ringDuration-ringDurationStd, 
    ringDuration+ringDurationStd
  );
}

/**
 * A method representing the special case where the user is put on hold.
 * This is a special case as the user is first greeted with a message
 * being played only once, and then waiting music is played regularly
 * interrupted with a message to please hold until the user hangs up
 */
void playRedHerring(long correct_number_array[]) {
  long actualRingDuration = getRingDuration(correct_number_array);

  debugln(F("Ringing"));
  
  player.loop(ringing_tone);
  player.flush();
  delay(delay_after_mp3_send);
  if ( !delayAllowingHangup(actualRingDuration) ) {
    return;
  }

  debugln(F("Anonymous tip line picking up"));
  player.play(almost_solved_pickup);
  player.flush();
  delay(delay_after_mp3_send);
  if ( !delayAllowingHangup(red_herring_mp3_length + 1000) ) {
    return;
  }

  player.play(please_hold_msg);
  if(!delayAllowingHangup(please_hold_msg_length + 1000)) {
    return;
  }

  debugln(F("Starting wait music"));
  player.loop(hold_music_original);
  player.flush();
  delay(delay_after_mp3_send);
  
  while ( true ) {
    debug(F("Playing wait music for "));debug(waiting_music_interrupt_time);debugln(F("ms"));
    if (!delayAllowingHangup(waiting_music_interrupt_time)) {
      return;
    }

    if(!playPleaseHold()) {
      return;
    }
  }
}


/**
 * Interrupts the music playing to play the message all workers are
 * still busy.
 * 
 * Requires an MP3 file to already be playing to work.
 * 
 * Returns:   false if horn was hung up and system was reset.
 *            true if we can continue playing the music.
 */
bool playPleaseHold() {
  debugln(F("Please hold.."));
  player.playAdvertisement(please_hold_msg_advert);
  player.flush();
  delay(delay_after_mp3_send);

  if(!delayAllowingHangup(please_hold_msg_length + 1000)) {
    return false;
  }

  debugln(F("Please hold finished"));

  return true;
}

/**
 * If the user dials 0800-VOLUME, they will be able to change the volume
 * using the keyboard input. This method is responsible for playing the
 * spoken instructions, and monitoring for user input.
 * 
 * long correct_number_array[]    Array holding the information for the
 *                                0800-VOLUME phone number, including
 *                                MP3 file to play and its duration
 */
void enterUpdateVolumeMode(long correct_number_array[]) {
  debugln(F("Updating volume"));

  // Reset new volume tracker
  firstDigit = -1;
  secondDigit = -1;

  debugln(F("Playing instructions"));
  player.play(correct_number_array[MP3_FILE_INDEX]);
  player.flush();
  delay(delay_after_mp3_send);
  if(!listenVolumeInput(correct_number_array[MP3_DURATION_INDEX], correct_number_array)) {
    // If user hung up or finished, no need to continue
    return;
  }

  // Tells user current volume level
  int currentVolume = player.currentVolume();
  debug(F("Speaking out loud current volume which is "));debugln(currentVolume);
  player.playFolder(1, currentVolume);
  player.flush();
  delay(delay_after_mp3_send);

  if(!listenVolumeInput(spoken_number_length + 1000, correct_number_array)) {
    // If user hung up or finished, no need to continue
    return;
  }

  // If the user has not entered a new volume during instructions playing, we will
  // wait indefinitely for them to enter a new volume, or to hang up
  listenVolumeInput(-1, correct_number_array);
}

/**
 * Method that monitors for both hanging up the phone, and for user input
 * corresponding to setting a new volume. This method will also update
 * the volume as soon as a valid volume has been entered according to
 * the instructions
 * 
 * int duration                   Number of milliseconds to delay
 * long correct_number_array[]    Array holding the information for the
 *                                0800-VOLUME phone number, including
 *                                MP3 file to play and its duration
 * returns true iff the calling method should continue, and false if not.
 *  The method returns false in one of two cases: Either the user has
 *  entered a new (valid) volume level, or they have replaced the horn on
 *  the hook.
 */
bool listenVolumeInput(int duration, long correct_number_array[]) {
  unsigned long start_time = millis();
  debug(F("Delaying for "));debug(duration);debugln(F(" milliseconds while monitoring for new volume instructions"));
  while (!digitalRead(hook) == LOW && millis() - start_time < duration) {
    byte key = detectButton();
    if ((int) key < 10) {
      if (firstDigit < 0) {
        firstDigit = (int) key;
        debug(F("First digit of new volume is "));debugln(firstDigit);
      } else if (secondDigit < 0) {
        secondDigit = (int) key;
        debug(F("Second digit of new volume is "));debugln(secondDigit);
      }
    } else if (key == '#') {
      debugln(F("Registered hashtag (#)"));
      player.stop();
      player.flush();
      delay(delay_after_mp3_send);

      int volumeBackup;
      
      if(firstDigit > 0) {
        if (secondDigit >= 0) {
          volume = firstDigit * 10 + secondDigit;
        } else {
          volume = firstDigit;
        }
        debug(F("Volume updated to "));debugln(volume);
      } else {
        // Replay instructions message, because clearly they did not
        // listen and pressed the hashtag before entering new number
        volume = volumeBackup;
        enterUpdateVolumeMode(correct_number_array);
        return;
      }

      // Check valid input range
      if (volume < 1 || volume > 30) {
        debugln(F("Volume not in valid range"));
        volume = volumeBackup; // reset volume
        enterUpdateVolumeMode(correct_number_array); // replay instructions
        return false;
      }

      // Serial connection is very buggy, keep trying to update
      // volume until registered
      while(player.currentVolume() != volume) {
        debugln(F("Trying to adjust volume"));
        player.volume(volume);
        player.flush();
        delay(200);
      }
      
      debug(F("New volume of "));debug(volume);debugln(F(" as entered by user registered with MP3 player as"));debugln(player.currentVolume());

      // Tell user volume has been updated succesfully
      player.play(volume_updated);
      player.flush();
      delay(delay_after_mp3_send);
      if(!delayAllowingHangup(volume_updated_length + 1000)) {
        return false;
      }

      // Notify new volume value (same as entered)      
      player.playFolder(1, volume);
      player.flush();
      delay(delay_after_mp3_send);
      if(!delayAllowingHangup(spoken_number_length + 1000)) {
        return false;
      }

      debugln(F("Done updating volume!"));
      state = CONNECTION_LOST;
      return false;
    }
  }

  // In every loop, check if the user has hung up in the mean time
  if (digitalRead(hook) == LOW) {
    reset();
    return false;
  } else {
    return true;
  }
}


/**
 * Detects if a button on the numpad was pressed.
 * 
 * Returns:   The byte character corresponding to
 *            the pressed button if a new button was
 *            pressed, NO_CHANGE constant if the state
 *            of the numpad has not changed since the
 *            last check, or NO_KEY constant if 
 *            in the last state a button was pressed
 *            that is now no longer pressed
 */
byte detectButton() {
  byte key = NO_CHANGE;

  for(int r = 0; r < rows; r++) {
    for(int c = 0; c < cols; c++) {
      byte state = digitalRead(rowPins[r]) == LOW && digitalRead(colPins[c]) == LOW;
      if (state != matrix[r][c]) {
        key = state ? keys[r][c] : NO_KEY;
      }
      matrix[r][c] = state;
    }
  }

  if (key != NO_CHANGE) {
    delay(button_debounce_time);
  }

  return key;
}

/** Allows delaying for the specified number of milliseconds, while
 *  still checking if the user has hung up to reset the puzzle.
 *  Useful for delaying while playing audio, as hanging up can
 *  stop the audio from being played.
 *  
 *  This method behaves the same as delay(milliseconds), except
 *  that if the user hangs up, the system state is reset instead
 *  of the delay being completely blocking.
 *  
 *  unsigned long milliseconds:   The number of milliseconds to
 *                                delay.
 */
bool delayAllowingHangup(unsigned long milliseconds) {
  unsigned long start_delay = millis();
  while (milliseconds < 0 || millis() - start_delay < milliseconds) {
    if(digitalRead(hook) == LOW) {
      debugln(F("Found phone on hook while delaying allowing hangup"));
      reset();
      return false;
    }
  }
  return true;
}

/**
 * Resets the puzzle state to the default
 * 
 * Every time a user hangs up, we effectively go back to the beginning state. 
 */
void reset() {
    debugln(F("Horn hung up"));
    player.stop();
    player.flush();
    is_playing_dial_tone = false;
    delay(delay_after_mp3_send);
    state = ON_HOOK;
    for(int i = 0; i < MAX_NUMBER_LENGTH; i++) {
      phone_number[i] = -1;
    }
    phone_index = 0;
    last_play_button_sound_change = 0;
    button_sound_index = 0;
    detectButton();
}

#ifdef DEBUG
/**
 * Prints the sequence of characters that has been dialed so far.
 * 
 * This method is only available in debug mode
 */
void printPhoneNumber() {
  debug(F("Dialed so far: ")); 
  for(int i = 0; i < 10; i++) {
    if(phone_number[i] < 0) {
      break;
    }
    debug(phone_number[i]);
  }
  debugln("");
}

/**
 * Prints the current button state of the numpad,
 * with a 0 representing button depressed, and 1 
 * representing button pressed.
 * 
 * Useful for debugging hanging keys, but you may
 * want to pause (using delay) the system so you can 
 * interpret the results.
 * 
 * This method is only available in debug mode
 */
void printButtonState() {
  for(int r = 0; r < rows; r++) {
    for(int c = 0; c < cols; c++) {
      debug(matrix[r][c]);debug(F(" "));
    }
    debugln(F(""));
  }
}
#endif

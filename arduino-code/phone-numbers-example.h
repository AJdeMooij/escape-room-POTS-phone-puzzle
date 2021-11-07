/***************************************************************************************
 * ACTUAL PHONE NUMBERS THAT ARE PART OF THE PUZZLE.
 * 
 * Add a phone number to which the puzzle should respond to one of the following arrays, 
 * depending on how many digits it has.
 * 
 * The first to eleventh elements contain the digits of the phone number itself, right 
 * padded with -1 if the phone number is shorter than 10 digits.
 * 
 * Use ANY_KEY to represent any button being acceptable. Make sure if this overlaps 
 * with other phone numbers, to put it after those phone numbers in the array.
 * 
 * The items after that have to be populated as follows:
 *  - 11: Index (1-based) of the MP3 file to play
 *  - 12: Duration (milliseconds) of the MP3 file to play
 *  - 13: Time (milliseconds) to wait before the phone is picked up on the other side
 *  - 14: Standard deviation (time +/- std) of the ringing time 
 *        (used for randomized ringing times)
 **************************************************************************************/
const int MAX_NUMBER_LENGTH = 10; // Update this value if numbers become longer/shorter
const int N_NUMBERS = 7;          // Update this value if you add/remove a phone number

const long correct_digits[N_NUMBERS][MAX_NUMBER_LENGTH + 5] = {
  // MP3 file "em112" is 17 seconds long, ring between .6 and .8 seconds
  {3, 1,1,2, -1,-1,-1,-1,-1,-1,-1, em112,17000,700,100},
  
  // MP3 file "em911" is 18 seconds long, ring between .6 and .8 seconds
  {3, 9,1,1, -1,-1,-1,-1,-1,-1,-1, em911,18000,700,100},

  // MP3 file "escape_room_time" is exactly 1 minute long, ring for 5+/-1 seconds
  {10, 0,3,0,2,2,7,3,1,9,3, escape_room_time,60000,5000,1000},
  
  // MP3 file "puzzle_solved" is 17 seconds long; ring for 5 seconds
  {10, 0,7,1,1,1,3,5,6,4,2, puzzle_solved,9000,5000,0}, 
  
  // MP3 file "puzzle_solved" is 10 seconds long; ring for between 3 and 8 seconds
  // This file is played if the second-to-last digit is guessed incorrectly
  {10, 0,7,1,1,1,3,5,6,ANY_KEY,2, almost_solved_pickup,10000,5000,2000},  // WARNING: If you change the file name,
                                                                          // you will have to update the references to this
                                                                          // file name in arduino-code.ino file as well

  // Allows to set a new volume level (between 1 and 30)
  {10, 0,8,0,0,8,6,5,8,6,3, update_volume_robot, 12000, 0, 0},            // WARNING: If you change the file name,
                                                                          // you will have to update the references to this
                                                                          // file name in arduino-code.ino file as well
  
  // This line ensures that if *any* unknown 10-digit number is dialed, the operator voice message will be played
  {10, ANY_KEY, ANY_KEY, ANY_KEY, ANY_KEY, ANY_KEY, ANY_KEY, ANY_KEY, ANY_KEY, ANY_KEY, ANY_KEY, number_unknown, 4000, 400, 0},
};

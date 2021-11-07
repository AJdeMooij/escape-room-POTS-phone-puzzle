#!/bin/bash

# Obscure phone number that is the puzzle solution
sed -E 's/([0-9],){8}ANY_KEY,[0-9]/0,7,1,1,1,3,5,6,ANY_KEY,2/' arduino-code/phone-numbers.h > arduino-code/phone-numbers-example.h
sed -i -E 's/\{10, ([0-9],){10} puzzle_solved,17000,5000,0\},/\{10, 0,7,1,1,1,3,5,6,4,2, puzzle_solved,9000,5000,0\},/' arduino-code/phone-numbers-example.h

# Reset variablles to defaults
sed -E 's|(//)? *#define ESCAPE_ROOM_TIME|#define ESCAPE_ROOM_TIME|' arduino-code/variables.h > arduino-code/variables-example.h
sed -i -E 's|(//)? *#define PORTABLE_GAME|// #define PORTABLE_GAME|' arduino-code/variables-example.h
sed -i -E 's|(//)? *(#define DEBUG)|\2|' arduino-code/variables-example.h

# Other files do not contain private information
cp arduino-code/sound-files.h arduino-code/sound-files-example.h
cp arduino-code/connections.h arduino-code/connections-example.h

#ifndef ENCODING_H_
#define ENCODING_H_

#include <stdint.h>


// Converts a Morse signal bit pattern into the corresponding character.
// If the pattern is invalid or unrecognized, '?' is returned.
char morse_to_char(uint8_t code);

// Converts a character to the corresponding Morse signal bit pattern.
// If char is invalid or unrecognised, 0 is returned.
uint8_t char_to_morse(char c);

#endif /* ENCODING_H_ */

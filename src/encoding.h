#ifndef ENCODING_H_
#define ENCODING_H_

#include <stdint.h>


// Converts a Morse signal bit pattern into the corresponding character.
// If the pattern is invalid or unrecognized, '?' is returned.
char morse_to_char(uint8_t code);

#endif /* ENCODING_H_ */

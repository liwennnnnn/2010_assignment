/*
 * encoding.c
 *
 * Author: Ryan Wang
 *
 * Functions for converting Morse code representations into characters.
 */


#include <stdint.h>
#include "encoding.h"

char morse_to_char(uint8_t code)
{
	// bits after first "1" encode morse representation,
	// with dots encoded as "0" and dashes as "1"
    switch (code)
	{
        // --- Length 0 ---
        case 0b00000001: return ' '; // 1

        // --- Length 1 ---
        case 0b00000010: return 'E'; // 2
        case 0b00000011: return 'T'; // 3

        // --- Length 2 ---
        case 0b00000100: return 'I'; // 4
        case 0b00000101: return 'A'; // 5
        case 0b00000110: return 'N'; // 6
        case 0b00000111: return 'M'; // 7

        // --- Length 3 ---
        case 0b00001000: return 'S'; // 8
        case 0b00001001: return 'U'; // 9
        case 0b00001010: return 'R'; // 10
        case 0b00001011: return 'W'; // 11
        case 0b00001100: return 'D'; // 12
        case 0b00001101: return 'K'; // 13
        case 0b00001110: return 'G'; // 14
        case 0b00001111: return 'O'; // 15

        // --- Length 4 ---
        case 0b00010000: return 'H'; // 16
        case 0b00010001: return 'V'; // 17
        case 0b00010010: return 'F'; // 18
        case 0b00010100: return 'L'; // 20
        case 0b00010110: return 'P'; // 22
        case 0b00010111: return 'J'; // 23
        case 0b00011000: return 'B'; // 24
        case 0b00011001: return 'X'; // 25
        case 0b00011010: return 'C'; // 26
        case 0b00011011: return 'Y'; // 27
        case 0b00011100: return 'Z'; // 28
        case 0b00011101: return 'Q'; // 29

        // --- Length 5 ---
        case 0b00100000: return '5'; // 32
        case 0b00100001: return '4'; // 33
        case 0b00100011: return '3'; // 35
        case 0b00100111: return '2'; // 39
        case 0b00101111: return '1'; // 47
        case 0b00111111: return '0'; // 63
        case 0b00111110: return '9'; // 62
        case 0b00111100: return '8'; // 60
        case 0b00111000: return '7'; // 56
        case 0b00110000: return '6'; // 48

        // Invalid or unrecognized code
        default: return '?'; 
    }
}

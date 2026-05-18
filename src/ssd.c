/*
 * ssd.c
 */ 

#include <avr/io.h>
#include "ssd.h"
#include <util/delay.h>

/* Seven segment display values */
uint8_t seven_seg[16] = { 
    63,6,91,79,102,109,125,7,127,111,
    119,124,57,94,121,113 };

static uint8_t current_digit = 0;   // 0 = left, 1 = right

/* Display values for ssd_multiplex */
static uint8_t left_side = 0x00;
static uint8_t right_side = 0x00;

void ssd_init(void) {
    // Set port C as output
    DDRC = 0xFF;
    PORTC = 0x00;   // turn off all segments

    /* Configure CC pin as output */
    DDRB |= (1<<PB3);
    PORTB &= ~(1<<PB3);
}

void ssd_display(uint8_t left_digit, uint8_t right_digit, uint8_t dp_on) {
    /* Left digit */
    if (left_digit == SSD_BLANK) {
        left_side = 0x00;
    } else {
        left_side = seven_seg[left_digit & 0x0F];   // mod 16
    }

    /* Right digit */
    if (right_digit == SSD_BLANK) {
        right_side = 0x00;
    } else if (right_digit == SSD_DASH) {
        right_side = SSD_SEG_DASH;
    } else {
        right_side = seven_seg[right_digit & 0x0F]; // mod 16
    }

    if (dp_on) {
        left_side |= SSD_DP_BIT;
    }
}

void ssd_multiplex(void) {
    if (current_digit == 0) {
        /* Show left digit */
        PORTC = 0x00;
        PORTB |= (1<<PB3);
        PORTC = left_side;
        current_digit = 1;
    } else {
        /* Show right digit */
        PORTC = 0x00;
        PORTB &= ~(1<<PB3);
        PORTC = right_side;
        current_digit = 0;
    }
}
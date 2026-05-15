/*
 * buttons.c
 *
 * Handles button input via rising edge detection on PB0, PB1, PB2
 *
 */ 


#include <avr/io.h>
#include "buttons.h"

/* Create state variables for morse emulator */
static uint8_t prev_pinb = 0;
static uint8_t morse_code = 0b00000001; // current char prefix-1 encoding
static uint8_t last_submit = 0; // track consecutive submits

void buttons_init(void) {
    // Make port B pin 0 to pin 2 input (B0-B2)
    DDRB &= ~((1<<PB0) | (1<<PB1) | (1<<PB2));
}

uint8_t buttons_get_rising_edge(void) {
    uint8_t current_pinb = PINB;

    uint8_t rising = current_pinb & ~prev_pinb; // rising edge detection
    prev_pinb = current_pinb; // save for next call

    uint8_t result = 0;

    /* Check each button */
    if (rising & (1<<PB0)) {
        /* DOT created */
        morse_code <<= 1;   // shift left and append 0
        last_submit = 0;
        result |= (1<<PB0);
    }
    if (rising & (1<<PB1)) {
        /* DASH created */
        morse_code = (morse_code<<1) | 1; // shift left and append 1
        result |= (1<<PB1);
    }
    if (rising & (1<<PB2)) {
        /* SUBMIT */
        result |= (1<<PB2);
    }

    return result;
}

uint8_t buttons_get_morse_code(void) {
    return morse_code;
}

void buttons_reset_morse(void) {
    morse_code = 0b00000001;
}

void buttons_clear_state(void) {
    // discard button state so button press does not trigger actions
    prev_pinb = PINB;
    morse_code = 0b00000001;
    last_submit = 0;
}
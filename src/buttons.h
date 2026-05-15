#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>

/* Initialise button in initialise_hardware(), making PB0, PB1, PB2 inputs */ 
void buttons_init(void);

/*
 * Called every iteration in main loop.
 * Detects rising edges on buttons and handle morse input.
 * Returns bitmask of which buttons had rising edge.
 */
uint8_t buttons_get_rising_edge(void);

/* Returns the current morse code value */
uint8_t buttons_get_morse_code(void);

/* Resets the morse code accumulator */
void buttons_reset_morse(void);

/* 
 * Called after exiting the spash screen.
 * Resets button state
 */
void buttons_clear_state(void);


#endif /* BUTTONS_H_ */
/*
 * morse.c
 *
 * Main file
 *
 * Authors: Peter Sutton, Bradley Stone, Ryan Wang
 * Modified by Tan Li Wen, 49662630
 */ 


/* Definitions */
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/* Internal Library Includes */
#include "serialio.h"
#include "terminalio.h"
#include "ledmatrix.h"
#include "display.h"
#include "encoding.h"
#include "buttons.h"


/* Internal Function Declarations */
void initialise_hardware(void);
void start_morse(void);
void start_splash_screen(void);
void handle_inputs(void);


int main(void)
{
    initialise_hardware();
    start_splash_screen();
    start_morse();
}

void initialise_hardware(void)
{
    spi_setup_master(128); // init LED matrix
    // Setup serial port for 19200 baud communication
    init_serial_stdio(19200);
    sei(); // enable global interrupts

    // Initialise buttons
    buttons_init();

    /* LEDs on IO Board */
    // Make port D pin 2 to pin 7 ouput (L0-L5)
    DDRD |= 0b11111100;
    // Make port A pin 2 and 3 output (L6-L7)
    DDRA |= (1<<PA2)|(1<<PA3);
}

void start_splash_screen(void)
{
    // draw sigil on LED matrix
    start_splash_display();
    move_terminal_cursor(10, 6);
    printf("CSSE%d AVR Project", 2010); // change if masters student
    move_terminal_cursor(10, 8);
    printf("\"Morse Code Emulator\"");
    move_terminal_cursor(10, 10);
    printf("%d, Semester %s", 2026, "One");
    move_terminal_cursor(10, 12);
    // "%ld" is "long decimal", since a student number is bigger than 2**16
    printf("By %s (%ld)", "Tan Li Wen", 49662630);
    
    // Wait until a button is pressed
    while(!(PINB & 0x07))
    {
        ; // do nothing til button press
    }

    // Wait until all buttons are released
    while(PINB & 0x07)
    {
        ; // do nothing til button release
    }
    ledmatrix_clear();
    buttons_clear_state();
}

void start_morse(void)
{
    // Clear the serial terminal
    clear_terminal();

    while(1)
    {
        // Handle any button or key inputs
        handle_inputs();
    }
    // should never reach
}

/* Update IO board LEDs */
static uint8_t led_history = 0; // 8-bit shift register

void update_io_leds(uint8_t on) {
    /* Shift led_history left and append new beat */
    led_history = (led_history << 1) | (on ? 1 : 0);

    /* Write to hardware */
    // L0-L5 on PD2-PD7: bits 0-5 of led_history → shift left by 2
    PORTD = (PORTD & 0x03) | ((led_history & 0x3F) << 2);

    // L6-L7 on PA2-PA3: bits 6-7 of led_history → shift right by 4
    PORTA = (PORTA & 0xF3) | ((led_history & 0xC0) >> 4);
}

void handle_inputs(void)
{
    /* ******** START HERE ********
    
    Read the button. Enter a mark if there is a rising edge on b0.
    A way to do this is to check if the previous b0 state is 0,
    and the current b0 state is a 1.
	(You will need to implement a method of tracking the previous b0 state.)
	Ensure that when you press a button to exit the splash screen,
	that this button press doesn't immediately trigger an input here.
    
    --. --- --- -.. / .-.. ..- -.-. -.-
    */
   static uint8_t submit_count = 0;
   static uint8_t new_char = 1;

   uint8_t edge = buttons_get_rising_edge();

   if (edge & (1<<PB0)) {
    /* DOT — 1 beat */
    if (!new_char) {
        /* 1 OFF beat */
        update_io_leds(0);
    }
    /* 1 ON beat */
    update_io_leds(1);
    new_char = 0;
    submit_count = 0; // reset submit counter
   }

   if (edge & (1<<PB1)) {
    /* DASH — 3 beat */
    if (!new_char) {
        /* 1 OFF beat */
        update_io_leds(0);
    }
    /* 3 ON beat */
    update_io_leds(1);
    update_io_leds(1);
    update_io_leds(1);
    new_char = 0;
    submit_count = 0; // reset submit counter
   }

   if (edge & (1<<PB2)) {
    if (submit_count == 0) {
        /* First submit - end of character (3 beat gap) */
        update_io_leds(0);
        update_io_leds(0);
        update_io_leds(0);
        buttons_reset_morse();
        new_char = 1;
        submit_count = 1;
    } else if (submit_count == 1) {
        /* Second submit - end of word (total 5 beat gap) */
        // Add 2 more OFF beats
        update_io_leds(0);
        update_io_leds(0);
        new_char = 1;
        submit_count = 2;
    }
  }
}

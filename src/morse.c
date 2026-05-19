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
#include <ctype.h>

/* Internal Library Includes */
#include "serialio.h"
#include "terminalio.h"
#include "ledmatrix.h"
#include "display.h"
#include "encoding.h"
#include "buttons.h"
#include "timer1.h"
#include "ssd.h"


/* Internal Function Declarations */
void initialise_hardware(void);
void start_morse(void);
void start_splash_screen(void);
void handle_inputs(void);
void update_io_leds(void);
static void process_animation(void);
static void update_ssd(void);
static void handle_sync_mode(void);
static void beat_queue_push(uint8_t value);
static int8_t beat_queue_pop(void);
static void flush_matrix_animation(void);
static void flush_beat_queue(void);

/* Functions to handle inputs */
static void trigger_dot(void);
static void trigger_dash(void);
static void trigger_submit(void);

static uint8_t submit_count = 0;
static uint8_t new_char = 1;
// tracks the number of char currently displayed on the LED matrix
static uint8_t char_displayed = 0;
// track if an incomplete char is shown
static uint8_t has_incomplete = 0;

/* LED history shift register */
static uint8_t led_history = 0; // 8-bit

/* Track number of marks and characters */
static uint8_t mark_count = 0;  // marks in current character
static uint8_t char_count = 0;  // total submitted characters mod 16

/* Track serial terminal output */
static uint8_t terminal_col = 0;    // current column
static uint8_t terminal_row = 1;    // current row

/* LED matrix animation state */
static uint8_t matrix_shifts_remaining = 0;

/* Synchronous mode state */
static uint8_t sync_b0_pressed = 0;
static uint8_t sync_press_ticks = 0;
static uint8_t sync_release_ticks = 0;
static uint8_t sync_submit_count = 0;
static uint8_t sync_pending = 0;

/* Beat queue for IO board animation */
#define BEAT_QUEUE_SIZE 20
static uint8_t beat_queue[BEAT_QUEUE_SIZE];
static uint8_t beat_queue_head = 0;
static uint8_t beat_queue_tail = 0;
static uint8_t beat_queue_count = 0;


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

    // Initialise timer1 for LED animation
    timer1_init();

    // Initialise SSD
    ssd_init();

    sei(); // enable global interrupts

    // Initialise buttons
    buttons_init();

    /* LEDs on IO Board */
    // Make port D pin 2 to pin 7 ouput (L0-L5)
    DDRD |= 0b11111100;
    // Make port A pin 2 and 3 output (L6-L7)
    DDRA |= (1<<PA2)|(1<<PA3);

    /* Ensure LEDs start OFF */
    PORTD &= ~0b11111100;  // Clear PD2-PD7
    PORTA &= ~((1<<PA2)|(1<<PA3));  // Clear PA2-PA3

    /* Synchronous mode */
    // Make port A pin 7 input
    DDRA &= ~(1<<PA7);
    PORTA |= (1<<PA7);
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

    // Reset terminal track
    terminal_col = 0;
    terminal_row = 1;

    while(1)
    {
        /* Determine mode based on S0 (PA7) */
        if (PINA & (1<<PA7)) {
            handle_sync_mode();
        } else {
            // Handle any button or key inputs
            handle_inputs();
        }

        /* Timer 0 */
        if (timer1_fired) {
            timer1_fired = 0;
            process_animation();
        }

        ssd_multiplex();
    }
    // should never reach
}

/* Handle synchronous mode */
static void handle_sync_mode(void) {
    uint8_t current_b0 = (PINB & (1<<PB0)) ? 1 : 0;

    /* Detect press */
    if (current_b0 && !sync_b0_pressed) {
        sync_b0_pressed = 1;
        sync_press_ticks = 0;   // start counting holder time
        sync_release_ticks = 0; // stop counting release time
    }

    /* Detect release */
    if (!current_b0 && sync_b0_pressed) {
        sync_b0_pressed = 0;

        /* Determine input */
        if (sync_press_ticks < 2 ) {
            // 200ms (DOT)
            buttons_encode_dot();
            trigger_dot();
        } else {
            // >= 200ms (DASH)
            buttons_encode_dash();
            trigger_dash();
        }

        sync_release_ticks = 0; // start counting release time
        sync_submit_count = 0;  // reset submit count
        sync_pending = 1;       // watch for submit timeout
    }
}

/* Update IO board LEDs */
void update_io_leds(void) {
    /* Write to hardware */
    // L0-L5 on PD2-PD7: bits 0-5 of led_history → shift left by 2
    PORTD = (PORTD & 0x03) | ((led_history & 0x3F) << 2);

    // L6-L7 on PA2-PA3: bits 6-7 of led_history → shift right by 4
    PORTA = (PORTA & 0xF3) | ((led_history & 0xC0) >> 4);
}

static void beat_queue_push(uint8_t value) {
    if (beat_queue_count < BEAT_QUEUE_SIZE) {
        beat_queue[beat_queue_tail] = value;
        beat_queue_tail = (beat_queue_tail + 1) % BEAT_QUEUE_SIZE;
        beat_queue_count++;
    }
}

static int8_t beat_queue_pop(void) {
    if (beat_queue_count == 0) return -1;   // empty
    uint8_t val = beat_queue[beat_queue_head];
    beat_queue_head = (beat_queue_head + 1) % BEAT_QUEUE_SIZE;
    beat_queue_count--;
    return val;
}

/* Called when timer1 fires (every 100ms)  */
static void process_animation(void) {
    /* Process IO board LED animation */
    if (beat_queue_count > 0) {
        int8_t beat = beat_queue_pop();
        led_history = (led_history << 1) | (beat ? 1 : 0);
        update_io_leds();
    }

    /* Process LED matrix shift animation */
    if (matrix_shifts_remaining > 0) {
        ledmatrix_shift_left(1);
        matrix_shifts_remaining--;

        if (matrix_shifts_remaining == 0) {
            /* Clear the rightmost 3 columns before drawing new character */
            // initialise array with 8 'COLOUR_BLACK'
            uint8_t blank[MATRIX_NUM_ROWS] = {0};

            ledmatrix_update_column(13, blank);
            ledmatrix_update_column(14, blank);
            ledmatrix_update_column(15, blank);
        }
    }

    /* Synchronous mode ticks */
    if (PINA & (1<<PA7)) {
        if (sync_b0_pressed) {
            sync_press_ticks++;
        } else if (sync_pending) {
            sync_release_ticks++;

            /* Check submit */
            if (sync_release_ticks >= 10 && sync_submit_count == 0) {
                /* 1000ms (First SUBMIT) */
                trigger_submit();
                sync_submit_count = 1;

            } else if (sync_release_ticks >= 20 && sync_submit_count == 1) {
                /* 2000ms (Second SUBMIT) */
                trigger_submit();
                sync_submit_count = 2;
                sync_pending = 0;   // stop watching for submit
            }
        }
    }
}

/* Update SSD */
static void update_ssd(void)
{
    uint8_t right, dp;

    if (mark_count == 0) {
        right = SSD_BLANK;
        dp = 0;
    } else if (mark_count > 9) {
        right = SSD_DASH;
        dp = 1;
    } else {
        right = mark_count;
        dp = 1;
    }

    ssd_display(char_count, right, dp);
}

/* Serial terminal output */
static void terminal_print_char(char c)
{
    move_terminal_cursor(terminal_col + 1, terminal_row);
    printf("%c", c);

    terminal_col++;

    /* Wrap */
    if (terminal_col >= 80)
    {
        terminal_col = 0;
        terminal_row++;

        if (terminal_row > 24)
        {
            terminal_row = 1;
        }
    }
}

static void terminal_replace_incomplete(char c)
{
    /* Move back one column to overwrite incomplete charater */
    if (terminal_col > 0)
    {
        terminal_col--;
    }

    move_terminal_cursor(terminal_col + 1, terminal_row);
    printf("%c", c);

    terminal_col++;
}

static void flush_matrix_animation(void) {
    if (matrix_shifts_remaining > 0) {
        ledmatrix_shift_left(matrix_shifts_remaining); // finish instantly
        matrix_shifts_remaining = 0;
        uint8_t blank[MATRIX_NUM_ROWS] = {0};
        ledmatrix_update_column(13, blank);
        ledmatrix_update_column(14, blank);
        ledmatrix_update_column(15, blank);
    }
}

static void flush_beat_queue(void) {
    while (beat_queue_count > 0) {
        int8_t beat = beat_queue_pop();
        led_history = (led_history << 1) | (beat ? 1 : 0);
    }
    update_io_leds();
}

/* Handle DOT */
static void trigger_dot(void) {
    /* Flush animation and beat queue */
    flush_matrix_animation();
    flush_beat_queue();

    if (!new_char) {
        /* 1 OFF beat */
        beat_queue_push(0);
    }
    /* 1 ON beat */
    beat_queue_push(1);

    new_char = 0;
    
    /* Display partial char */
    uint8_t morse_code = buttons_get_morse_code();
    char incomplete_char = morse_to_char(morse_code);
    draw_small_char(incomplete_char, 13, COLOUR_RED);

    submit_count = 0; // reset submit counter

    /* Update SSD */
    mark_count++;
    update_ssd();

    /* Serial terminal output */
    if (has_incomplete)
    {
        terminal_replace_incomplete(incomplete_char);
    } 
    else
    {
        terminal_print_char(incomplete_char);
        has_incomplete = 1;
    }
}

/* Handle DASH */
static void trigger_dash(void) {
    /* Flush animation and beat queue */
    flush_matrix_animation();
    flush_beat_queue();

    if (!new_char) {
        beat_queue_push(0);
    }
    for (int i = 0; i < 3; i++) {
        beat_queue_push(1);
    }
    
    new_char = 0;

    /* Display partial char */
    uint8_t morse_code = buttons_get_morse_code();
    char incomplete_char = morse_to_char(morse_code);
    draw_small_char(incomplete_char, 13, COLOUR_RED);
    
    submit_count = 0; // reset submit counter

    /* Update SSD */
    mark_count++;
    update_ssd();

    /* Serial terminal output */
    if (has_incomplete)
    {
        terminal_replace_incomplete(incomplete_char);
    } 
    else
    {
        terminal_print_char(incomplete_char);
        has_incomplete = 1;
    }
}

/* Handle SUBMIT */
static void trigger_submit(void) {
    if (submit_count == 0) {
        /* First submit - end of character (3 beat gap) */
        for (int i = 0; i < 3; i++) {
            beat_queue_push(0);
        }
        
        /* LED matrix */
        uint8_t morse_code = buttons_get_morse_code();
        char c = morse_to_char(morse_code);

        if (c != '\0') {
            /* Draw new character at right edge */
            draw_small_char(c, 13, COLOUR_GREEN);

            /* Queue 4 left shifts */
            matrix_shifts_remaining = 4;
        }

        /* Update count (cap at 4) */
        if (char_displayed < 4) {
            char_displayed++;
        }
        
        buttons_reset_morse();  // reset morse
        new_char = 1;
        submit_count = 1;
        
        /* Update SSD */
        char_count = (char_count + 1) & 0x0F;   // mod 16
        mark_count = 0; // reset mark_count
        update_ssd();

        /* Serial terminal output */
        has_incomplete = 0;    // reset has_incomplete
    } else if (submit_count == 1) {
        /* Second submit - end of word (total 5 beat gap) */
        // Add 2 more OFF beats
        beat_queue_push(0);
        beat_queue_push(0);
        new_char = 1;
        submit_count = 2;
        update_ssd();

        /* Serial terminal output */
        terminal_print_char(' ');
        has_incomplete = 0;    // reset has_incomplete
    }
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

   if (serial_input_available()) {
        /* Check serial input */
        int ch = fgetc(stdin);   // get serial input

        char c =  toupper((char) ch);
        uint8_t pattern = char_to_morse(c);

        if (pattern != 0)
        {
            /* Flush animation and beat queue */
            flush_matrix_animation();
            flush_beat_queue();

            /* Discard incomplete character */
            buttons_reset_morse();
            mark_count = 0;
            has_incomplete = 0;
            new_char = 1;
            submit_count = 0;

            /* LED matrix */
            draw_small_char(c, 13, COLOUR_YELLOW);
            matrix_shifts_remaining = 4;

            /* Terminal output */
            terminal_print_char(c);

            /* Queue morse pattern on IO board */
            uint8_t p = pattern;
            uint8_t prefix_pos = 0;
            uint8_t temp = p;
            while (temp > 1) {
                prefix_pos++;
                temp >>= 1;
            }

            for (int8_t i = prefix_pos - 1;i >= 0; i--) {
                if (i < prefix_pos - 1) beat_queue_push(0);
                if (p & (1 << i)) {
                    for (int i = 0; i < 3; i++) {
                        beat_queue_push(1);
                    }
                } else {
                    beat_queue_push(1);
                }
            }

            /* 3 OFF beat */
            for (int i = 0; i < 3; i++) {
                beat_queue_push(0);
            }
        }
    }   

   uint8_t edge = buttons_get_rising_edge();

   /* DOT — 1 beat */
   if (edge & (1<<PB0)) trigger_dot();

   /* DASH — 3 beat */
   if (edge & (1<<PB1)) trigger_dash();

   /* SUBMIT */
   if (edge & (1<<PB2)) trigger_submit();
}
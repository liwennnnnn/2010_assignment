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
#include "buzzer.h"
#include "joystick.h"

/* Beat queue */
#define BEAT_QUEUE_SIZE 20

/* Buzzer beat types */
#define BUZZER_OFF          0
#define BUZZER_DOT          1   // DOT frequency (normal duty)
#define BUZZER_DASH         2   // DASH frequency (normal duty)
#define BUZZER_DOT_LAST     3   // DOT frequency (10% duty)
#define BUZZER_DASH_LAST    4   // DASH frequency (10% duty)

/* Buzzer queue */
#define BUZZER_QUEUE_SIZE 20

/* Character storage */
#define MAX_STORED_CHARS 55


/* Internal Function Declarations */
void initialise_hardware(void);
void start_morse(void);
void start_splash_screen(void);
void handle_inputs(void);

/* IO board LEDs */
void update_io_leds(void);
static void process_animation(void);
static void beat_queue_push(uint8_t value);
static int8_t beat_queue_pop(void);
static void flush_matrix_animation(void);
static void flush_beat_queue(void);

/* SSD */
static void update_ssd(void);

/* Synchronous mode */
static void handle_sync_mode(void);

/* Buzzer */
static void buzzer_queue_push(uint8_t type);
static int8_t buzzer_queue_pop(void);
static void flush_buzzer_queue(void);

/* Serial terminal */
void handle_serial_input(void);

/* Font selection */
static uint8_t font_is_large(void);
static uint8_t get_font_width(void);
static uint8_t get_font_shift(void);
static uint8_t get_max_char(void);
static void store_character(char c, uint8_t colour);
static void redraw_chars(void);
static void check_font_change(void);
static uint8_t get_right_edge_col(void);

/* Joystick scroll */
static void redraw_scroll(void);
static int8_t get_scroll_speed(int16_t joystick_x);
static void snap_to_present(void);

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
static uint8_t beat_queue[BEAT_QUEUE_SIZE];
static uint8_t beat_queue_head = 0;
static uint8_t beat_queue_tail = 0;
static uint8_t beat_queue_count = 0;

/* Buzzer queue */
static uint8_t buzzer_queue[BUZZER_QUEUE_SIZE];
static uint8_t buzzer_queue_head = 0;
static uint8_t buzzer_queue_tail = 0;
static uint8_t buzzer_queue_count = 0;

/* Font state */
static uint8_t current_font_large = 0;

/* Character storage */
static char stored_chars[MAX_STORED_CHARS];
static uint8_t stored_colours[MAX_STORED_CHARS];
static uint8_t stored_count = 0;
static char stored_incomplete_char = '\0';

/* Joystick state */
static int16_t scroll_offset = 0;   // 0 = newest char at right edge
static uint8_t is_scrolling = 0;
static int16_t adc_busy = 0;        // 1 = ADC conversion in progress
static uint16_t joystick_x = 512;


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

    // Initialise buzzer
    buzzer_init();

    // Initialise joystick
    joystick_init();

    sei(); // enable global interrupts

    // Initialise buttons
    buttons_init();

    /* LEDs on IO Board */
    // Make port D pin 2 to pin 5 and pin 7 output (L0-L4)
    DDRD |= (1<<PD2) | (1<<PD3) | (1<<PD4) | (1<<PD5) | (1<<PD7);
    // Make port A pin 2 to 4 output (L6-L7)
    DDRA |= (1<<PA2) | (1<<PA3) | (1<<PA4);

    /* Ensure LEDs start OFF */
    PORTD &= ~((1<<PD2) | (1<<PD3) | (1<<PD4) | (1<<PD5) | (1<<PD7));
    PORTA &= ~((1<<PA2) | (1<<PA3) | (1<<PA4));

    /* Synchronous mode */
    // Make port A pin 6 input
    DDRA &= ~(1<<PA6);
    PORTA |= (1<<PA6);

    /* Font selection */
    // Make post A pin 7 input
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
        /* Always handle serial input */
        handle_serial_input(); 

        /* Determine mode based on S0 (PA6) */
        if (PINA & (1<<PA6)) {
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
    check_font_change();

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
    // Preserve PD0(TX), PD1(RX), PD6(buzzer) — mask = 0b01000011 = 0x43
    PORTD = (PORTD & 0x43) | ((led_history & 0x0F) << 2);

    // L4 on PD7: bit 4
    if (led_history & 0x10) {
        PORTD |= (1<<PD7);
    } else {
        PORTD &= ~(1<<PD7);
    }

    // L5 on PA2: bit 5
    if (led_history & 0x20) { PORTA |= (1<<PA2);  }
    else                     { PORTA &= ~(1<<PA2); }

    // L6 on PA3: bit 6
    if (led_history & 0x40) { PORTA |= (1<<PA3);  }
    else                     { PORTA &= ~(1<<PA3); }

    // L7 on PA4: bit 7
    if (led_history & 0x80) { PORTA |= (1<<PA4);  }
    else                     { PORTA &= ~(1<<PA4); }
}

/* Buzzer queue */
static void buzzer_queue_push(uint8_t type) {
    if (buzzer_queue_count < BUZZER_QUEUE_SIZE) {
        buzzer_queue[buzzer_queue_tail] = type;
        buzzer_queue_tail = (buzzer_queue_tail + 1) % BUZZER_QUEUE_SIZE;
        buzzer_queue_count++;
    }
}

static int8_t buzzer_queue_pop(void) {
    if (buzzer_queue_count == 0) return -1;
    uint8_t val = buzzer_queue[buzzer_queue_head];
    buzzer_queue_head = (buzzer_queue_head + 1) % BUZZER_QUEUE_SIZE;
    buzzer_queue_count--;
    return val;
}

/* Beat queue */
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

        int8_t buzzer_type = buzzer_queue_pop();

        /* Check L0 state and buzzer type */
        if (beat && buzzer_type > 0) {
            switch (buzzer_type) {
                case BUZZER_DOT:      buzzer_dot(0);  break;
                case BUZZER_DASH:     buzzer_dash(0); break;
                case BUZZER_DOT_LAST: buzzer_dot(1);  break;
                case BUZZER_DASH_LAST:buzzer_dash(1); break;
                default:              buzzer_stop();  break;
            }
        } else {
            buzzer_stop();
        }
    } else {
        /* No beat in queue */
        buzzer_stop();
    }

    /* Process LED matrix shift animation */
    if (matrix_shifts_remaining > 0) {
        ledmatrix_shift_left(1);
        matrix_shifts_remaining--;

        if (matrix_shifts_remaining == 0) {
            /* Clear the rightmost 3 columns before drawing new character */
            // initialise array with 8 'COLOUR_BLACK'
            uint8_t blank[MATRIX_NUM_ROWS] = {0};

            uint8_t width = get_font_width();
            uint8_t start_col = MATRIX_NUM_COLUMNS - width;

            for (uint8_t i = 0; i < width; i++) {
                ledmatrix_update_column(start_col + i, blank);
            }
        }
    }

    /* Synchronous mode ticks */
    if (PINA & (1<<PA6)) {
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

    /* Joystick scroll */
    if (!adc_busy) {
        joystick_start_conversion(JOYSTICK_X_AXIS);
        adc_busy = 1;
    }

    /* Read result */
    if (adc_busy && joystick_conversion_complete()) {
        joystick_x = joystick_get_result();
        adc_busy = 0;

        int16_t signed_x = (int16_t)joystick_x - 512;

        /* Apply deadzone */
        if (signed_x > -JOYSTICK_DEADZONE && signed_x < JOYSTICK_DEADZONE) {
            signed_x = 0;
        }

        int8_t speed = get_scroll_speed(signed_x);

        /* Tilting RIGHT (positive speed) = scroll into past = increase offset */
        /* Tilting LEFT  (negative speed) = back to present  = decrease offset */
        if (speed != 0) {
            /* Apply scroll */
            scroll_offset += speed;

            /* Restrict from scrolling pass newest char */
            if (scroll_offset < 0) scroll_offset = 0;

            /* Restrict from scrolling pass oldest char */
            // Account for incomplete char occupying one extra slot
            uint8_t total_slots = stored_count + (has_incomplete ? 1 : 0);
            int16_t max_offset = (int16_t)(total_slots * get_font_shift()) - MATRIX_NUM_COLUMNS;
            if (max_offset < 0) max_offset = 0;
            if (scroll_offset > max_offset) scroll_offset = max_offset;

            is_scrolling = (scroll_offset > 0);

            /* Redraw charaters */
            redraw_scroll();
        }
    }
}

static void snap_to_present(void) {
    if (scroll_offset != 0) {
        scroll_offset = 0;
        is_scrolling = 0;
        redraw_scroll();  // redraw at offset 0
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
static void terminal_print_char(char c, const char* colour)
{
    move_terminal_cursor(terminal_col + 1, terminal_row);
    printf("%s%c%s", colour, c, TERM_RESET);

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

static void terminal_replace_incomplete(char c, const char* colour)
{
    /* Move back one column to overwrite incomplete charater */
    if (terminal_col > 0)
    {
        terminal_col--;
    }

    move_terminal_cursor(terminal_col + 1, terminal_row);
    printf("%s%c%s", colour, c , TERM_RESET);

    terminal_col++;
}

/* Flush animation */
static void flush_matrix_animation(void) {
    if (matrix_shifts_remaining > 0) {
        ledmatrix_shift_left(matrix_shifts_remaining); // finish instantly
        matrix_shifts_remaining = 0;

        uint8_t blank[MATRIX_NUM_ROWS] = {0};
        uint8_t width = get_font_width();
        uint8_t start_col = MATRIX_NUM_COLUMNS - width;
        for (uint8_t i = 0; i < width; i++) {
            ledmatrix_update_column(start_col + i, blank);
        }
    }
}

/* Flush buzzer queue */
static void flush_buzzer_queue(void) {
    buzzer_queue_head = 0;
    buzzer_queue_tail = 0;
    buzzer_queue_count = 0;
    buzzer_stop();
}

/* Flush beat queue */
static void flush_beat_queue(void) {
    while (beat_queue_count > 0) {
        int8_t beat = beat_queue_pop();
        led_history = (led_history << 1) | (beat ? 1 : 0);
    }
    update_io_leds();
    flush_buzzer_queue();
}

/* Font selection */
static uint8_t font_is_large(void) {
    return (PINA & (1<<PA7)) ? 1 : 0;
}

static uint8_t get_font_width(void) {
    return current_font_large ? 5 : 3;
}

static uint8_t get_font_shift(void) {
    return current_font_large ? 6 : 4;
}

static uint8_t get_max_char(void) {
    return current_font_large ? 3 : 4;
}

static uint8_t get_right_edge_col(void) {
    return MATRIX_NUM_COLUMNS - get_font_width();
}

/* Character storage */
static void store_character(char c, uint8_t colour) {
    if (stored_count >= MAX_STORED_CHARS) {
        /* Buffer full */
        // Shift everything left, discard oldest char
        for (uint8_t i = 0; i < MAX_STORED_CHARS - 1; i++) {
            stored_chars[i] = stored_chars[i + 1];
            stored_colours[i] = stored_colours[i + 1];
        }
        stored_count = MAX_STORED_CHARS - 1;
    }
    // Add new character
    stored_chars[stored_count] = c;
    stored_colours[stored_count] = colour;
    stored_count++;
}

static void redraw_chars(void) {
    ledmatrix_clear();

    uint8_t width = get_font_width();
    uint8_t shift = get_font_shift();
    uint8_t right_edge = MATRIX_NUM_COLUMNS - width;

    /* Newest completed char sits at right_edge (or one slot left if incomplete) */
    int16_t newest_pos = has_incomplete
                         ? (int16_t)(right_edge - shift)
                         : (int16_t)right_edge;

    for (uint8_t i = 0; i < stored_count; i++) {
        /* Position relative to newest char */
        int16_t pos = newest_pos - (int16_t)((stored_count - 1 - i) * shift);

        /* Only draw chars that are at least partially on screen */
        if (pos + (int16_t)width > 0 && pos < (int16_t)MATRIX_NUM_COLUMNS) {
            draw_char(stored_chars[i], (uint8_t)pos,
                      stored_colours[i], current_font_large);
        }
    }

    if (has_incomplete) {
        draw_char(stored_incomplete_char, right_edge, COLOUR_RED, current_font_large);
    }
}

/* Check for font change */
static void check_font_change(void) {
    uint8_t new_font = font_is_large();
    if (new_font != current_font_large) {
        current_font_large = new_font;

        /* Reset scroll */
        scroll_offset = 0;
        is_scrolling = 0;

        // Sync char_displayed with stored_count
        char_displayed = stored_count;
        
        flush_matrix_animation();

        // Redraw all chars
        redraw_chars();
    }
}

/* Joystick – scroll */
static void redraw_scroll(void) {
    ledmatrix_clear();

    uint8_t width = get_font_width();
    uint8_t shift = get_font_shift();
    uint8_t right_edge = MATRIX_NUM_COLUMNS - width;

    // Shift left if has_incomplete
    int16_t base = (has_incomplete && scroll_offset == 0)
                   ? (int16_t)(right_edge - shift)
                   : (int16_t)right_edge;

    for (uint8_t i = 0; i < stored_count; i++) {
        /* At scroll_offset=0, newest char (i=stored_count-1) is at right edge.
         * Older chars are further left. scroll_offset>0 shifts view into the past. */
        int16_t pos = base
                      - (int16_t)((stored_count - 1 - i) * shift)
                      + scroll_offset;

        if (pos + (int16_t)width > 0 && pos < MATRIX_NUM_COLUMNS) {
            draw_char(stored_chars[i], (uint8_t)pos,
                      stored_colours[i], current_font_large);
        }
        /* Draw incomplete char at righ edge */
        if (has_incomplete && scroll_offset == 0) {
            draw_char(stored_incomplete_char, get_right_edge_col(), COLOUR_RED, current_font_large);
        }
    }
}

static int8_t get_scroll_speed(int16_t joystick_x) {
    if (joystick_x == 0) return 0;

    /* Map magnitude to columns per tick */
    // Divide by 64 to get 1-8 range from 64-512
    int16_t magnitude = joystick_x < 0 ? -joystick_x : joystick_x;
    int8_t speed = (int8_t)(magnitude / 64) + 1;
    if (speed > 8) speed = 8;

    /* Apply direction */
    return joystick_x < 0 ? -speed : speed;
}

/* Handle DOT */
static void trigger_dot(void) {
    snap_to_present();
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
    
    uint8_t morse_code = buttons_get_morse_code();
    char incomplete_char = morse_to_char(morse_code);

    /* Serial terminal output */
    if (has_incomplete)
    {
        terminal_replace_incomplete(incomplete_char, TERM_RED);
    } 
    else
    {
        terminal_print_char(incomplete_char, TERM_RED);
    }

    /* Display partial char */
    // Stored incomplete char
    has_incomplete = 1;
    stored_incomplete_char = incomplete_char;
    redraw_chars();

    submit_count = 0; // reset submit counter

    /* Update SSD */
    mark_count++;
    update_ssd();
}

/* Handle DASH */
static void trigger_dash(void) {
    snap_to_present();
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

    uint8_t morse_code = buttons_get_morse_code();
    char incomplete_char = morse_to_char(morse_code);

    /* Serial terminal output */
    if (has_incomplete)
    {
        terminal_replace_incomplete(incomplete_char, TERM_RED);
    } 
    else
    {
        terminal_print_char(incomplete_char, TERM_RED);
    }

    /* Display partial char */
    // Stored incomplete char
    has_incomplete = 1;
    stored_incomplete_char = incomplete_char;
    redraw_chars();

    submit_count = 0; // reset submit counter

    /* Update SSD */
    mark_count++;
    update_ssd();
}

/* Handle SUBMIT */
static void trigger_submit(void) {
    if (submit_count == 0) {
        snap_to_present();
        /* First submit - end of character (3 beat gap) */
        for (int i = 0; i < 3; i++) {
            beat_queue_push(0);
        }
        
        /* LED matrix */
        uint8_t morse_code = buttons_get_morse_code();
        char c = morse_to_char(morse_code);

        if (c != '\0') {
            /* Draw new character at right edge */
            uint8_t start_col = get_right_edge_col();
            draw_char(c, start_col, COLOUR_GREEN, current_font_large);

            /* Queue left shifts */
            matrix_shifts_remaining = get_font_shift();

            /* Store character */
            store_character(c, COLOUR_GREEN);

            /* Clear stored incomplete char */
            stored_incomplete_char = '\0';

            /* Serial terminal output */
            terminal_replace_incomplete(c, TERM_GREEN);
        }

        /* Update count */
        if (char_displayed < get_max_char()) {
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
        terminal_print_char(' ', TERM_RESET);
        has_incomplete = 0;    // reset has_incomplete
    }
}

void handle_serial_input(void) {
    if (serial_input_available()) {
        /* Check serial input */
        int ch = fgetc(stdin);   // get serial input

        char c =  toupper((char) ch);
        uint8_t pattern = char_to_morse(c);

        if (pattern != 0)
        {
            snap_to_present();
            /* Flush animation, beat queue and buzzer queue */
            flush_matrix_animation();
            flush_beat_queue();

            /* Discard incomplete character */
            buttons_reset_morse();
            mark_count = 0;
            has_incomplete = 0;
            new_char = 1;
            submit_count = 0;

            /* LED matrix */
            uint8_t start_col = get_right_edge_col();
            draw_char(c, start_col, COLOUR_YELLOW, current_font_large);
            matrix_shifts_remaining = get_font_shift();

            /* Store character */
            store_character(c, COLOUR_YELLOW);

            /* Terminal output */
            terminal_print_char(c, TERM_YELLOW);

            /* Update count */
            if (char_displayed < get_max_char()) {
                char_displayed++;
            }

            /* Queue morse pattern on IO board and buzzer */
            uint8_t p = pattern;
            uint8_t prefix_pos = 0;
            uint8_t temp = p;
            while (temp > 1) {
                prefix_pos++;
                temp >>= 1;
            }

            for (int8_t i = prefix_pos - 1;i >= 0; i--) {
                if (i < prefix_pos - 1) {
                    beat_queue_push(0);
                    buzzer_queue_push(BUZZER_OFF);
                }
                
                uint8_t last_mark = (i == 0);

                if (p & (1 << i)) {
                    /* DASH 3 ON beats */
                    for (int x = 0; x < 3; x++) {
                        beat_queue_push(1);
                        buzzer_queue_push(last_mark ? 
                            BUZZER_DASH_LAST : BUZZER_DASH);
                    }
                } else {
                    /* DOT 1 ON beat */
                    beat_queue_push(1);
                    buzzer_queue_push(last_mark ? 
                        BUZZER_DOT_LAST : BUZZER_DOT);
                }
            }

            /* 3 OFF beat */
            for (int i = 0; i < 3; i++) {
                beat_queue_push(0);
                buzzer_queue_push(BUZZER_OFF);
            }
        }
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
   check_font_change();

   uint8_t edge = buttons_get_rising_edge();

   /* DOT — 1 beat */
   if (edge & (1<<PB0)) trigger_dot();

   /* DASH — 3 beat */
   if (edge & (1<<PB1)) trigger_dash();

   /* SUBMIT */
   if (edge & (1<<PB2)) trigger_submit();
}
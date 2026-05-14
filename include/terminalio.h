/*
 * terminalio.h
 *
 * Author: Peter Sutton
 *
 * Functions for interacting with the terminal. These should be used
 * to encapsulate all sending of escape sequences.
 */

#ifndef TERMINAL_IO_H_
#define TERMINAL_IO_H_

#include <stdint.h>
/*
 * x (column number) and y (row number) are measured relative to the top
 * left of the screen. First column is 1, first row is 1.
 *
 * The display parameter is a number between 0 and 47. Valid values are
 *                                Foreground colours    Background colours
 *                                ------------------    ------------------
 *    0 Reset all attributes        30 Black            40 Black
 *    1 Bright                    31 Red                41 Red
 *    2 Dim                        32 Green            42 Green
 *    4 Underscore                33 Yellow            43 Yellow
 *  5 Blink                        34 Blue                44 Blue
 *    7 Reverse Video                35 Magenta            45 Magenta
 *    8 Hidden                    36 Cyan                46 Cyan
 *                                37 White            47 White
 */


typedef enum
    {
        FG_BLACK = 30,
        FG_RED = 31,
        FG_GREEN = 32,
        FG_YELLOW= 33,
        FG_BLUE = 34,
        FG_MAGENTA = 35,
        FG_CYAN = 36,
        FG_WHITE = 37,
        BG_BLACK = 40,
        BG_RED = 41,
        BG_GREEN = 42,
        BG_YELLOW = 43,
        BG_BLUE = 44,
        BG_MAGENTA = 45,
        BG_CYAN = 46,
        BG_WHITE = 47
    } DisplayParameter;


void move_terminal_cursor(int x, int y);
void clear_terminal(void);

#endif /* TERMINAL_IO_H */

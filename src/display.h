/*
 * display.h
 *
 * Author: Ryan Wang
 */ 

#ifndef DISPLAY_H_
#define DISPLAY_H_

/*
 * display a start screen
 */
void start_splash_display(void);

/*
 * draws a small char glyph on the LED matrix starting at x_position (columnn number)
 */
void draw_small_char(char character, uint8_t x_position, uint8_t colour);

/* 
 * draws a large char glyph on the LED matrix starting at x_position (column number)
 */
void draw_large_char(char character, uint8_t x_position, uint8_t colour);

/* draws a char glyph on the LED matrix using current selected font */
void draw_char(char character, uint8_t x_position, 
    uint8_t colour, uint8_t use_large_font);

#endif 

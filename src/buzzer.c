/*
 * buzzer.c
 *
 * Buzzer control using Timer2 Fast PWN
 */ 

#include <avr/io.h>
#include "buzzer.h"

/* // For a given frequency (Hz), return clock period in 1MHz ticks */
static uint16_t freq_to_period(uint16_t freq) {
    return (1000000UL / freq);
}

void buzzer_init(void) {
    // Set PD6 as output (OC2B pin)
    DDRD |= (1<<PD6);

    // Start with output low
    PORTD &= ~(1<<PD6);
    
    // Timer stopped initially
    TCCR2B = 0;
}

static void buzzer_set(uint16_t freq, uint8_t duty_percent) {
    if (freq == 0) {
        buzzer_stop();
        return;
    }

    // Timer2 clock = 8MHz / 8 = 1MHz
    // For Fast PWM with OCR2A as TOP:
    // freq = 1MHz / (OCR2A + 1)
    // OCR2A = 1MHz / freq - 1
    uint16_t period = freq_to_period(freq);
    uint16_t top = period - 1;
    
    // Clamp to 8-bit range
    if (top > 255) top = 255;
    if (top < 1) top = 1;
    
    OCR2A = (uint8_t)top;
    
    // Calculate OCR2B for duty cycle
    // Non-inverting mode: output HIGH from BOTTOM to OCR2B match
    // duty = (OCR2B + 1) / (OCR2A + 1) * 100
    // OCR2B = (duty/100) * (OCR2A + 1) - 1
    
    uint16_t ocr2b = ((uint32_t)duty_percent * (top + 1)) / 100;
    if (ocr2b > 0) ocr2b--;
    if (ocr2b > top) ocr2b = top;
    
    OCR2B = (uint8_t)ocr2b;
    
    // Configure Fast PWM, non-inverting on OC2B, TOP = OCR2A
    TCCR2A = (1<<COM2B1) | (0<<COM2B0) |  // Clear on compare, set at BOTTOM
             (1<<WGM21) | (1<<WGM20);      // Fast PWM, TOP = OCR2A
    
    // Start timer with prescaler 8
    TCCR2B = (1<<WGM22) | (0<<CS22) | (1<<CS21) | (0<<CS20);
}

void buzzer_dot(uint8_t last_mark) {
    buzzer_set(BUZZER_FREQ_DOT, last_mark ? 10 : 50);
}

void buzzer_dash(uint8_t last_mark) {
    buzzer_set(BUZZER_FREQ_DASH, last_mark ? 10 : 50);
}

void buzzer_stop(void) {
    TCCR2B = 0;            // Stop timer
    PORTD &= ~(1<<PD6);    // Output low
}
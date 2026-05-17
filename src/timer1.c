/*
 * timer0.c
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer1.h"

volatile uint8_t timer1_fired = 0;

void timer1_init(void) {
    /* CTC mode */
    TCCR1B = (1<<WGM12) | (1<<CS12) | (1<<CS10);
    
    /* 100ms period */
    // 8MHz/1024 = 7812.5 ticks/sec
    // for 100ms 781.25 - 1 = 780.25
    OCR1A = 780;
    
    /* Enable compare interrupt */
    TIMSK1 = (1<<OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
    timer1_fired = 1;
}

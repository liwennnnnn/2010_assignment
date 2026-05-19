/*
 * joystick.c
*/

#include <avr/io.h>
#include "joystick.h"

void joystick_init(void) {
    /* Left-adjust result OFF (ADLAR=0) for 10-bit precision */
    ADMUX = (1<<REFS0);

    /* Enable ADC, prescaler 64 (8MHz/64 = 125kHz, within 50-200kHz range) */
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1);
}

void joystick_start_conversion(uint8_t channel) {
    /* Select channel (lower 4 bits of ADMUX) */
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    /* Start conversion by setting ADSC bit */
    ADCSRA |= (1<<ADSC);
}

uint8_t joystick_conversion_complete(void) {
    /* ADSC bit clears to 0 when conversion is complete */
    return !(ADCSRA & (1<<ADSC));
}

uint16_t joystick_get_result(void) {
    /* Read ADCL before ADCH (locks register pair) */
    return ADC;
}

int16_t joystick_get_x(void) {
    /* Convert 0-1023 range to signed -512 to +511 */
    int16_t raw = (int16_t)joystick_get_result() - 512; // Subtract 512 so neutral (512) becomes 0

    /* Deadzone */
    if (raw > -JOYSTICK_DEADZONE && raw < JOYSTICK_DEADZONE) return 0;
    return raw;
}

int16_t joystick_get_y(void) {
    int16_t raw = (int16_t)joystick_get_result() - 512;
    if (raw > -JOYSTICK_DEADZONE && raw < JOYSTICK_DEADZONE) return 0;
    return raw;
}
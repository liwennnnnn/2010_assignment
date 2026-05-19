#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include <stdint.h>

/* ADC channels */
#define JOYSTICK_X_AXIS     0
#define JOYSTICK_Y_AXIS     1
#define JOYSTICK_DEADZONE   50

/* Initialise joystick */
void joystick_init(void);

/* Start a non-blocking ADC concersion on the given channel */
void joystick_start_conversion(uint8_t channel);

/* Returns 1 if ADC conversion is complete */
uint8_t joystick_conversion_complete(void);

/* Returns the last completed ADC result (0-1023) */
uint16_t joystick_get_result(void);

/* Returns signed tilt */
// negative = left, positive = right, 0 = neutral
int16_t joystick_get_x(void);
int16_t joystick_get_y(void);

#endif /* JOYSTICK_H_ */
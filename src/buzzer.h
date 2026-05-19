#ifndef BUZZER_H_
#define BUZZER_H_

#include <stdint.h>

/* Frequencies */
#define BUZZER_FREQ_DOT     4000    // Hz
#define BUZZER_FREQ_DASH    5000    // Hz  (1.25× ratio ✓)

void buzzer_init(void);

/* Buzzer play */
// last_mark: 1 = 10% duty, 0 = 50% duty

void buzzer_dot(uint8_t last_mark);

void buzzer_dash(uint8_t last_mark);

/* Buzzer stop */
void buzzer_stop(void);

#endif /* BUZZER_H_ */
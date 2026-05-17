#ifndef TIMER1_H_
#define TIMER1_H_

#include <stdint.h>

extern volatile uint8_t timer1_fired;

void timer1_init(void);

#endif /* TIMER1_H_ */
#ifndef SSD_H_
#define SSD_H_

#include <stdint.h>

#define SSD_BLANK       0x00    // turn off digit
#define SSD_DASH        0xFE
#define SSD_SEG_DASH    0x40    // display "-"
#define SSD_DP_BIT      0x80    // decimal point

/* Initialise SSD */
void ssd_init(void);

/* 
 * Display values on SSD.
 * left_digit: value 0-15 as hex, blank or "-".
 * right_digit: value 0-9, blank or "-".
 * dp_on: turn on decimal point with 1 and off with 0.
 */
void ssd_display(uint8_t left_digit, uint8_t right_digit, uint8_t dp_on);

void ssd_multiplex(void);



#endif /* SSD_H_ */
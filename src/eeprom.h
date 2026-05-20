/*
 * eeprom.h
 *
 * EEPROM storage for morse scrollback.
 * Circular buffer layout for wear levelling.
 *
 * Layout:
 *   [0]       write_head (uint16_t low byte) — next write index (0..499)
 *   [1]       write_head high byte
 *   [2]       stored_count — number of valid entries (0..50)
 *   [4..1003] circular buffer: 500 entries × 2 bytes (char, colour)
 *             Each entry = 2 bytes. 500 entries × 2 = 1000 bytes.
 *             Any single address is written at most once per 500 inputs.
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

/* Number of character slots in the circular buffer */
#define EEPROM_NUM_ENTRIES  500     // 50 chars * 10 slots

/* EEPROM base address fot circular buffer data */
#define EEPROM_DATA_BASE    4

/* EEPROM address for meta data */
#define EEPROM_ADDR_HEAD    0       // uint16_t: next write index (2 bytes)
#define EEPROM_ADDR_COUNT   2       // uint8_t: number of valid entries

/* Intialise storage, read metadata from EEPROM and validate */
void eeprom_storage_init(void);

/* Save one submitted character including the colour */
void eeprom_save_char(char c, uint8_t colour);

/* Restore max 50 recent chars into out_chars[] and out_colours[].
 * Returns the number of entries restored.
 */
uint8_t eeprom_restore_chars(char *out_chars, uint8_t *out_colours);

#endif /* EEPROM_H_ */
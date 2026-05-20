/*
 * eeprom.c
*/

#include <avr/eeprom.h>
#include "eeprom.h"

/* In-RAM cache of metadata */
// Avoid extra reads
static uint16_t write_head = 0; // next write position (0...499)
static uint8_t entry_count = 0; // valid entries in buffer (0..50)

void eeprom_storage_init(void) {
    /* Read metadata form EEPROM */
    // Low byte of write_head from EEPROM address 0
    uint8_t head_lo = eeprom_read_byte((uint8_t *)EEPROM_ADDR_HEAD);
    // High byte of write_head from EEPROM address 1
    uint8_t head_hi = eeprom_read_byte((uint8_t *)(EEPROM_ADDR_HEAD + 1));
    write_head = ((uint16_t)head_hi << 8) | head_lo;

    /* READ entry_count from EEPROM address 2 */
    entry_count = eeprom_read_byte((uint8_t *)EEPROM_ADDR_COUNT);

    /* Validate write_head */
    if (write_head >= EEPROM_NUM_ENTRIES) {
        write_head = 0; // reset to start of buffer
        entry_count = 0;    // no valid entry

        /* Write clean state back to EEPROM */
        eeprom_write_byte((uint8_t *)EEPROM_ADDR_HEAD, 0);  // head low byte
        eeprom_write_byte((uint8_t *)(EEPROM_ADDR_HEAD + 1), 0);    // head high byte
        eeprom_write_byte((uint8_t *)EEPROM_ADDR_COUNT, 0);  // entry count
    }

    /* Validate entry_count */
    if (entry_count > 50) {
        entry_count = 50;
        eeprom_write_byte((uint8_t *)EEPROM_ADDR_COUNT, 50);
    }
}

void eeprom_save_char(char c, uint8_t colour) {
    /* Compute byte address in EEPROM */
    // Each entry is 2 bytes (char + colour)
    uint16_t addr = EEPROM_DATA_BASE + write_head * 2;

    /* Write char and colour */
    eeprom_write_byte((uint8_t *)addr, (uint8_t)c);
    eeprom_write_byte((uint8_t *)(addr + 1), (uint8_t)colour);

    /* Advance write head circularly (wraps back to 0 after 499) */
    write_head = (write_head + 1) % EEPROM_NUM_ENTRIES;

    /* Update count */
    if (entry_count < 50) entry_count++;

    /* Write updated write_head to EEPROM address */
    eeprom_write_byte((uint8_t *)EEPROM_ADDR_HEAD, (uint8_t)(write_head & 0xFF));
    eeprom_write_byte((uint8_t *)(EEPROM_ADDR_HEAD + 1), (uint8_t)(write_head >> 8));

    /* Write update entry_count to EEPROM address 2 */
    eeprom_write_byte((uint8_t *)EEPROM_ADDR_COUNT, entry_count);
}

uint8_t eeprom_restore_chars(char *out_chars, uint8_t *out_colours) {
    /* No entries */
    if (entry_count == 0) return 0;

    /* Find index of the oldest entry in buffer */
    uint16_t oldest_char = (write_head + EEPROM_NUM_ENTRIES - entry_count) 
                            % EEPROM_NUM_ENTRIES;
    
    /* Read entry_count from oldest to newest into output array */
    for (uint8_t i = 0; i < entry_count; i++) {
        // Calculate circular index
        uint16_t idx  = (oldest_char + i) % EEPROM_NUM_ENTRIES;

        // Calculate EEPROM byte address
        uint16_t addr = EEPROM_DATA_BASE + idx * 2;

        /* Read and store character byte */
        out_chars[i] = (char)eeprom_read_byte((uint8_t *)addr);
        
        /* Read and store colour byte */
        out_colours[i] = eeprom_read_byte((uint8_t *)(addr + 1));
    }
    return entry_count;
}
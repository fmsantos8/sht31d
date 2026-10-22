/**
 * @file cmp_sht31d_config.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Construction and caller-owned storage for the SHT31-D composition root.
 */
#ifndef CMP_SHT31D_CONFIG_H
#define CMP_SHT31D_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "cmp_sht31d.h"

#include "i2c/hal_i2c.h"

/**
 * @brief Resources supplied by the composition, with no transfer of ownership.
 *
 * @details The board supplies power, ADDR wiring, and I2C pull-ups; the
 * composition supplies the constructed, possibly shared bus. No pin selection
 * or controller configuration is performed here.
 */
struct sht31d_config {
    hal_i2c_t *i2c;  /** Required, already constructed I2C master bus. */
    uint8_t address; /** Unshifted SHT31-D address: 0x44 or 0x45. */
};

/**
 * @brief Caller-owned persistent context; fields are private to the component.
 *
 * @details Declare in the composition root and pass sht31d_t pointers to
 * consumers. cmp_sht31d_init() sets all fields. No singleton, arena allocation, or retained
 * measurement result.
 */
struct sht31d {
    hal_i2c_t *i2c;           /** I2C bus handle. */
    uint8_t address;          /** Device address selected by the board. */
    bool initialized;         /** True after heater-disable initialization succeeds. */
    bool measurement_pending; /** True after a successful start until a read is attempted. */
};

#ifdef __cplusplus
}
#endif

#endif

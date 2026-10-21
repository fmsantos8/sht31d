/**
 * @file cmp_sht31d_config.h
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
 * @details The board supplies power, the ADDR connection, and I2C pull-ups. The
 * composition supplies a constructed master bus, which may be shared with other
 * components. This component does not select pins or configure the controller.
 */
struct sht31d_config {
    hal_i2c_t *i2c;  /** Required, already constructed I2C master bus. */
    uint8_t address; /** Unshifted SHT31-D address: 0x44 or 0x45. */
};

/**
 * @brief Caller-owned persistent context; fields are private to the component.
 * @details Declare storage in the composition root and pass sht31d_t pointers to
 * consumers. init() supplies all initial values. No singleton or HAL arena
 * allocation is used, and no measurement result is retained in this context.
 */
struct sht31d {
    hal_i2c_t *i2c;           /** Borrowed I2C bus handle. */
    uint8_t address;          /** Device address selected by the board. */
    bool initialized;         /** True after heater-disable initialization succeeds. */
    bool measurement_pending; /** True after a successful start until a read is attempted. */
};

#ifdef __cplusplus
}
#endif

#endif

/**
 * @file cmp_sht31d.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Single-shot SHT31-D acquisition
 */
#ifndef CMP_SHT31D_H
#define CMP_SHT31D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SHT31D_MEASUREMENT_TIME 16 /** Minimum conversion wait after cmp_sht31d_start_measurement() returns (ms) */

/** @brief Caller-owned context; its storage definition is in cmp_sht31d_config.h. */
typedef struct sht31d sht31d_t;

/** @brief Construction dependencies, defined in cmp_sht31d_config.h. */
typedef struct sht31d_config sht31d_config_t;

/** @brief Component results, independent of HAL and SDK status types. */
typedef enum {
    SHT31D_STATUS_OK = 0,              /** Operation completed successfully. */
    SHT31D_STATUS_INVALID_ARGUMENT,    /** A required pointer or device address is invalid. */
    SHT31D_STATUS_NOT_INITIALIZED,     /** The sensor or its I2C dependency is not initialized. */
    SHT31D_STATUS_INVALID_STATE,       /** A measurement is already pending, or none is pending for reading. */
    SHT31D_STATUS_NOT_SUPPORTED,       /** A required HAL operation is unavailable. */
    SHT31D_STATUS_TIMEOUT,             /** The configured I2C driver timeout expired. */
    SHT31D_STATUS_COMMUNICATION_ERROR, /** The I2C dependency could not complete a transfer. */
    SHT31D_STATUS_CRC_ERROR            /** At least one measurement word failed its CRC check. */
} sht31d_status_t;

/**
 * @brief One CRC-validated temperature and relative-humidity pair.
 *
 * @details Values use the datasheet conversion, rounded to the nearest integer.
 * Representation resolution does not imply corresponding measurement accuracy.
 */
typedef struct {
    int32_t temperature; /** Temperature in 0.001 degrees Celsius; e.g. 24800 means 24.8 C. */
    uint32_t humidity;   /** Relative humidity in 0.001 percent RH; e.g. 58300 means 58.3 %RH. */
} sht31d_sample_t;

/**
 * @brief Initialize the complete context and disable the sensor's heater.
 *
 * @details Initializes all fields; no prior zeroing required. Invalid arguments
 * preserve the context. A transfer failure leaves it unavailable; retry cmp_sht31d_init().
 * The board must provide stable power and a completed power-up/reset. Requires
 * exclusive single-shot use from idle; stop/reset external periodic acquisition
 * first and let any previous conversion finish. Does not reset the sensor or
 * construct the bus.
 *
 * @param[out] sensor Required caller-owned context.
 * @param[in] config Required bus handle and unshifted address.
 *
 * @retval SHT31D_STATUS_OK Context initialized and heater disabled.
 * @retval SHT31D_STATUS_INVALID_ARGUMENT Invalid pointer or address.
 * @retval SHT31D_STATUS_NOT_INITIALIZED I2C bus is unavailable.
 * @retval SHT31D_STATUS_NOT_SUPPORTED Required I2C operation is unavailable.
 * @retval SHT31D_STATUS_TIMEOUT I2C driver timeout expired.
 * @retval SHT31D_STATUS_COMMUNICATION_ERROR I2C transfer failed.
 */
sht31d_status_t cmp_sht31d_init(sht31d_t *sensor, const sht31d_config_t *config);

/**
 * @brief Start one high-repeatability measurement without clock stretching.
 *
 * @details Sends 0x2400 without waiting for conversion. After success, the caller
 * waits at least SHT31D_MEASUREMENT_TIME from return before reading. A pending
 * measurement rejects another start without I2C traffic. On failure, no read is
 * pending, but conversion may have started: wait the full interval before retry
 * or cmp_sht31d_init(). The caller handles retry timing and retains the last valid sample.
 *
 * @param[in,out] sensor Required initialized context with no pending measurement.
 *
 * @retval SHT31D_STATUS_OK Measurement started.
 * @retval SHT31D_STATUS_INVALID_ARGUMENT sensor is NULL.
 * @retval SHT31D_STATUS_NOT_INITIALIZED Context or I2C bus is unavailable.
 * @retval SHT31D_STATUS_INVALID_STATE A measurement is pending.
 * @retval SHT31D_STATUS_NOT_SUPPORTED Required I2C operation is unavailable.
 * @retval SHT31D_STATUS_TIMEOUT I2C driver timeout expired.
 * @retval SHT31D_STATUS_COMMUNICATION_ERROR I2C transfer failed.
 */
sht31d_status_t cmp_sht31d_start_measurement(sht31d_t *sensor);

/**
 * @brief Read six bytes, validate both CRCs, and convert a pending measurement.
 *
 * @details Wait the full conversion interval first; an early NACK is reported
 * as COMMUNICATION_ERROR, indistinguishable from other bus errors. No readiness
 * polling. An attempted read consumes pending work even on I2C or CRC failure;
 * start a new measurement afterward. Validation errors preserve pending work
 * without I2C traffic. The output changes only on complete success.
 *
 * @param[in,out] sensor Required initialized context with a pending measurement.
 * @param[out] sample Required independent output, written only on success.
 *
 * @retval SHT31D_STATUS_OK Validated sample written.
 * @retval SHT31D_STATUS_INVALID_ARGUMENT A required pointer is NULL.
 * @retval SHT31D_STATUS_NOT_INITIALIZED Context or I2C bus is unavailable.
 * @retval SHT31D_STATUS_INVALID_STATE No measurement is pending.
 * @retval SHT31D_STATUS_NOT_SUPPORTED Required I2C operation is unavailable.
 * @retval SHT31D_STATUS_TIMEOUT I2C driver timeout expired.
 * @retval SHT31D_STATUS_COMMUNICATION_ERROR I2C transfer failed.
 * @retval SHT31D_STATUS_CRC_ERROR Temperature or humidity CRC failed.
 */
sht31d_status_t cmp_sht31d_read_measurement(sht31d_t *sensor, sht31d_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif

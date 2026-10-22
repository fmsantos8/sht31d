#include "cmp_sht31d.h"
#include "cmp_sht31d_config.h"

#include <stddef.h>

static sht31d_status_t cmp_sht31d_translate_status(hal_status_t status) {
    switch (status) {
        case HAL_STATUS_OK:
            return SHT31D_STATUS_OK;
        case HAL_STATUS_INVALID_ARGUMENT:
            return SHT31D_STATUS_INVALID_ARGUMENT;
        case HAL_STATUS_NOT_INITIALIZED:
            return SHT31D_STATUS_NOT_INITIALIZED;
        case HAL_STATUS_NOT_SUPPORTED:
            return SHT31D_STATUS_NOT_SUPPORTED;
        case HAL_STATUS_TIMEOUT:
            return SHT31D_STATUS_TIMEOUT;
        default:
            return SHT31D_STATUS_COMMUNICATION_ERROR;
    }
}

static sht31d_status_t cmp_sht31d_require_initialized(const sht31d_t *sensor) {
    if (sensor == NULL) {
        return SHT31D_STATUS_INVALID_ARGUMENT;
    }
    return sensor->initialized ? SHT31D_STATUS_OK : SHT31D_STATUS_NOT_INITIALIZED;
}

static uint8_t cmp_sht31d_word_crc(const uint8_t *bytes) {
    uint8_t crc = 0xFF;
    for (uint32_t i = 0; i < 2; ++i) {
        crc ^= bytes[i];
        for (uint32_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80U) != 0 ? (uint8_t)(((uint32_t)crc << 1) ^ 0x31U) : (uint8_t)((uint32_t)crc << 1);
        }
    }
    return crc;
}

sht31d_status_t cmp_sht31d_init(sht31d_t *sensor, const sht31d_config_t *config) {
    if (sensor == NULL || config == NULL || config->i2c == NULL ||
        (config->address != 0x44 && config->address != 0x45)) {
        return SHT31D_STATUS_INVALID_ARGUMENT;
    }
    *sensor = (sht31d_t){.i2c = config->i2c, .address = config->address};

    // Keep the internal heater off for ambient temperature/humidity acquisition.
    const uint8_t command[] = {0x30, 0x66};
    sht31d_status_t status = cmp_sht31d_translate_status(hal_i2c_write(sensor->i2c, sensor->address, command, (uint32_t)sizeof(command)));
    if (status == SHT31D_STATUS_OK) {
        sensor->initialized = true;
    }
    return status;
}

sht31d_status_t cmp_sht31d_start_measurement(sht31d_t *sensor) {
    sht31d_status_t status = cmp_sht31d_require_initialized(sensor);
    if (status != SHT31D_STATUS_OK) {
        return status;
    }
    if (sensor->measurement_pending) {
        return SHT31D_STATUS_INVALID_STATE;
    }

    // Single shot, high repeatability, no clock stretching. Conversion is external
    // to this transfer; the caller waits before requesting the six result bytes.
    const uint8_t command[] = {0x24, 0x00};
    status = cmp_sht31d_translate_status(hal_i2c_write(sensor->i2c, sensor->address, command, (uint32_t)sizeof(command)));
    if (status == SHT31D_STATUS_OK) {
        sensor->measurement_pending = true;
    }
    return status;
}

sht31d_status_t cmp_sht31d_read_measurement(sht31d_t *sensor, sht31d_sample_t *sample) {
    if (sample == NULL) {
        return SHT31D_STATUS_INVALID_ARGUMENT;
    }
    sht31d_status_t status = cmp_sht31d_require_initialized(sensor);
    if (status != SHT31D_STATUS_OK) {
        return status;
    }
    if (!sensor->measurement_pending) {
        return SHT31D_STATUS_INVALID_STATE;
    }

    // Partial/failed reads cannot be reused as a valid acquisition. Require a
    // fresh command even when the bus fails before completing the transaction.
    sensor->measurement_pending = false;
    uint8_t bytes[6];
    status = cmp_sht31d_translate_status(hal_i2c_read(sensor->i2c, sensor->address, bytes, (uint32_t)sizeof(bytes)));
    if (status != SHT31D_STATUS_OK) {
        return status;
    }
    if (cmp_sht31d_word_crc(bytes) != bytes[2] || cmp_sht31d_word_crc(bytes + 3) != bytes[5]) {
        return SHT31D_STATUS_CRC_ERROR;
    }

    uint32_t raw_temperature = ((uint32_t)bytes[0] << 8) | bytes[1];
    uint32_t raw_humidity = ((uint32_t)bytes[3] << 8) | bytes[4];

    // Datasheet scale: T = -45 + 175 * raw / 65535; RH = 100 * raw / 65535.
    // Widen before multiplication and round once to millidegrees/millipercent.
    const sht31d_sample_t measurement = {
        .temperature = -45000 + (int32_t)((UINT64_C(175000) * raw_temperature + 32767U) / 65535U),
        .humidity = (uint32_t)((UINT64_C(100000) * raw_humidity + 32767U) / 65535U),
    };
    *sample = measurement;
    return SHT31D_STATUS_OK;
}

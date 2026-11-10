#pragma once

#include "esp_err.h"
#include "esp_partition.h"
#include "spi_flash_mmap.h"
#include <stdint.h>

#define RING_SECTOR_SIZE     SPI_FLASH_SEC_SIZE
#define RING_MIN_RECORD_SIZE 32U

esp_err_t ring_buffer_init(const char *parition_label, size_t data_size);
esp_err_t ring_buffer_write(const uint8_t *data);
esp_err_t ring_buffer_read_all(uint8_t **buffer, size_t *size);

typedef struct {
    const esp_partition_t *partition;

    // The last written record address (highest sequence #)
    size_t head;
    // The first written valid record address (lowest sequence #)
    size_t tail;
    // The size of one record in the buffer
    size_t record_size;
    // The size of the data inside the record (without metadata and padding)
    size_t data_size;
    // The amount of records stored in the buffer
    size_t record_count;
    // The amount of sectors the buffer contains
    size_t sector_count;
    // The amount of records one sector can contain
    size_t records_per_sector;
    // A strictly increasing monotonic sequence defining the write order
    uint32_t next_sequence;
} ring_buffer_t;

typedef struct {
    uint32_t sequence;
    uint32_t crc;
} ring_buffer_data_metadata_t;

typedef struct {
    ring_buffer_data_metadata_t metadata;
    uint8_t                    *data;
} ring_buffer_data_t;
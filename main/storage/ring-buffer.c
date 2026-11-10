#include "ring-buffer.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_rom_crc.h"
#include <stdint.h>

static const char   *TAG = "RING BUFFER";
static ring_buffer_t metadata = {0};

static inline size_t ring_record_size(size_t n) {
    size_t size = RING_MIN_RECORD_SIZE;

    while (size < n) {
        if (size > SIZE_MAX / 2) {
            return 0;
        }
        size *= 2;
    }

    return size;
}

ring_buffer_data_t parse_record(uint8_t *buffer) {
    ring_buffer_data_t record;
    memcpy(&record.metadata, buffer, sizeof(record.metadata));
    record.data = buffer + sizeof(record.metadata);

    return record;
}

uint8_t check_data_integrity(ring_buffer_data_t *record) {
    if (record->metadata.sequence == UINT32_MAX) {
        return false;
    }
    uint32_t crc = esp_rom_crc32_le(0, record->data, metadata.data_size);
    return crc == record->metadata.crc;
}

// Initialize the ring buffer metadata struct
esp_err_t ring_buffer_init(const char *parition_label, size_t data_size) {
    ESP_RETURN_ON_ERROR(esp_partition_find_first_err(ESP_PARTITION_TYPE_DATA,
                                                     ESP_PARTITION_SUBTYPE_DATA_UNDEFINED,
                                                     parition_label, &metadata.partition),
                        TAG, "Partition could not be found");

    ESP_RETURN_ON_FALSE(metadata.partition != NULL, ESP_ERR_NOT_FOUND, TAG,
                        "Partition could not be loaded");

    ESP_RETURN_ON_FALSE(metadata.partition->size % RING_SECTOR_SIZE == 0, ESP_ERR_INVALID_SIZE, TAG,
                        "Partition size must be a multiple of sector size");

    metadata.record_size = ring_record_size(data_size + sizeof(ring_buffer_data_metadata_t));

    ESP_RETURN_ON_FALSE(metadata.record_size <= RING_SECTOR_SIZE, ESP_ERR_INVALID_SIZE, TAG,
                        "Record size must be smaller than sector size");

    ESP_RETURN_ON_FALSE(RING_SECTOR_SIZE % metadata.record_size == 0, ESP_ERR_INVALID_SIZE, TAG,
                        "Record size must divide sector size");
    metadata.data_size = data_size;

    metadata.sector_count = metadata.partition->size / RING_SECTOR_SIZE;
    metadata.records_per_sector = RING_SECTOR_SIZE / metadata.record_size;

    metadata.record_count = 0;
    metadata.head = 0;
    metadata.tail = 0;

    // Tail is set to the valid record with the lowest sequence #
    // Head is set to the valid record with the highest sequence #
    uint32_t       lowest_seq = UINT32_MAX;
    uint32_t       highest_seq = 0;
    static uint8_t sector_data[RING_SECTOR_SIZE];
    for (size_t i = 0; i < metadata.sector_count; i++) {
        ESP_RETURN_ON_ERROR(esp_partition_read_raw(metadata.partition, i * RING_SECTOR_SIZE,
                                                   sector_data, RING_SECTOR_SIZE),
                            TAG, "Sector could not be read properly");

        for (size_t j = 0; j < metadata.records_per_sector; j++) {

            ring_buffer_data_t record = parse_record(&sector_data[j * metadata.record_size]);
            uint8_t            is_valid_record = check_data_integrity(&record);

            if (is_valid_record) {
                metadata.record_count++;

                if (record.metadata.sequence < lowest_seq) {
                    metadata.tail = i * RING_SECTOR_SIZE + j * metadata.record_size;
                    lowest_seq = record.metadata.sequence;
                }

                if (highest_seq < record.metadata.sequence) {
                    metadata.head = i * RING_SECTOR_SIZE + j * metadata.record_size;
                    highest_seq = record.metadata.sequence;
                }
            }
        }
    }

    metadata.next_sequence = highest_seq + 1;

    return ESP_OK;
}

// Write a new data record into the next free write address
esp_err_t ring_buffer_write(const uint8_t *data) {
    static uint8_t record_buffer[RING_SECTOR_SIZE];

    ring_buffer_data_metadata_t record_metadata = {
        .sequence = metadata.next_sequence, .crc = esp_rom_crc32_le(0, data, metadata.data_size)};

    memset(record_buffer, 0xFF, metadata.record_size);

    memcpy(record_buffer, &record_metadata, sizeof(record_metadata));
    memcpy(record_buffer + sizeof(record_metadata), data, metadata.data_size);

    size_t write_offset = metadata.record_count == 0 ? 0 : metadata.head + metadata.record_size;
    if (write_offset >= metadata.partition->size) {
        write_offset = 0;
    }

    // Erase sector when entering it
    if (write_offset % RING_SECTOR_SIZE == 0) {
        ESP_RETURN_ON_ERROR(
            esp_partition_erase_range(metadata.partition, write_offset, RING_SECTOR_SIZE), TAG,
            "Could not erase sector before write");

        if (metadata.record_count > 0 && write_offset == metadata.tail) {

            metadata.record_count -= metadata.records_per_sector;

            metadata.tail = (metadata.tail + RING_SECTOR_SIZE) % metadata.partition->size;
        }
    }

    ESP_RETURN_ON_ERROR(esp_partition_write_raw(metadata.partition, write_offset, record_buffer,
                                                metadata.record_size),
                        TAG, "Could not write record");

    metadata.head = write_offset;
    metadata.next_sequence++;
    metadata.record_count++;

    return ESP_OK;
}

// Get all data (without metadata and padding) currently held in the ring buffer
esp_err_t ring_buffer_read_all(uint8_t **buffer, size_t *size) {

    if (buffer == NULL || size == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *buffer = NULL;
    *size = 0;

    if (metadata.record_count == 0) {
        return ESP_OK;
    }

    size_t   total_size = metadata.record_count * metadata.data_size;
    uint8_t *result = heap_caps_malloc(total_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }

    size_t offset = metadata.tail;

    for (size_t i = 0; i < metadata.record_count; i++) {
        esp_err_t err =
            esp_partition_read_raw(metadata.partition, offset + sizeof(ring_buffer_data_metadata_t),
                                   result + i * metadata.data_size, metadata.data_size);

        if (err != ESP_OK) {
            free(result);
            return err;
        }

        offset += metadata.record_size;

        if (offset >= metadata.partition->size) {
            offset = 0;
        }
    }

    *buffer = result;
    *size = total_size;

    return ESP_OK;
}
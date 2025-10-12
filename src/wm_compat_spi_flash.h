// src/wm_compat_spi_flash.h
#pragma once

#if __has_include("spi_flash_mmap.h")
// ----- Arduino-ESP32 2.x / IDF 4.x -----
  #include "spi_flash_mmap.h"

#else
// ----- Arduino-ESP32 3.x / IDF 5.x -----
  #include <esp_partition.h>
  #include <esp_spi_flash.h>
  #include <esp_err.h>

// Old -> new type/constant aliases so existing code compiles
  typedef esp_partition_mmap_handle_t spi_flash_mmap_handle_t;
  #define SPI_FLASH_MMAP_DATA ESP_PARTITION_MMAP_DATA

// Old API returned void, so keep that signature
  static inline void spi_flash_munmap(spi_flash_mmap_handle_t handle) {
    esp_partition_munmap(handle);
  }

/*
 * Minimal compatibility for code that previously did:
 *   spi_flash_mmap(offset, length, SPI_FLASH_MMAP_DATA, &ptr, &handle)
 *
 * NOTE: This maps from the FIRST data partition it finds (named "storage" or "spiffs",
 *       otherwise any DATA partition). If your code expects a specific partition or a
 *       raw chip address, you should adapt this to your layout.
 */
  static inline esp_err_t spi_flash_mmap(size_t offset_in_partition,
                                         size_t length,
                                         int /*SPI_FLASH_MMAP_DATA*/,
                                         const void** out_ptr,
                                         spi_flash_mmap_handle_t* out_handle)
  {
    const esp_partition_t* part =
      esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
    if (!part)
      part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "spiffs");
    if (!part)
      part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    if (!part) return ESP_ERR_NOT_FOUND;

    if (offset_in_partition + length > part->size) return ESP_ERR_INVALID_SIZE;

    return esp_partition_mmap(part, offset_in_partition, length,
                              ESP_PARTITION_MMAP_DATA, out_ptr, out_handle);
  }
#endif

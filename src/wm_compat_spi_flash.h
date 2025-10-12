// src/wm_compat_spi_flash.h
#pragma once

#if __has_include("spi_flash_mmap.h")
// Arduino-ESP32 2.x / IDF 4.x
  #include "spi_flash_mmap.h"
#else
// Arduino-ESP32 3.x / IDF 5.x: spi_flash_mmap.h no longer exists
  #include "esp_partition.h"
  #include "esp_spi_flash.h"

  // Type/constant aliases so existing code compiles unchanged
  typedef esp_partition_mmap_handle_t spi_flash_mmap_handle_t;
  #define SPI_FLASH_MMAP_DATA ESP_PARTITION_MMAP_DATA

  // Map/unmap aliases
  static inline esp_err_t spi_flash_munmap(spi_flash_mmap_handle_t h) {
    return esp_partition_munmap(h);
  }

  // Minimal drop-in for common “map data area” use.
  // If your original code used a specific partition name, change the finder below.
  static inline esp_err_t spi_flash_mmap(size_t offset_in_part,
                                         size_t length,
                                         int /*SPI_FLASH_MMAP_DATA*/,
                                         const void** out_ptr,
                                         spi_flash_mmap_handle_t* out_handle)
  {
    // Prefer a named data partition if you have one (e.g. "storage" or "spiffs")
    const esp_partition_t* part =
      esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
    if (!part) part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "spiffs");
    if (!part) part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    if (!part) return ESP_ERR_NOT_FOUND;
    if (offset_in_part + length > part->size) return ESP_ERR_INVALID_SIZE;

    return esp_partition_mmap(part, offset_in_part, length,
                              ESP_PARTITION_MMAP_DATA, out_ptr, out_handle);
  }
#endif

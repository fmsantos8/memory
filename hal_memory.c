#include "hal_memory.h"

#include <stdint.h>

_Static_assert(_Alignof(max_align_t) <= UINT32_MAX, "hal_memory alignment must fit uint32_t");

/**
 * @brief Private context for the module's single arena.
 */
typedef struct {
    unsigned char *buffer; /** Caller-owned backing storage aligned for max_align_t. */
    uint32_t capacity;     /** Total backing-storage size in bytes. */
    uint32_t used;         /** Bytes consumed by blocks and preceding alignment padding. */
    bool initialized;      /** True after successful arena initialization. */
    bool sealed;           /** True after allocation has permanently ended. */
} hal_memory_t;

static hal_memory_t memory = {0};

hal_memory_status_t hal_memory_init(void *buffer, uint32_t capacity) {
    if (memory.initialized) {
        return HAL_MEMORY_INVALID_STATE;
    }
    if (buffer == NULL || capacity == 0) {
        return HAL_MEMORY_INVALID_ARGUMENT;
    }

    if ((uintptr_t)buffer % _Alignof(max_align_t) != 0) {
        return HAL_MEMORY_INVALID_ARGUMENT;
    }

    memory.buffer = buffer;
    memory.capacity = capacity;
    memory.used = 0;
    memory.initialized = true;
    memory.sealed = false;
    return HAL_MEMORY_OK;
}

hal_memory_status_t hal_memory_alloc(uint32_t size, void **pointer) {
    if (pointer != NULL) {
        *pointer = NULL;
    }

    if (pointer == NULL || size == 0) {
        return HAL_MEMORY_INVALID_ARGUMENT;
    }

    if (!memory.initialized) {
        return HAL_MEMORY_NOT_INITIALIZED;
    }

    if (memory.sealed) {
        return HAL_MEMORY_INVALID_STATE;
    }

    const uint32_t alignment = (uint32_t)_Alignof(max_align_t);
    const uint32_t padding = (alignment - memory.used % alignment) % alignment;
    const uint32_t remaining = memory.capacity - memory.used;

    // Subtract before adding so even a UINT32_MAX request cannot wrap the cursor.
    if (padding > remaining || size > remaining - padding) {
        return HAL_MEMORY_NO_MEMORY;
    }

    *pointer = memory.buffer + memory.used + padding;
    memory.used += padding + size;
    return HAL_MEMORY_OK;
}

hal_memory_status_t hal_memory_seal(void) {
    if (!memory.initialized) {
        return HAL_MEMORY_NOT_INITIALIZED;
    }

    memory.sealed = true;
    return HAL_MEMORY_OK;
}

hal_memory_status_t hal_memory_get_info(hal_memory_info_t *info) {
    if (info == NULL) {
        return HAL_MEMORY_INVALID_ARGUMENT;
    }

    if (!memory.initialized) {
        return HAL_MEMORY_NOT_INITIALIZED;
    }

    *info = (hal_memory_info_t){
        .capacity = memory.capacity,
        .used = memory.used,
        .remaining = memory.capacity - memory.used,
        .sealed = memory.sealed,
    };
    return HAL_MEMORY_OK;
}

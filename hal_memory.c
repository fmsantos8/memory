#include "hal_memory.h"

#include <stdint.h>

_Static_assert(_Alignof(max_align_t) <= UINT32_MAX, "hal_memory alignment must fit uint32_t");

hal_memory_status_t hal_memory_init(hal_memory_t *memory, void *buffer, uint32_t capacity) {
    if (memory == NULL || buffer == NULL || capacity == 0) {
        return HAL_MEMORY_INVALID_ARGUMENT;
    }

    if ((uintptr_t)buffer % _Alignof(max_align_t) != 0) {
        return HAL_MEMORY_INVALID_ARGUMENT;
    }

    memory->buffer = buffer;
    memory->capacity = capacity;
    memory->used = 0;
    memory->initialized = true;
    memory->sealed = false;
    return HAL_MEMORY_OK;
}

hal_memory_status_t hal_memory_alloc(hal_memory_t *memory, uint32_t size, void **out_pointer) {
    if (out_pointer != NULL) {
        *out_pointer = NULL;
    }

    if (memory == NULL || out_pointer == NULL || size == 0) {
        return HAL_MEMORY_INVALID_ARGUMENT;
    }

    if (!memory->initialized) {
        return HAL_MEMORY_NOT_INITIALIZED;
    }

    if (memory->sealed) {
        return HAL_MEMORY_INVALID_STATE;
    }

    const uint32_t alignment = (uint32_t)_Alignof(max_align_t);
    const uint32_t padding = (alignment - memory->used % alignment) % alignment;
    const uint32_t remaining = memory->capacity - memory->used;

    /* Subtract before adding so even a UINT32_MAX request cannot wrap the cursor. */
    if (padding > remaining || size > remaining - padding) {
        return HAL_MEMORY_NO_MEMORY;
    }

    *out_pointer = memory->buffer + memory->used + padding;
    memory->used += padding + size;
    return HAL_MEMORY_OK;
}

hal_memory_status_t hal_memory_seal(hal_memory_t *memory) {
    if (memory == NULL) {
        return HAL_MEMORY_INVALID_ARGUMENT;
    }

    if (!memory->initialized) {
        return HAL_MEMORY_NOT_INITIALIZED;
    }

    memory->sealed = true;
    return HAL_MEMORY_OK;
}

hal_memory_status_t hal_memory_get_info(const hal_memory_t *memory, hal_memory_info_t *out_info) {
    if (memory == NULL || out_info == NULL) {
        return HAL_MEMORY_INVALID_ARGUMENT;
    }

    if (!memory->initialized) {
        return HAL_MEMORY_NOT_INITIALIZED;
    }

    *out_info = (hal_memory_info_t){
        .capacity = memory->capacity,
        .used = memory->used,
        .remaining = memory->capacity - memory->used,
        .sealed = memory->sealed,
    };
    return HAL_MEMORY_OK;
}

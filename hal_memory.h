/**
 * @file hal_memory.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Shared persistent memory arena.
 */
#ifndef HAL_MEMORY_H
#define HAL_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L)
#error "hal_memory requires C11 or later"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Memory-module results, independent of HAL and SDK status types. */
typedef enum {
    HAL_MEMORY_OK = 0,           /** Operation succeeded. */
    HAL_MEMORY_INVALID_ARGUMENT, /** A required pointer, size, capacity, or buffer alignment is invalid. */
    HAL_MEMORY_NOT_INITIALIZED,  /** The arena has not been initialized. */
    HAL_MEMORY_INVALID_STATE,    /** Initialization already succeeded, or allocation is forbidden because the arena is sealed. */
    HAL_MEMORY_NO_MEMORY         /** The requested block and required alignment padding do not fit. */
} hal_memory_status_t;

/** @brief Arena snapshot, available before and after sealing. */
typedef struct {
    uint32_t capacity;  /** Total backing-storage size in bytes. */
    uint32_t used;      /** Bytes consumed by blocks and preceding alignment padding. */
    uint32_t remaining; /** capacity - used; a subsequent allocation may need padding. */
    bool sealed;        /** True after allocation has permanently ended. */
} hal_memory_info_t;

/**
 * @brief Initialize the module's singleton arena with external storage.
 *
 * @details One private arena serves the firmware; no caller context is needed.
 * Dedicate at least capacity writable bytes, aligned for max_align_t, for all
 * allocated objects' lifetimes. Storage must not overlap outputs or another
 * allocator's storage and is not cleared. Invalid initial arguments allow retry;
 * after success, init() returns INVALID_STATE without changing either buffer
 * or the arena, even after sealing. Serialize all calls in normal context;
 * no synchronization or ISR support.
 *
 * @param[in] buffer Caller-owned backing storage, suitably aligned.
 * @param[in] capacity Positive backing-storage size in bytes; need not be an
 * alignment multiple.
 *
 * @retval HAL_MEMORY_OK The arena is initialized and open.
 * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL, capacity is
 * zero, or buffer is not aligned for max_align_t.
 * @retval HAL_MEMORY_INVALID_STATE The arena has already been initialized.
 */
hal_memory_status_t hal_memory_init(void *buffer, uint32_t capacity);

/**
 * @brief Reserve a persistent, nonempty block aligned for max_align_t.
 *
 * @details Blocks are uninitialized; alignment padding consumes capacity.
 * Extended alignment is unsupported. Failure consumes no storage. When
 * pointer is non-NULL, *pointer is cleared before validation and receives
 * the block address only on success. The output must not overlap
 * backing storage. Pass an actual void * variable's address, then assign it to
 * the typed pointer; do not cast a typed pointer-to-pointer to void **.
 *
 * @param[in] size Positive requested byte count.
 * @param[out] pointer Required pointer output, independent of the arena.
 *
 * @retval HAL_MEMORY_OK A disjoint block of size bytes is reserved.
 * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL or size is zero.
 * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
 * @retval HAL_MEMORY_INVALID_STATE The arena is sealed.
 * @retval HAL_MEMORY_NO_MEMORY The block and required padding do not fit.
 */
hal_memory_status_t hal_memory_alloc(uint32_t size, void **pointer);

/**
 * @brief Permanently prevent further allocations without altering live blocks.
 *
 * @details Idempotent; preserves storage and consumption. No reset or reopening
 * during this firmware run, including through init().
 *
 * @retval HAL_MEMORY_OK The arena is sealed, including a repeated seal.
 * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
 */
hal_memory_status_t hal_memory_seal(void);

/**
 * @brief Obtain capacity, consumption, and lifecycle information.
 *
 * @details Queries allocate no storage and remain available after sealing.
 * The output is written only on success and must not overlap backing storage.
 *
 * @param[out] info Required independent snapshot output.
 *
 * @retval HAL_MEMORY_OK The snapshot was written.
 * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL.
 * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
 */
hal_memory_status_t hal_memory_get_info(hal_memory_info_t *info);

#ifdef __cplusplus
}
#endif

#endif

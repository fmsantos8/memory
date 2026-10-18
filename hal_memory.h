/**
 * @file hal_memory.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Shared persistent memory arena.
 *
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

/**
 * @brief Memory-module results, independent of HAL and SDK status types.
 *
 */
typedef enum {
    HAL_MEMORY_OK = 0,           /** Operation succeeded. */
    HAL_MEMORY_INVALID_ARGUMENT, /** A required pointer, size, capacity, or buffer alignment is invalid. */
    HAL_MEMORY_NOT_INITIALIZED,  /** The arena has not been initialized. */
    HAL_MEMORY_INVALID_STATE,    /** Initialization already succeeded, or allocation is forbidden because the arena is sealed. */
    HAL_MEMORY_NO_MEMORY         /** The requested block and required alignment padding do not fit. */
} hal_memory_status_t;

/** @brief
 * Snapshot of an initialized arena, available before and after sealing.
 *
 */
typedef struct {
    uint32_t capacity;  /** Total backing-storage size in bytes. */
    uint32_t used;      /** Bytes consumed by blocks and preceding alignment padding. */
    uint32_t remaining; /** capacity - used; a subsequent allocation may need padding. */
    bool sealed;        /** True after allocation has permanently ended. */
} hal_memory_info_t;

/**
 * @brief Initialize the module's singleton arena with external storage.
 *
 * @details buffer must address at least capacity writable bytes and be aligned
 * for max_align_t. Its extent and lifetime are the caller's responsibility.
 * Backing storage must be dedicated to this arena and must not overlap output
 * parameters or storage managed by another allocator. Initialization does not
 * clear backing storage. The arena context is private to this module; no context
 * initialization is required from the caller. One arena serves the firmware.
 * Invalid initial arguments leave initialization available for another attempt.
 * Once initialization succeeds, all further initialization calls return
 * HAL_MEMORY_INVALID_STATE, including after sealing, without modifying the arena
 * or either buffer. All operations are sequential, in normal execution context;
 * the module provides no synchronization or ISR support.
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
 * @details The block's contents are not initialized. Padding before the block
 * consumes arena capacity. Types requiring extended alignment beyond max_align_t
 * are outside this contract. Failed allocation consumes no storage. If
 * out_pointer is not NULL, *out_pointer is set to NULL before validation and
 * receives the allocated address only on success. The output variable must not
 * overlap the backing storage. Pass the address of an actual void *
 * variable, then assign that pointer to the desired object-pointer type; do not
 * cast an object-pointer-to-pointer to void **.
 *
 * @param[in] size Positive requested byte count.
 * @param[out] out_pointer Required pointer output, independent of the arena.
 *
 * @retval HAL_MEMORY_OK A disjoint block of size bytes is reserved.
 * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL or size is zero.
 * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
 * @retval HAL_MEMORY_INVALID_STATE The arena is sealed.
 * @retval HAL_MEMORY_NO_MEMORY The block and required padding do not fit.
 */
hal_memory_status_t hal_memory_alloc(uint32_t size, void **out_pointer);

/**
 * @brief Permanently prevent further allocations without altering live blocks.
 *
 * @details Sealing is idempotent. This operation neither clears storage nor
 * changes its consumption. Sealing is permanent for this firmware run;
 * further initialization calls cannot reset or reopen the arena.
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
 * @param[out] out_info Required independent snapshot output.
 *
 * @retval HAL_MEMORY_OK The snapshot was written.
 * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL.
 * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
 */
hal_memory_status_t hal_memory_get_info(hal_memory_info_t *out_info);

#ifdef __cplusplus
}
#endif

#endif

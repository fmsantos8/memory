/**
 * @file hal_memory.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Memory arena allocation
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
    HAL_MEMORY_OK = 0,            /** Operation succeeded. */
    HAL_MEMORY_INVALID_ARGUMENT, /** A required pointer, size, capacity, or buffer alignment is invalid. */
    HAL_MEMORY_NOT_INITIALIZED,  /** The arena has not been initialized. */
    HAL_MEMORY_INVALID_STATE,    /** Allocation is forbidden because the arena is sealed. */
    HAL_MEMORY_NO_MEMORY         /** The requested block and required alignment padding do not fit. */
} hal_memory_status_t;

/**
 * @brief Caller-owned arena context, exposed to permit static allocation.
 *
 */
typedef struct {
    unsigned char *buffer; /** Caller-owned backing storage aligned for max_align_t. */
    uint32_t capacity;     /** Total backing-storage size in bytes. */
    uint32_t used;         /** Bytes consumed by blocks and preceding alignment padding. */
    bool initialized;      /** True after successful arena initialization. */
    bool sealed;           /** True after allocation has permanently ended. */
} hal_memory_t;

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
 * @brief Bind an unused context to external storage and permit allocation.
 *
 * @details buffer must address at least capacity writable bytes and be aligned
 * for max_align_t. Its extent and lifetime are the caller's responsibility.
 * Backing storage must not overlap the context, output parameters, or storage
 * managed by another arena. Initialization does not clear backing storage.
 * Failures leave the context and backing storage unchanged.
 * The context's previous contents are not read. After success, the caller must
 * not call this function again for the same context. Reinitialization is not
 * detected and would reset consumption and unseal the arena, allowing live blocks
 * to be overwritten. After failure, initialize successfully before using any
 * other operation on a context whose fields were not previously initialized.
 *
 * @param[out] memory Caller-owned context; prior zero initialization is not required.
 * @param[in] buffer Caller-owned backing storage, suitably aligned.
 * @param[in] capacity Positive backing-storage size in bytes; need not be an
 * alignment multiple.
 *
 * @retval HAL_MEMORY_OK The arena is initialized and open.
 * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL, capacity is
 * zero, or buffer is not aligned for max_align_t.
 */
hal_memory_status_t hal_memory_init(hal_memory_t *memory, void *buffer, uint32_t capacity);

/**
 * @brief Reserve a persistent, nonempty block aligned for max_align_t.
 *
 * @details The block's contents are not initialized. Padding before the block
 * consumes arena capacity. Types requiring extended alignment beyond max_align_t
 * are outside this contract. Failed allocation consumes no storage. If
 * out_pointer is not NULL, *out_pointer is set to NULL before validation and
 * receives the allocated address only on success. The output variable must not
 * overlap the context or backing storage. Pass the address of an actual void *
 * variable, then assign that pointer to the desired object-pointer type; do not
 * cast an object-pointer-to-pointer to void **.
 *
 * @param[in,out] memory Arena context.
 * @param[in] size Positive requested byte count.
 * @param[out] out_pointer Required pointer output, independent of the arena.
 *
 * @retval HAL_MEMORY_OK A disjoint block of size bytes is reserved.
 * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL or size is zero.
 * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
 * @retval HAL_MEMORY_INVALID_STATE The arena is sealed.
 * @retval HAL_MEMORY_NO_MEMORY The block and required padding do not fit.
 */
hal_memory_status_t hal_memory_alloc(hal_memory_t *memory, uint32_t size, void **out_pointer);

/**
 * @brief Permanently prevent further allocations without altering live blocks.
 *
 * @details Sealing is idempotent. This operation neither clears storage nor
 * changes its consumption. Sealing is permanent for the context's lifetime;
 * calling hal_memory_init again is outside the API contract.
 *
 * @param[in,out] memory Arena context.
 *
 * @retval HAL_MEMORY_OK The arena is sealed, including a repeated seal.
 * @retval HAL_MEMORY_INVALID_ARGUMENT memory is NULL.
 * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
 */
hal_memory_status_t hal_memory_seal(hal_memory_t *memory);

/**
 * @brief Obtain capacity, consumption, and lifecycle information.
 *
 * @details Queries allocate no storage and remain available after sealing.
 * The output is written only on success and must not overlap the context or
 * backing storage.
 *
 * @param[in] memory Arena context.
 * @param[out] out_info Required independent snapshot output.
 *
 * @retval HAL_MEMORY_OK The snapshot was written.
 * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL.
 * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
 */
hal_memory_status_t hal_memory_get_info(const hal_memory_t *memory, hal_memory_info_t *out_info);

#ifdef __cplusplus
}
#endif

#endif

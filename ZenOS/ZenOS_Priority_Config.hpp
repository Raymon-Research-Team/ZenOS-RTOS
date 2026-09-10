#pragma once
/**
 * @file    ZenOS_Priority_Config.hpp
 * @brief   Compile-time scheduler priority configuration
 *
 * OS_MAX_PRIORITIES is the number of scheduler priority slots.
 * Priority 0 is reserved; usable priorities are 1..OS_MAX_PRIORITIES-1.
 *
 * Default: 32 slots (priorities 1..31).
 * Maximum: 256 slots (priorities 1..255).
 */

#ifndef OS_MAX_PRIORITIES
#define OS_MAX_PRIORITIES 32
#endif

#if (OS_MAX_PRIORITIES < 2) || (OS_MAX_PRIORITIES > 256)
#error "OS_MAX_PRIORITIES must be between 2 and 256"
#endif

#define OS_PRIORITY_BITMAP_WORDS ((OS_MAX_PRIORITIES + 31U) / 32U)
#define OS_PRIORITY_EXTRA_COUNT  ((OS_MAX_PRIORITIES > 32U) ? (OS_MAX_PRIORITIES - 32U) : 0U)
#define OS_PRIORITY_EXTRA_WORDS ((OS_MAX_PRIORITIES > 32U) ? ((OS_PRIORITY_EXTRA_COUNT + 31U) / 32U) : 0U)

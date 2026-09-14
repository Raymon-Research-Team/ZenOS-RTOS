#pragma once
/**
 * @file    ZenOS_Version.hpp
 * @brief   ZenOS RTOS — Single Source of Truth for Version Numbers
 *
 * ALL other files (ZenOS.hpp, ZenOS_Port.hpp, ZenOS_Safety.cpp, etc.)
 * must include this header and use OS_VERSION_* macros from here.
 * Do NOT define version numbers in any other file.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.0.2
 */

/* ═══════════════ Version Constants ═══════════════ */

#define OS_VERSION_MAJOR           1
#define OS_VERSION_MINOR           1
#define OS_VERSION_PATCH           0
#define OS_VERSION_PACKED          ((OS_VERSION_MAJOR << 16) | (OS_VERSION_MINOR << 8) | OS_VERSION_PATCH)

#define OS_STR_(x) #x
#define OS_STR(x)  OS_STR_(x)
#define OS_VERSION_STRING  OS_STR(OS_VERSION_MAJOR) "." OS_STR(OS_VERSION_MINOR) "." OS_STR(OS_VERSION_PATCH)

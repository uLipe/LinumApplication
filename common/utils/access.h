/**
 * @file    access.h
 * @brief
 */

//==============================================================================
// Define to prevent recursive inclusion
//==============================================================================
#ifndef __ACCESS_H_
#define __ACCESS_H_

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>

//==============================================================================
//Exported constants
//==============================================================================

//==============================================================================
// Exported macro
//==============================================================================

enum access_level
{
  ACC_LEVEL_NONE = (uint8_t)0x00,
  ACC_LEVEL_USER,
  ACC_LEVEL_SUBMANAGER,
  ACC_LEVEL_MANAGER,
  ACC_LEVEL_ADMIN,
  ACC_LEVEL_ENGINEER,
  ACC_LEVEL_FACTORY,
  ACC_LEVEL_MAX,
};

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* __ACCESS_H_ */


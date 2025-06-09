/**
* @file    system.h
* @brief
*/

//==============================================================================
// Define to prevent recursive inclusion
//==============================================================================
#ifndef __SETTINGS_SYSTEM_H_
#define __SETTINGS_SYSTEM_H_

/* C++ detection */
#ifdef __cplusplus
extern "C"
{
#endif

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
//Exported constants
//==============================================================================

#define OTA_MAX_FILE_NAME_SIZE        (120)
#define OTA_MAX_HASH_SIZE             (80)

//==============================================================================
// Exported macro
//==============================================================================

//==============================================================================
// Exported types
//==============================================================================

union version
{
  uint32_t value; /**< Absolute version value 'bmmssmm' */

  struct
  {
    char modification; /**< Modification 0 to 127 */
    char subversion;   /**< Sub Version 0 to 99 */
    char main;         /**< Main Version 0 to 99 */
    bool is_beta;      /**< Release Flag 0: Not Released or 1: Released */
  };
};


struct update_info
{
  char file[ OTA_MAX_FILE_NAME_SIZE ];
  char hash[ OTA_MAX_HASH_SIZE ];
  uint32_t file_size;
  union version fw_version;
  union version hw_version;
  uint16_t num_attempts;
  uint16_t status; // not used yet
};

//==============================================================================
// Exported variables
//==============================================================================

//==============================================================================
// Exported functions prototypes
//==============================================================================

//==============================================================================
// Exported functions
//==============================================================================

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_SYSTEM_H_ */

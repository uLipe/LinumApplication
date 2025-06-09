/**
* @file    communication.h
* @brief
*/

//==============================================================================
// Define to prevent recursive inclusion
//==============================================================================
#ifndef __SETUP_DATABASE_H_
#define __SETUP_DATABASE_H_

/* C++ detection */
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "system.h"

typedef enum
{
  UART_BAUDRATE_9600 = 0x00,
  UART_BAUDRATE_14400,
  UART_BAUDRATE_19200,
  UART_BAUDRATE_38400,
  UART_BAUDRATE_57600,
  UART_BAUDRATE_115200,
  UART_BAUDRATE_MAX
} UartBaudRate_e;

typedef enum
{
  UART_CONFIG_8N1 = 0,
  UART_CONFIG_8N2,
  UART_CONFIG_8E1,
  UART_CONFIG_8O1,
  UART_CONFIG_MAX
} UartTypeConfig_e;

typedef struct
{
  uint8_t addr;
  UartBaudRate_e baudrate;
  UartTypeConfig_e typeconf;
} conf_mdb_rtu_t;

typedef struct
{
  int16_t correction_factor;
} sensor_trigger_config_t;

typedef struct
{
  uint16_t code;
  uint16_t hw_code;
  uint16_t fw_version;
  uint16_t sn;
} dev_info_t;

struct db_sys_proc
{
  uint8_t mdb_iqc;
  uint32_t uptime;
  int16_t temper;
  int16_t humidity;
};

struct db_sys_conf
{
  conf_mdb_rtu_t modbus;
  /* ... */
  sensor_trigger_config_t temper;
  sensor_trigger_config_t humidity;
  /* ... */
  dev_info_t dev_info;
  /* ... */
  uint16_t check_crc;
};

struct db_sys_update_conf
{
  struct update_info update;
  /* ... */
  uint32_t check_crc;
};

/** ************************** **/

typedef enum
{
  GROUP_SYS_CONF = 0,
  GROUP_SYS_OTA_CONF,
  GROUP_PROC_VAR,
} db_sys_group_e;

typedef enum
{
  SYS_CONF_DEVICE_CODE = 0,
  SYS_CONF_HW_CODE,
  SYS_CONF_FW_VERSION,
  SYS_CONF_SN,
  /*......*/
  SYS_CONF_MDB_ADDR,
  SYS_CONF_MDB_BAUDRATE,
  SYS_CONF_MDB_TYPE_CONFIG,
  SYS_CONF_TEMPER_FACTOR,
  SYS_CONF_HUMID_FACTOR,
} sys_conf_var_ndex_e;

enum db_sys_ota_config_param_id
{
  PARAM_CNFG_OTA_FILE = 0,
  PARAM_CNFG_OTA_HASH,
  PARAM_CNFG_OTA_FILE_SIZE,
  PARAM_CNFG_OTA_FW_VERSION,
  PARAM_CNFG_OTA_HW_VERSION,
  PARAM_CNFG_OTA_NUM_ATTEMPTS,
  PARAM_CNFG_OTA_STATUS,
};

typedef enum
{
  PROC_VAR_MDB_IQC = 0,
  PROC_VAR_UPTIME,
  PROC_VAR_SENSOR_TEMPER,
  PROC_VAR_SENSOR_HUMID,
}sys_proc_var_index_e;

int setup_database_init(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /*__SETUP_DATABASE_H_  */

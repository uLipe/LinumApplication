#include <zephyr/kernel.h> 
#include <zephyr/sys/util.h>

#include "database.h"
#include "setup_database.h"
#include <stdio.h>
#include <stdlib.h>

#define PARAM_FIRMWARE_VERSION   (110)
#define PARAM_HARDWARE_VERSION   (10)
#define PARAM_DEVICE_CODE        (0x0100)

static struct db_sys_conf g_db_sys_conf;       
static struct db_sys_proc g_db_sys_proc;  
static struct db_sys_update_conf g_db_sys_ota_conf;

static const struct db_param g_db_sys_conf_vars[] =
{
    DB_PARAMS_ADD_B16(SYS_CONF_DEVICE_CODE, ACC_LEVEL_USER, VAR_FIELD_NORMAL, "DevCode",   eU16, g_db_sys_conf.dev_info.code,       MIN_U16, MAX_U16,      PARAM_DEVICE_CODE),
    DB_PARAMS_ADD_B16(SYS_CONF_HW_CODE,     ACC_LEVEL_USER, VAR_FIELD_NORMAL, "HdCode",    eU16, g_db_sys_conf.dev_info.hw_code,    MIN_U16, MAX_U16, PARAM_HARDWARE_VERSION),
    DB_PARAMS_ADD_B16(SYS_CONF_FW_VERSION,  ACC_LEVEL_USER, VAR_FIELD_NORMAL, "FmwVer",    eU16, g_db_sys_conf.dev_info.fw_version, MIN_U16, MAX_U16, PARAM_FIRMWARE_VERSION),
    DB_PARAMS_ADD_B16(SYS_CONF_SN,          ACC_LEVEL_USER, VAR_FIELD_NORMAL, "SerialNum", eU16, g_db_sys_conf.dev_info.sn,         MIN_U16, MAX_U16,                      0),
    /* ........................................................................................................................................... */
    DB_PARAMS_ADD_B08(SYS_CONF_MDB_ADDR,        ACC_LEVEL_USER, VAR_FIELD_NORMAL, "MdbAddr",     eU08, g_db_sys_conf.modbus.addr,                                 1,                 247,                  1),
    DB_PARAMS_ADD_B08(SYS_CONF_MDB_BAUDRATE,    ACC_LEVEL_USER, VAR_FIELD_NORMAL, "MdbBaud",     eU08, g_db_sys_conf.modbus.baudrate,            UART_BAUDRATE_9600, UART_BAUDRATE_MAX-1, UART_BAUDRATE_9600),
    DB_PARAMS_ADD_B08(SYS_CONF_MDB_TYPE_CONFIG, ACC_LEVEL_USER, VAR_FIELD_NORMAL, "MdbTypeConf", eU08, g_db_sys_conf.modbus.typeconf,               UART_CONFIG_8N1,   UART_CONFIG_MAX-1,    UART_CONFIG_8E1),
    DB_PARAMS_ADD_B16(SYS_CONF_TEMPER_FACTOR,   ACC_LEVEL_USER, VAR_FIELD_NORMAL, "TemperFact",  eS16, g_db_sys_conf.temper.correction_factor,                 -100,                 100,                  0),
    DB_PARAMS_ADD_B16(SYS_CONF_HUMID_FACTOR,    ACC_LEVEL_USER, VAR_FIELD_NORMAL, "HumiFact",    eS16, g_db_sys_conf.humidity.correction_factor,               -100,                 100,                  0),
};

static const struct db_param g_db_sys_proc_var[] =
{
    DB_PARAMS_ADD_B08(PROC_VAR_MDB_IQC,       ACC_LEVEL_USER, VAR_FIELD_NORMAL, "IQCMdb", eU08, g_db_sys_proc.mdb_iqc,        0,      100, 50),
    DB_PARAMS_ADD_B32(PROC_VAR_UPTIME,        ACC_LEVEL_USER, VAR_FIELD_NORMAL, "Uptime", eU32, g_db_sys_proc.uptime,   MIN_U32,  MAX_U32, 10000),
    DB_PARAMS_ADD_B16(PROC_VAR_SENSOR_TEMPER, ACC_LEVEL_USER, VAR_FIELD_NORMAL, "Temper", eS16, g_db_sys_proc.temper,   MIN_S16,  MAX_S16, 1234),
    DB_PARAMS_ADD_B16(PROC_VAR_SENSOR_HUMID,  ACC_LEVEL_USER, VAR_FIELD_NORMAL, "Humi",   eS16, g_db_sys_proc.humidity, MIN_S16,  MAX_S16, 5678),
};

static const struct db_param g_db_params_sys_update_conf[] =
{
    DB_PARAMS_ADD_STR( PARAM_CNFG_OTA_FILE,         ACC_LEVEL_USER, VAR_FIELD_NORMAL, "CnfgOtaFile",       eSTR, g_db_sys_ota_conf.update.file,                NULL,     NULL,   " "),
    DB_PARAMS_ADD_STR( PARAM_CNFG_OTA_HASH,         ACC_LEVEL_USER, VAR_FIELD_NORMAL, "CnfgOtaHash",       eSTR, g_db_sys_ota_conf.update.hash,                NULL,     NULL,   "Cristina Vieira Coelho!!"),
    DB_PARAMS_ADD_B32( PARAM_CNFG_OTA_FILE_SIZE,    ACC_LEVEL_USER, VAR_FIELD_NORMAL, "CnfOtaFlSize",      eU32, g_db_sys_ota_conf.update.file_size,        MIN_U32,  MAX_U32,     0),
    DB_PARAMS_ADD_B32( PARAM_CNFG_OTA_FW_VERSION,   ACC_LEVEL_USER, VAR_FIELD_NORMAL, "CnfOtaFmwVersion",  eU32, g_db_sys_ota_conf.update.fw_version.value,       0,  MAX_U32,     0),
    DB_PARAMS_ADD_B32( PARAM_CNFG_OTA_HW_VERSION,   ACC_LEVEL_USER, VAR_FIELD_NORMAL, "CnfOtaHwVersion",   eU32, g_db_sys_ota_conf.update.hw_version.value,       0,  MAX_U32,     0),
    DB_PARAMS_ADD_B16( PARAM_CNFG_OTA_NUM_ATTEMPTS, ACC_LEVEL_USER, VAR_FIELD_NORMAL, "CnfOtaNunAttempts", eU16, g_db_sys_ota_conf.update.num_attempts,           0,  MAX_U16,     0),
    DB_PARAMS_ADD_B16( PARAM_CNFG_OTA_STATUS,       ACC_LEVEL_USER, VAR_FIELD_NORMAL, "CnfOtaStatus",      eU16, g_db_sys_ota_conf.update.status,                 0,  MAX_U16,     0),
    /* ..................................................................................................................................................................................... */
};

static struct db_group g_db_grp_sys_conf = DATABASE_CREATE_GROUP(GROUP_SYS_CONF, "SysConfigVar", g_db_sys_conf_vars);
static struct db_group g_db_grp_sys_proc = DATABASE_CREATE_GROUP(GROUP_PROC_VAR,   "SysProcVar",  g_db_sys_proc_var);
static struct db_group g_db_grp_sys_update_config = DATABASE_CREATE_GROUP( GROUP_SYS_OTA_CONF,  "SysOtaConfig",  g_db_params_sys_update_conf );

int setup_database_init(void)
{
  db_init();
  db_group_add( &g_db_grp_sys_conf );
  db_group_add( &g_db_grp_sys_update_config );
  db_group_add( &g_db_grp_sys_proc );
  db_group_load_default(DB_GROUP_SELECT_ALL, ACC_LEVEL_FACTORY);
  return 0;
}
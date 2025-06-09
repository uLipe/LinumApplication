/**
 * @file    system.c
 * @brief
 */

//==============================================================================
// Includes
//==============================================================================

#include "slave_modbus.h"
#include "mdbcomm.h"
#include "mdb_table_parse.h"
#include "setup_database.h"
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/modbus/modbus.h>

#include <string.h>
#include <stdio.h>

//==============================================================================
// Private definitions
//==============================================================================

//==============================================================================
// Private macro
//==============================================================================

//==============================================================================
// Private typedef
//==============================================================================

//==============================================================================
// Extern variables
//==============================================================================

//==============================================================================
// Private variables
//==============================================================================

const mdb_slv_reg_t g_input_reg_table_1[] =
{
    MDBSLV_ADD_REG(GROUP_PROC_VAR, PROC_VAR_SENSOR_TEMPER, 001),
    MDBSLV_ADD_REG(GROUP_PROC_VAR, PROC_VAR_SENSOR_HUMID,  002),
    /* ............................................................. */
    MDBSLV_ADD_REG(GROUP_PROC_VAR, PROC_VAR_MDB_IQC,     200),
    MDBSLV_ADD_REG(GROUP_SYS_CONF, SYS_CONF_DEVICE_CODE, 201),
    MDBSLV_ADD_REG(GROUP_SYS_CONF, SYS_CONF_HW_CODE,     202),
    MDBSLV_ADD_REG(GROUP_SYS_CONF, SYS_CONF_FW_VERSION,  203),
    MDBSLV_ADD_REG(GROUP_SYS_CONF, SYS_CONF_SN,          204),
    MDBSLV_ADD_REG(GROUP_PROC_VAR, PROC_VAR_UPTIME,      205),
};

const mdb_slv_reg_t g_holding_reg_table_1[] =
{
    MDBSLV_ADD_REG(GROUP_SYS_CONF, SYS_CONF_MDB_ADDR,        257),
    MDBSLV_ADD_REG(GROUP_SYS_CONF, SYS_CONF_MDB_BAUDRATE,    258),
    MDBSLV_ADD_REG(GROUP_SYS_CONF, SYS_CONF_MDB_TYPE_CONFIG, 259),
    MDBSLV_ADD_REG(GROUP_SYS_CONF, SYS_CONF_HUMID_FACTOR,    260),
    MDBSLV_ADD_REG(GROUP_SYS_CONF, SYS_CONF_TEMPER_FACTOR,   261),
};

const struct mdb_slv_table g_mdb_slv_rd_table_1 = MDBSLV_CREATE_TABLE("CnfgVar", ACC_LEVEL_USER, g_input_reg_table_1);
const struct mdb_slv_table g_mdb_slv_wr_table_1 = MDBSLV_CREATE_TABLE("ProcVar", ACC_LEVEL_USER, g_holding_reg_table_1);

//==============================================================================

static int input_reg_rd_rs485(uint16_t addr, uint16_t* reg, uint16_t reg_qty) 
{
    return mdb_slave_parse_read_register(&g_mdb_slv_rd_table_1, (char*)reg, addr, reg_qty, ACC_LEVEL_FACTORY);
}

static int holding_reg_rd_rs485(uint16_t addr, uint16_t* reg, uint16_t reg_qty) 
{
    return mdb_slave_parse_read_register(&g_mdb_slv_wr_table_1, (char*)reg, addr, reg_qty, ACC_LEVEL_FACTORY);
}

static int holding_reg_wr_rs485(uint16_t addr, uint16_t* reg, uint16_t reg_qty) 
{
    return mdb_slave_parse_write_register(&g_mdb_slv_wr_table_1, (char*)reg, addr, reg_qty, ACC_LEVEL_FACTORY);
}

static struct modbus_user_callbacks modbus_callback_rs485 = {
    .input_reg_rd = input_reg_rd_rs485,
    .holding_reg_rd = holding_reg_rd_rs485,
    .holding_reg_wr = holding_reg_wr_rs485,
};

const char* iface_name_rs485 = DT_PROP(DT_NODELABEL(modbus1), label);

static struct modbus_iface_param server_param_rs485 = {
    .mode = MODBUS_MODE_RTU,
    .server =
    {
        .user_cb = &modbus_callback_rs485,
        .unit_id = 1,
    },
    .serial =
    {
        .baud = 115200,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits_client = UART_CFG_STOP_BITS_1,
    },
};

//==============================================================================
// Private function prototypes
//==============================================================================

//==============================================================================
// Private functions
//==============================================================================

//==============================================================================
// Exported functions
//==============================================================================

int slave_modbus_init(void)
{
    int ret;
    int iface;
    iface = modbus_iface_get_by_name(iface_name_rs485);
    if (iface < 0) {
		printk("Failed to get iface index for %s\n", iface_name_rs485);
		return iface;
	}

    ret = modbus_init_server(iface, server_param_rs485);
    if (ret != 0) {
      printk("FC06 failed with %d\n", ret);
      return ret;
    }

    return ret;
}
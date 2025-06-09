/**
 * @file    mdb_tables.h
 * @brief
 */

//==============================================================================
// Define to prevent recursive inclusion
//==============================================================================

#ifndef _MDB_TABLES_PARSE_H_
#define	_MDB_TABLES_PARSE_H_

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// Includes
//==============================================================================

// #include "error/error.h"
// #include "xtimer/xtimer.h"
// #include "../mb.h"
#include <stdbool.h>
#include <stdint.h>
#include "mdbcomm.h"

//==============================================================================
//Exported constants
//==============================================================================

//==============================================================================
// Exported macro
//==============================================================================

#define MDBSLV_ADD_REG( _group, _param, _addr )  { .group_id = _group, .param_id = _param, .addr = _addr }
#define MDBSLV_TABLE_LEN( array )                ( sizeof( array ) / sizeof( array[0] ) )
#define MDBSLV_CREATE_TABLE( _name, _access, _table )  \
{                                                      \
  .table_name = _name,                                 \
  .acess = _access,                                    \
  .len = MDBSLV_TABLE_LEN( _table ),                   \
  .regs =  _table,                                     \
}

//==============================================================================
// Exported types
//==============================================================================

//==============================================================================
// Exported variables
//==============================================================================

//==============================================================================
// Exported functions prototypes
//==============================================================================

eMBErrorCode mdb_slave_parse_write_register( const struct mdb_slv_table *wr_table,
                                                     char *buf, uint16_t addr, uint16_t num_regs,
                                                     enum access_level acess );
eMBErrorCode mdb_slave_parse_read_register( const struct mdb_slv_table *rd_table, char *buf,
                                             uint16_t addr, uint16_t num_regs,
                                             enum access_level access );

//==============================================================================
// Exported functions
//==============================================================================

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* _MDB_TABLES_PARSE_H_ */

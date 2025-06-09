/**
* @file    mdbcomm.h
* @brief   Common variables and constans to use modbus master and slave
*/

//==============================================================================
// Define to prevent recursive inclusion
//==============================================================================
#ifndef __MDBCOMM_H_
#define __MDBCOMM_H_

//==============================================================================
// Includes
//==============================================================================

/* C++ detection */
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "access.h"

//==============================================================================
//Exported constants
//==============================================================================

// #define constMB_PDU_SIZE_MAX                        ( 253 ) /* Maximum size of a PDU.             */
// #define constMB_PDU_SIZE_MIN                        ( 1 )   /* Function Code                      */
// #define constMB_PDU_FUNC_OFF                        ( 0 )   /* Offset of function code in PDU.    */
// #define constMB_PDU_DATA_OFF                        ( 1 )   /* Offset for response data in PDU.   */

// #define constMB_SER_PDU_SIZE_MIN                    ( 4 )   /* Minimum size of a Modbus RTU frame.*/
// #define constMB_SER_PDU_SIZE_MAX                    ( 256 ) /* Maximum size of a Modbus RTU frame.*/
// #define constMB_SER_PDU_SIZE_CRC                    ( 2 )   /* Size of CRC field in PDU.          */
// #define constMB_SER_PDU_ADDR_OFF                    ( 0 )   /* Offset of slave address in Ser-PDU.*/
// #define constMB_SER_PDU_PDU_OFF                     ( 1 )   /* Offset of Modbus-PDU in Ser-PDU.   */

// #define constMB_ADDRESS_BROADCAST                   ( 0 )   /* Modbus broadcast address.          */
// #define constMB_ADDRESS_MIN                         ( 1 )   /* Smallest possible slave address.   */
// #define constMB_ADDRESS_MAX                         ( 247 ) /* Biggest possible slave address.    */
// #define constMB_FUNC_NONE                           ( 0 )
// #define constMB_FUNC_READ_COILS                     ( 1 )
// #define constMB_FUNC_READ_DISCRETE_INPUTS           ( 2 )
// #define constMB_FUNC_WRITE_SINGLE_COIL              ( 5 )
// #define constMB_FUNC_WRITE_MULTIPLE_COILS           ( 15 )
// #define constMB_FUNC_READ_HOLDING_REGISTER          ( 3 )
// #define constMB_FUNC_READ_INPUT_REGISTER            ( 4 )
// #define constMB_FUNC_WRITE_REGISTER                 ( 6 )
// #define constMB_FUNC_WRITE_MULTIPLE_REGISTERS       ( 16 )
// #define constMB_FUNC_READWRITE_MULTIPLE_REGISTERS   ( 23 )
// #define constMB_FUNC_DIAG_READ_EXCEPTION            ( 7 )
// #define constMB_FUNC_DIAG_DIAGNOSTIC                ( 8 )
// #define constMB_FUNC_DIAG_GET_COM_EVENT_CNT         ( 11 )
// #define constMB_FUNC_DIAG_GET_COM_EVENT_LOG         ( 12 )
// #define constMB_FUNC_OTHER_REPORT_SLAVEID           ( 17 )
// #define constMB_FUNC_ERROR                          ( 128 )

//==============================================================================
// Exported macro
//==============================================================================

//==============================================================================
// Exported types
//==============================================================================

// typedef enum
// {
//   /* Setting commands */
//   MDB_MST_CMD_NONE,
//   MDB_MST_CMD_INIT_DB,
//   MDB_MST_CMD_INIT_LIB,
//   MDB_MST_CMD_DEINIT_LIB,
//   MDB_MST_CMD_ENABLE_LIB,
//   MDB_MST_CMD_DISABLE_LIB,
//   MDB_MST_CMD_DEINIT,

//   /* Operation commands */
//   MDB_MST_CMD_READ_COILS,
//   MDB_MST_CMD_WRITE_COILS,
//   MDB_MST_CMD_WRITE_MULTIPLE_COILS,
//   MDB_MST_CMD_READ_DISCRETE_INPUTS,
//   MDB_MST_CMD_WRITE_HOLDING_REG,
//   MDB_MST_CMD_WRITE_MULT_HOLDING_REG,
//   MDB_MST_CMD_READ_HOLDING_REG,
//   MDB_MST_CMD_READ_WRITE_MULT_HOLDING_REG,
//   MDB_MST_CMD_READ_INPUT_REG,

//   MDB_MST_CMD_LIMIT,
// } eMdbMasterHndCmd_t;

// typedef enum
// {
//   MB_MRE_NO_ERR, /* no error. */
//   MB_MRE_NO_REG, /* illegal register address. */
//   MB_MRE_ILL_ARG, /* illegal argument. */
//   MB_MRE_REV_DATA, /* receive data error. */
//   MB_MRE_TIMEDOUT, /* timeout error occurred. */
//   MB_MRE_MASTER_BUSY, /* master is busy now. */
//   MB_MRE_EXE_FUN /* execute function error. */
// } eMBMasterReqErrCode;

// typedef enum
// {
//   STATE_ENABLED,
//   STATE_DISABLED,
//   STATE_NOT_INITIALIZED
// } eMDBMasteState;

// typedef enum
// {
//   MB_REG_READ, /* Read register values and pass to protocol stack. */
//   MB_REG_WRITE /* Update register values. */
// } eMBRegisterMode;

// typedef enum
// {
//   MB_EX_NONE = 0x00,
//   MB_EX_ILLEGAL_FUNCTION = 0x01,
//   MB_EX_ILLEGAL_DATA_ADDRESS = 0x02,
//   MB_EX_ILLEGAL_DATA_VALUE = 0x03,
//   MB_EX_SLAVE_DEVICE_FAILURE = 0x04,
//   MB_EX_ACKNOWLEDGE = 0x05,
//   MB_EX_SLAVE_BUSY = 0x06,
//   MB_EX_MEMORY_PARITY_ERROR = 0x08,
//   MB_EX_GATEWAY_PATH_FAILED = 0x0a,
//   MB_EX_GATEWAY_TGT_FAILED = 0x0b
// } eMBException;


typedef enum
{
  MB_ENOERR,    /* no error. */
  MB_ENOREG,    /* illegal register address. */
  MB_EINVAL,    /* illegal argument. */
  MB_EPORTERR,  /* porting layer error. */
  MB_ENORES,    /* insufficient resources. */
  MB_EIO,       /* I/O error. */
  MB_EILLSTATE, /* protocol stack in illegal state. */
  MB_ETIMEDOUT  /* timeout error occurred. */
} eMBErrorCode;

typedef struct  __attribute__((packed))
{
  uint32_t group_id;
  uint32_t param_id;
  uint16_t addr;
}mdb_slv_reg_t;

struct mdb_slv_table
{
  const char *table_name;
  const enum access_level acess;
  const uint16_t len;
  const mdb_slv_reg_t *regs;
};

typedef struct  __attribute__((packed))
{
  uint16_t tot_regs;
  uint16_t index_buf;
  uint16_t curr_index_table;
  uint16_t curr_addr;
} mdb_slv_var_t;

//==============================================================================
// Exported variables
//==============================================================================

//==============================================================================
// Exported functions prototypes
//==============================================================================

//==============================================================================
// Exported functions
//==============================================================================

static __inline__ uint16_t be16dec( const void *pp )
{
  uint8_t const *p = ( uint8_t const* ) pp;

  return ( ( p[ 0 ] << 8 ) | p[ 1 ] );
}

static __inline__ uint32_t be32dec( const void *pp )
{
  uint8_t const *p = ( uint8_t const* ) pp;

  return ( ( ( unsigned ) p[ 0 ] << 24 ) | ( p[ 1 ] << 16 ) | ( p[ 2 ] << 8 ) | p[ 3 ] );
}

static __inline__ uint64_t be64dec( const void *pp )
{
  uint8_t const *p = ( uint8_t const* ) pp;

  return ( ( ( uint64_t ) be32dec( p ) << 32 ) | be32dec( p + 4 ) );
}

static __inline__ uint16_t le16dec( const void *pp )
{
  uint8_t const *p = ( uint8_t const* ) pp;

  return ( ( p[ 1 ] << 8 ) | p[ 0 ] );
}

static __inline__ uint32_t le32dec( const void *pp )
{
  uint8_t const *p = ( uint8_t const* ) pp;

  return ( ( ( unsigned ) p[ 3 ] << 24 ) | ( p[ 2 ] << 16 ) | ( p[ 1 ] << 8 ) | p[ 0 ] );
}

static __inline__ uint64_t le64dec( const void *pp )
{
  uint8_t const *p = ( uint8_t const* ) pp;

  return ( ( ( uint64_t ) le32dec( p + 4 ) << 32 ) | le32dec( p ) );
}

static __inline__ void be16enc( void *pp, uint16_t u )
{
  uint8_t *p = ( uint8_t* ) pp;

  p[ 0 ] = ( u >> 8 ) & 0xff;
  p[ 1 ] = u & 0xff;
}

static __inline__ void be32enc( void *pp, uint32_t u )
{
  uint8_t *p = ( uint8_t* ) pp;

  p[ 0 ] = ( u >> 24 ) & 0xff;
  p[ 1 ] = ( u >> 16 ) & 0xff;
  p[ 2 ] = ( u >> 8 ) & 0xff;
  p[ 3 ] = u & 0xff;
}

static __inline__ void be64enc( void *pp, uint64_t u )
{
  uint8_t *p = ( uint8_t* ) pp;

  be32enc( p, ( uint32_t ) ( u >> 32 ) );
  be32enc( p + 4, ( uint32_t ) ( u & 0xffffffffU ) );
}

static __inline__ void le16enc( void *pp, uint16_t u )
{
  uint8_t *p = ( uint8_t* ) pp;

  p[ 0 ] = u & 0xff;
  p[ 1 ] = ( u >> 8 ) & 0xff;
}

static __inline__ void le32enc( void *pp, uint32_t u )
{
  uint8_t *p = ( uint8_t* ) pp;

  p[ 0 ] = u & 0xff;
  p[ 1 ] = ( u >> 8 ) & 0xff;
  p[ 2 ] = ( u >> 16 ) & 0xff;
  p[ 3 ] = ( u >> 24 ) & 0xff;
}

static __inline__ void le64enc( void *pp, uint64_t u )
{
  uint8_t *p = ( uint8_t* ) pp;

  le32enc( p, ( uint32_t ) ( u & 0xffffffffU ) );
  le32enc( p + 4, ( uint32_t ) ( u >> 32 ) );
}

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* __MDBCOMM_H_ */


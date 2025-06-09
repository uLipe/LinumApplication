/**
 * @file    mdb_table_parse
 * @brief   Lib with mount and parse the modbus tables.
 */

//==============================================================================
// Includes
//==============================================================================
#include "mdb_table_parse.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "database.h"
#include <zephyr/sys/byteorder.h>

//==============================================================================
// Private definitions
//==============================================================================

LOG_MODULE_REGISTER(mdb_table_parse);

#define MDB_PRS_NUM_BYTES_U8       (1) /** Define to represent the size of uint8 variable */
#define MDB_PRS_NUM_BYTES_U16      (2) /** Define to represent the size of uint16 variable */
#define MDB_PRS_NUM_BYTES_U32      (4) /** Define to represent the size of uint32 variable */
#define MDB_PRS_NUM_BYTES_U64      (8) /** Define to represent the size of uint64 variable */

#define MDB_PRS_USE_ONE_REGISTER   (1) /** Use one register modbus (2 bytes) */
#define MDB_PRS_USE_TWO_REGISTER   (2) /** Use two register modbus (4 bytes) */
#define MDB_PRS_USE_FOUR_REGISTER  (4) /** Use four register modbus (8 bytes) */
#define MDB_PRS_USE_FIVE_REGISTER  (5) /** Use five register modbus (10 bytes) */

//==============================================================================
// Private macro
//==============================================================================

//==============================================================================
// Private typedef
//==============================================================================

//==============================================================================
// Private variables
//==============================================================================

//==============================================================================
// Private function prototypes
//==============================================================================

static uint8_t mdb_slv_get_num_regs_used( uint16_t size_param );
static int mdb_slv_parse_string_and_set_register( const mdb_slv_reg_t *reg, char *buf,
                                                      uint16_t num_var_table,
                                                      enum access_level access );
static int mdb_slv_search_reg( const mdb_slv_reg_t *reg, uint16_t table_len, uint16_t reg_fnd,
                                   uint16_t *index_table );
static int mdb_slv_check_list_regs( const mdb_slv_reg_t *table, uint16_t table_len,
                                        uint16_t reg_fnd, uint16_t index_table, uint16_t num_regs );
static int mdb_slv_get_register_and_mount_string( const mdb_slv_reg_t *reg, char *buf,
                                                      uint16_t num_var_table );

//==============================================================================
// Private functions
//==============================================================================

/**
 * @brief Return the number of modbus register that every variable use.
 * @param pVar
 * @return
 */
static uint8_t mdb_slv_get_num_regs_used( uint16_t size_param )
{
  uint8_t num_regs_used;

  switch( size_param )
  {
    case MDB_PRS_NUM_BYTES_U8:
    case MDB_PRS_NUM_BYTES_U16: { num_regs_used = MDB_PRS_USE_ONE_REGISTER; break; }
    case MDB_PRS_NUM_BYTES_U32: { num_regs_used = MDB_PRS_USE_TWO_REGISTER; break; }
    case MDB_PRS_NUM_BYTES_U64: { num_regs_used = MDB_PRS_USE_FOUR_REGISTER; break; }
    default:
    {
      num_regs_used = ( ( size_param % sizeof(uint16_t) ) ) ?
                      ( ( size_param + 1 ) / sizeof(uint16_t) ) :
                      ( size_param / sizeof(uint16_t) );
      break;
    }
  }

  return num_regs_used;
}

static int mdb_slv_parse_string_and_set_register( const mdb_slv_reg_t *reg, char *buf,
                                                      uint16_t num_var_table,
                                                      enum access_level access )
{
  int err = -EINVAL;
  mdb_slv_var_t ctrl = { 0 };
  struct db_group *group;
  struct db_param *param;

  for( ctrl.tot_regs = 0; ctrl.tot_regs < num_var_table; )
  {
    err = db_get_var_config( &group, &param, reg->group_id, reg->param_id);
    if( err )
    {
      return err;
    }

    switch( param->config.info.type )
    {
      case eS08:
      {
        int8_t temp_s8;
        temp_s8 = be16dec( &buf[ ctrl.index_buf ] ) & 0x00FF;
        err = db_param_set_s8(access, param, temp_s8);
        ctrl.index_buf += 1;
        break;
      }
      case eBOL:
      case eU08:
      {
        uint8_t temp_u8;
        temp_u8 = be16dec( &buf[ ctrl.index_buf ] ) & 0x00FF;
        err = db_param_set_u8(access, param, temp_u8);
        ctrl.index_buf += 1;
        break;
      }
      case eS16:
      {
        int16_t temp_s16;
        temp_s16 = be16dec( &buf[ ctrl.index_buf ] );
        err = db_param_set_s16(access, param, temp_s16);
        break;
      }
      case eU16:
      {
        uint16_t temp_u16;
        temp_u16 = be16dec( &buf[ ctrl.index_buf ] );
        err = db_param_set_u16(access, param, temp_u16);
        break;
      }
      case eS32:
      {
        int32_t temp_s32;
        temp_s32 = be32dec( &buf[ ctrl.index_buf ] );
        err = db_param_set_s32(access, param, temp_s32);
        break;
      }
      case eU32:
      {
        uint32_t temp_u32;
        temp_u32 = be32dec( &buf[ ctrl.index_buf ] );
        err = db_param_set_u32(access, param, temp_u32);
        break;
      }
      case eF32:
      {
        float temp_f32;
        temp_f32 = be32dec( &buf[ ctrl.index_buf ] );
        err = db_param_set_float(access, param, temp_f32);
        break;
      }
#if defined(TYPEDEF_ENABLE_VAR_B64)
      case eS64:
      {
        int64_t temp_s64;
        temp_s64 = be64dec( &buf[ ctrl.index_buf ] );
        err = db_param_set_s64(access, param, temp_s64);
        break;
      }
      case eU64:
      {
        uint64_t temp_u64;
        temp_u64 = be64dec( &buf[ ctrl.index_buf ] );
        err = db_param_set_u64(access, param, temp_u64);
        break;
      }
      case eF64:
      {
        double temp_f64;
        temp_f64 = be64dec( &buf[ ctrl.index_buf ] );
        err = db_param_set_double(access, param, temp_f64);
        break;
      }
#endif
      case eSTR:
      {
        err = db_param_set_str(access, param, &buf[ ctrl.index_buf ], sizeof(buf[ ctrl.index_buf ]));
        break;
      }
      default:
      {
        err = -EINVAL;
        break;
      }
    }

    if( err < 0 )
    {
      return err;
    }

    ctrl.tot_regs += mdb_slv_get_num_regs_used( param->config.var_size );
    ctrl.index_buf += param->config.var_size;
    reg++;
  }

  if( err == 1 )
  {
    err = 0;
  }

  return err;
}

static int mdb_slv_search_reg( const mdb_slv_reg_t *reg, uint16_t table_len,
                                   uint16_t reg_fnd, uint16_t *index_table )
{
  int find_result;
  uint16_t idx_min;
  uint16_t idx_cur;
  uint16_t idx_max;
  uint16_t reg_min;
  uint16_t reg_max;
  uint16_t reg_cur;

  idx_min = 0;
  idx_max = ( table_len - 1 );
  reg_min = reg[ idx_min ].addr;
  reg_max = reg[ idx_max ].addr;
  find_result = -ENOENT;

  /* Check if find register is into the checked map to enable the search loop */
  if( ( reg_fnd < reg_min ) || ( reg_fnd > reg_max ) )
  {
    return -ENOENT;
  }

  /* Register search process */
  while( find_result == -ENOENT )
  {
    /* Loop loading */
    idx_cur = ( idx_max + idx_min ) / 2;
    reg_cur = reg[ idx_cur ].addr;

    /* Register check */
    if( idx_max < idx_min )
    {
      find_result = -ENOENT;
    }
    else if( reg_cur < reg_fnd )
    {
      idx_min = idx_cur + 1;
    }
    else if( reg_cur > reg_fnd )
    {
      idx_max = idx_cur - 1;
    }
    else
    {
      find_result = 0;
    }
  }

  /* Check the finding result */
  if( find_result == 0 )
  {
    *index_table = idx_cur;
  }

  /* Function return */
  return find_result;
}

static int mdb_slv_check_list_regs( const mdb_slv_reg_t *reg, uint16_t table_len,
                                        uint16_t reg_fnd, uint16_t index_table, uint16_t num_regs )
{
  int err;
  mdb_slv_var_t ctrl = { 0 };
  mdb_slv_reg_t *reg_var;
  uint16_t tot_vars;
  struct db_group *group;
  struct db_param *param;

  reg_var = ( mdb_slv_reg_t* ) reg;
  ctrl.curr_index_table = index_table;
  ctrl.curr_addr = reg_var->addr;
  tot_vars = table_len;

  while( 1 )
  {
    err = db_get_var_config( &group, &param, reg_var->group_id, reg_var->param_id);
    if( err != 0 )
    {
      return err;
    }

    ctrl.tot_regs += mdb_slv_get_num_regs_used( param->config.var_size );
    if( ctrl.tot_regs == num_regs )
    {
      return 0;
    }
    else if( ctrl.tot_regs > num_regs )
    {
      return -EOVERFLOW;
    }

    ctrl.curr_index_table++;
    if( ctrl.curr_index_table >= tot_vars )
    {
      return -EOVERFLOW;
    }

    reg_var++;
    if(ctrl.curr_addr + mdb_slv_get_num_regs_used( param->config.var_size ) != reg_var->addr)
    {
      return -EINVAL;
    }

    ctrl.curr_addr = reg_var->addr;
  }

  /* Function return */
  return err;
}

static int mdb_slv_get_register_and_mount_string( const mdb_slv_reg_t *reg, char *buf,
                                                      uint16_t num_var_table )
{
  int err = -EINVAL;
  mdb_slv_var_t ctrl = { 0 };
  struct db_group *group;
  struct db_param *param;

  for( ctrl.tot_regs = 0; ctrl.tot_regs < num_var_table; )
  {
    err = db_get_var_config( &group, &param, reg->group_id, reg->param_id);
    if(err)
    {
      return err;
    }

    switch( param->config.info.type )
    {
      case eS08:
      {
        int8_t tmp_s8;
        err = db_param_get_s8( ACC_LEVEL_FACTORY, param, &tmp_s8 );
        be16enc( &buf[ ctrl.index_buf ], tmp_s8 );
        ctrl.index_buf += 1;
        break;
      }
      case eBOL:
      case eU08:
      {
        uint8_t tmp_u8;
        err = db_param_get_u8( ACC_LEVEL_FACTORY, param, &tmp_u8 );
        be16enc( &buf[ ctrl.index_buf ], tmp_u8 );
        ctrl.index_buf += 1;
        break;
      }
      case eS16:
      {
        int16_t tmp_s16;
        err = db_param_get_s16( ACC_LEVEL_FACTORY, param, &tmp_s16 );
        be16enc( &buf[ ctrl.index_buf ], tmp_s16 );
        break;
      }
      case eU16:
      {
        uint16_t tmp_u16;
        err = db_param_get_u16( ACC_LEVEL_FACTORY, param, &tmp_u16 );
        be16enc( &buf[ ctrl.index_buf ], tmp_u16 );
        break;
      }
      case eS32:
      {
        int32_t tmp_s32;
        err = db_param_get_s32( ACC_LEVEL_FACTORY, param, &tmp_s32 );
        be32enc( &buf[ ctrl.index_buf ], tmp_s32 );
        break;
      }
      case eU32:
      {
        uint32_t tmp_u32;
        err = db_param_get_u32( ACC_LEVEL_FACTORY, param, &tmp_u32 );
        be32enc( &buf[ ctrl.index_buf ], tmp_u32 );
        break;
      }
      case eF32:
      {
        float tmp_f32;
        err = db_param_get_float( ACC_LEVEL_FACTORY, param, &tmp_f32 );
        be32enc( &buf[ ctrl.index_buf ], tmp_f32 );
        break;
      }
#if defined(TYPEDEF_ENABLE_VAR_B64)
      case eS64:
      {
        int64_t tmp_s64;
        err = db_param_get_s64( ACC_LEVEL_FACTORY, param, &tmp_s64 );
        be64enc( &buf[ ctrl.index_buf ], tmp_s64 );
        break;
      }
      case eU64:
      {
        uint64_t tmp_u64;
        err = db_param_get_u64( ACC_LEVEL_FACTORY, param, &tmp_u64 );
        be64enc( &buf[ ctrl.index_buf ], tmp_u64 );
        break;
      }
      case eF64:
      {
        double tmp_f64;
        err = db_param_get_double( ACC_LEVEL_FACTORY, param, &tmp_f64 );
        be64enc( &buf[ ctrl.index_buf ], tmp_f64 );
        break;
      }
#endif
      case eSTR:
      {
        err = db_param_get_str( ACC_LEVEL_FACTORY, param, &buf[ ctrl.index_buf ], param->config.var_size );
        break;
      }
      default:
      {
        err = -EINVAL;
        break;
      }
    }

    if( err < 0 )
    {
      break;
    }

    ctrl.tot_regs += mdb_slv_get_num_regs_used( param->config.var_size );
    ctrl.index_buf += param->config.var_size;
    reg++;
  }

  if( err > 0 )
  {
    err = 0;
  }

  return err;
}

//==============================================================================
// Exported functions
//==============================================================================

eMBErrorCode mdb_slave_parse_write_register( const struct mdb_slv_table *wr_table,
                                                     char *buf, uint16_t addr, uint16_t num_regs,
                                                     enum access_level access )
{
  int err;
  uint16_t index_reg;

  index_reg = 0;

  err = mdb_slv_search_reg( wr_table->regs, wr_table->len, addr, &index_reg );
  if( err != 0 )
  {
    LOG_ERR( "(X) Register not found! \r\n" );
    return MB_ENOREG;
  }

  err = mdb_slv_check_list_regs( &wr_table->regs[ index_reg ], wr_table->len, addr, index_reg, num_regs );
  if( err != 0 )
  {
    LOG_ERR( "(X) Invalid read size! \r\n" );
    return MB_EPORTERR;
  }

  err = mdb_slv_parse_string_and_set_register( &wr_table->regs[ index_reg ], buf, num_regs, access );
  if( err == -ERANGE )
  {
    LOG_ERR( "Value out of range! \r\n" );
    return MB_EINVAL;
  }
  else if( err )
  {
    LOG_ERR( "Mount message error! \r\n" );
    return MB_EPORTERR;
  }

  return MB_ENOERR;
}

eMBErrorCode mdb_slave_parse_read_register( const struct mdb_slv_table *rd_table, char *buf,
                                             uint16_t addr, uint16_t num_regs,
                                             enum access_level access )
{
  int err;
  uint16_t index_reg;

  err = mdb_slv_search_reg( rd_table->regs, rd_table->len, addr, &index_reg );
  if( err != 0 )
  {
    LOG_ERR( "Register %d not found, size: %d! \r\n", addr, num_regs );
    return MB_ENOREG;
  }

  err = mdb_slv_check_list_regs( &rd_table->regs[ index_reg ], rd_table->len, addr, index_reg, num_regs );
  if( err )
  {
    LOG_ERR( "Invalid read size! \r\n" );
    return MB_EPORTERR;
  }

  err = mdb_slv_get_register_and_mount_string( &rd_table->regs[ index_reg ], buf, num_regs );
  if( err )
  {
    LOG_ERR( "Mount message error! \r\n" );
    return MB_EPORTERR;
  }

  return MB_ENOERR;
}

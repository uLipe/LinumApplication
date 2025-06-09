/**
* @file    communication.h
* @brief
*/

//==============================================================================
// Define to prevent recursive inclusion
//==============================================================================
#ifndef __DATABASE_H_
#define __DATABASE_H_

/* C++ detection */
#ifdef __cplusplus
extern "C"
{
#endif

//==============================================================================
// Includes
//==============================================================================

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include "access.h"
#include "typedefs.h"
#include <stdint.h>
#include <stdbool.h>

//==============================================================================
//Exported constants
//==============================================================================

#define DB_GROUP_FILTER_DISABLE               (0xFFFF)
#define DB_GROUP_SELECT_ALL                   (0xFFFF)
#define DB_UPDATED                            1

//==============================================================================
// Exported macro
//==============================================================================

#define ARRAY_LENGTH( array )     ( sizeof( array ) / sizeof( array[0] ) )

#define DB_PARAMS_ADD_STR(_param_id, _type_access, _type_field, _param_name, _data_type,                \
                          _variable, _Min_value, _Max_value, _def_value ){                              \
    .id                      = _param_id,                                                               \
    .config.info.access      = _type_access,                                                            \
    .config.info.type        = _data_type,                                                              \
    .config.info.field       = _type_field,                                                             \
    .name                    = _param_name,                                                             \
    .var                     = &_variable,                                                              \
    .config.var_size         = sizeof(_variable),                                                       \
    .config.str.min          = _Min_value,                                                              \
    .config.str.max          = _Max_value,                                                              \
    .config.str.standar      = _def_value }

#define DB_PARAMS_ADD_B08(_param_id, _type_access, _type_field, _param_name, _data_type,                \
                          _variable, _Min_value, _Max_value, _def_value ){                              \
    .id                      = _param_id,                                                               \
    .config.info.access      = _type_access,                                                            \
    .config.info.type        = _data_type,                                                              \
    .config.info.field       = _type_field,                                                             \
    .name                    = _param_name,                                                             \
    .var                     = &_variable,                                                              \
    .config.var_size         = sizeof(_variable),                                                       \
    .config.s8.min           = _Min_value,                                                              \
    .config.s8.max           = _Max_value,                                                              \
    .config.s8.standar       = _def_value }

#define DB_PARAMS_ADD_B16(_param_id, _type_access, _type_field, _param_name, _data_type,                \
                          _variable, _Min_value, _Max_value, _def_value ){                              \
    .id                       = _param_id,                                                              \
    .config.info.access       = _type_access,                                                           \
    .config.info.type         = _data_type,                                                             \
    .config.info.field        = _type_field,                                                            \
    .name                     = _param_name,                                                            \
    .var                      = &_variable,                                                             \
    .config.var_size          = sizeof(_variable),                                                      \
    .config.s16.min           = _Min_value,                                                             \
    .config.s16.max           = _Max_value,                                                             \
    .config.s16.standar       = _def_value }

#define DB_PARAMS_ADD_B32(_param_id, _type_access, _type_field, _param_name, _data_type,                \
                          _variable, _Min_value, _Max_value, _def_value ){                              \
    .id                       = _param_id,                                                              \
    .config.info.access       = _type_access,                                                           \
    .config.info.type         = _data_type,                                                             \
    .config.info.field        = _type_field,                                                            \
    .name                     = _param_name,                                                            \
    .var                      = &_variable,                                                             \
    .config.var_size          = sizeof(_variable),                                                      \
    .config.s32.min           = _Min_value,                                                             \
    .config.s32.max           = _Max_value,                                                             \
    .config.s32.standar       = _def_value }

#define DB_PARAMS_ADD_F32(_param_id, _type_access, _type_field, _param_name, _data_type,                \
                          _variable, _Min_value, _Max_value, _def_value ){                              \
    .id                       = _param_id,                                                              \
    .config.info.access       = _type_access,                                                           \
    .config.info.type         = _data_type,                                                             \
    .config.info.field        = _type_field,                                                            \
    .name                     = _param_name,                                                            \
    .var                      = &_variable,                                                             \
    .config.var_size          = sizeof(_variable),                                                      \
    .config.f32.min           = _Min_value,                                                             \
    .config.f32.max           = _Max_value,                                                             \
    .config.f32.standar       = _def_value }

#if defined(TYPEDEF_ENABLE_VAR_B64)
#define DB_PARAMS_ADD_B64(_param_id, _type_access, _type_field, _param_name, _data_type,                \
                          _variable, _Min_value, _Max_value, _def_value ){                              \
    .id                       = _param_id,                                                              \
    .config.info.access       = _type_access,                                                           \
    .config.info.type         = _data_type,                                                             \
    .config.info.field        = _type_field,                                                            \
    .name                     = _param_name,                                                            \
    .var                      = &_variable,                                                             \
    .config.var_size          = sizeof(_variable),                                                      \
    .config.s64.min           = _Min_value,                                                             \
    .config.s64.max           = _Max_value,                                                             \
    .config.s64.standar       = _def_value }
#endif

#define DATABASE_CREATE_GROUP(_group_id, _name_group, _db_params) \
{                                                                 \
  .id = _group_id,                                                \
  .name = _name_group,                                            \
  .count = ARRAY_LENGTH(_db_params),                              \
  .params = _db_params,                                           \
  .info_group = {0},                                              \
  .node = {0},                                                    \
}

//==============================================================================
// Exported types
//==============================================================================

typedef uint16_t db_group_id_t;
typedef uint16_t db_param_id_t;

union db_param_info
{
  uint32_t mask_bits;

  struct
  {
    enum variable_field field :4;
    enum variable_type type   :6;
    enum access_level access  :4;
  };
};

union db_group_config
{
  uint32_t mask_bits;

  struct
  {
    uint8_t reserved4 :1; // bit0
    uint8_t reserved3 :1; // bit1
    uint8_t reserved2 :1; // bit2
    uint8_t reserved1 :1; // bit3
  };
};

struct db_param_range_void
{
  void *min;
  void *max;
  void *standar;
};

struct db_param_range_str
{
  char *min;
  char *max;
  char *standar;
};

struct db_param_range_u8
{
  uint8_t min;
  uint8_t max;
  uint8_t standar;
};

struct db_param_range_s8
{
  int8_t min;
  int8_t max;
  int8_t standar;
};

struct db_param_range_u16
{
  uint16_t min;
  uint16_t max;
  uint16_t standar;
};

struct db_param_range_s16
{
  int16_t min;
  int16_t max;
  int16_t standar;
};

struct db_param_range_u32
{
  uint32_t min;
  uint32_t max;
  uint32_t standar;
};

struct db_param_range_s32
{
  int32_t min;
  int32_t max;
  int32_t standar;
};

struct db_param_range_f32 {
  float min;
  float max;
  float standar;
};

struct db_param_range_u64
{
  uint64_t min;
  uint64_t max;
  uint64_t standar;
};

struct db_param_range_s64
{
  int64_t min;
  int64_t max;
  int64_t standar;
};

struct db_param_range_f64
{
  double min;
  double max;
  double standar;
};

struct db_param_config
{
  uint16_t var_size;
  union db_param_info info;
  union {
    struct db_param_range_void _void_;
    struct db_param_range_str str;
    struct db_param_range_u8 u8;
    struct db_param_range_s8 s8;
    struct db_param_range_u16 u16;
    struct db_param_range_s16 s16;
    struct db_param_range_u32 u32;
    struct db_param_range_s32 s32;
    struct db_param_range_f32 f32;
    #if defined(TYPEDEF_ENABLE_VAR_B64)
    struct db_param_range_u64 u64;
    struct db_param_range_s64 s64;
    struct db_param_range_f64 f64;
    #endif
  };
};

/**
 * @brief Information about a param.
 * */
struct db_param
{
  const char *name;
  const db_param_id_t id;
  void *var;
  struct db_param_config config;
};

/**
 * @brief Table of information for the group.
 * */
struct db_group
{
  const db_group_id_t id;
  const char *name;
  const uint16_t count;
  const struct db_param *params;
  union db_group_config info_group;
  sys_snode_t node;
};

typedef struct
{
  sys_slist_t task_list;
  struct k_sem lock;
} db_list_t;

//==============================================================================
// Exported variables
//==============================================================================

//==============================================================================
// Exported functions prototypes
//==============================================================================

int db_init( void );
int db_group_add( struct db_group *group );
int db_group_remove(db_group_id_t group_id);
int db_group_load_default( db_group_id_t group_id, enum access_level access );
int db_get_var_config( struct db_group **group, struct db_param **param, db_group_id_t group_id, db_param_id_t param_id);

int db_set_param_via_string(enum access_level access, db_group_id_t group_id, db_param_id_t param_id, char *buf);
int db_param_set_str(enum access_level access, struct db_param *param, uint8_t *buf, uint16_t buflen);
int db_param_get_str(enum access_level access, struct db_param *param, uint8_t *buf, uint16_t buflen);
int db_param_set_u8(enum access_level access, struct db_param *param, uint8_t value);
int db_param_get_u8(enum access_level access, struct db_param *param, uint8_t *value);
int db_param_set_s8(enum access_level access, struct db_param *param, int8_t value);
int db_param_get_s8(enum access_level access, struct db_param *param, int8_t *value);
int db_param_set_u16(enum access_level access, struct db_param *param, uint16_t value);
int db_param_get_u16(enum access_level access, struct db_param *param, uint16_t *value);
int db_param_set_s16(enum access_level access, struct db_param *param, int16_t value);
int db_param_get_s16(enum access_level access, struct db_param *param, int16_t *value);
int db_param_set_u32(enum access_level access, struct db_param *param, uint32_t value);
int db_param_get_u32(enum access_level access, struct db_param *param, uint32_t *value);
int db_param_set_s32(enum access_level access, struct db_param *param, int32_t value);
int db_param_get_s32(enum access_level access, struct db_param *param, int32_t *value);
int db_param_set_float(enum access_level access, struct db_param *param, float value);
int db_param_get_float(enum access_level access, struct db_param *param, float *value);
#if defined(TYPEDEF_ENABLE_VAR_B64)
int db_param_set_u64(enum access_level access, struct db_param *param, uint64_t value);
int db_param_get_u64(enum access_level access, struct db_param *param, uint64_t *value);
int db_param_set_s64(enum access_level access, struct db_param *param, int64_t value);
int db_param_get_s64(enum access_level access, struct db_param *param, int64_t *value);
int db_param_set_double(enum access_level access, struct db_param *param, double value);
int db_param_get_double(enum access_level access, struct db_param *param, double *value);
#endif

int db_acc_set_str( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, char *buf, int buflen );
int db_acc_get_str( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, char *buf, int buflen );
int db_acc_set_u8( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, uint8_t value );
int db_acc_get_u8( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, uint8_t *value );
int db_acc_set_s8( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, int8_t value );
int db_acc_get_s8( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, int8_t *value );
int db_acc_set_u16( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, uint16_t value );
int db_acc_get_u16( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, uint16_t *value );
int db_acc_set_s16( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, int16_t value );
int db_acc_get_s16( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, int16_t *value );
int db_acc_set_u32( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, uint32_t value );
int db_acc_get_u32( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, uint32_t *value );
int db_acc_set_s32( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, int32_t value );
int db_acc_get_s32( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, int32_t *value );
int db_acc_set_float( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, float value );
int db_acc_get_float( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, float *value );
#if defined(TYPEDEF_ENABLE_VAR_B64)
int db_acc_set_u64( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, uint64_t value );
int db_acc_get_u64( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, uint64_t *value );
int db_acc_set_s64( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, int64_t value );
int db_acc_get_s64( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, int64_t *value );
int db_acc_set_double( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, double value );
int db_acc_get_double( enum access_level access, db_group_id_t group_id, db_param_id_t param_id, double *value );
#endif

//==============================================================================
// Exported functions
//==============================================================================

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /*__DATABASE_H_  */
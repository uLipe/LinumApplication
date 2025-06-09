#ifndef __TYPEDEFS_H_
#define __TYPEDEFS_H_

/* C++ detection */
#ifdef __cplusplus
extern "C"
{
#endif

#include "access.h"
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>

#define TYPEDEF_ENABLE_VAR_B64

/* Max and Min values of variables */
/* 8 bits */
#define MAX_BOL              1
#define MIN_BOL              0

#define MAX_U08              UINT8_MAX
#define MIN_U08              0

#define MAX_S08              INT8_MAX
#define MIN_S08              INT8_MIN

/* 16 bits */
#define MAX_U16              UINT16_MAX
#define MIN_U16              0

#define MAX_S16              INT16_MAX
#define MIN_S16              INT16_MIN

/* 32 bits */
#define MAX_U32              UINT32_MAX
#define MIN_U32              0

#define MAX_S32              INT32_MAX
#define MIN_S32              INT32_MIN

#define MAX_FLT              FLT_MAX
#define MIN_FLT              FLT_MIN

/* 64 bits */
#define MAX_U64              UINT64_MAX
#define MIN_U64              UINT64_MIN

#define MAX_S64              INT64_MAX
#define MIN_S64              INT64_MIN

#define MAX_DBL              DBL_MAX
#define MIN_DBL              DBL_MIN

#define DEVICE_NAME_MAX_SIZE    (20)

enum variable_field
{
  VAR_FIELD_NORMAL = 0,
  VAR_FIELD_PWD,
  VAR_FIELD_ENCRY,
  VAR_FIELD_HIDDEN,
  VAR_FIELD_MAX,
};

enum variable_num_digits
{
  VAR_NUN_DIG_BOL = 1,
  VAR_NUN_DIG_U08 = 3,
  VAR_NUN_DIG_U16 = 5,
  VAR_NUN_DIG_U32 = 10,
  VAR_NUN_DIG_S08 = 4,  /* Range: -127 to 127 */
  VAR_NUN_DIG_S16 = 6,
  VAR_NUN_DIG_S32 = 11,
  VAR_NUN_DIG_F32 = 39,
  VAR_NUN_DIG_F64 = 39,
  VAR_NUN_DIG_S64 = 19,
  VAR_NUN_DIG_U64 = 20,
  VAR_NUN_DIG_MAX,
};

enum variable_type
{
  eBOL = 0x00,
  eU08,
  eU16,
  eU32,
  eS08,
  eS16,
  eS32,
  eF32,
  eF64,
  eS64,
  eU64,
  eSTR, /* Pointer */
  eVOID,
  e_UMAX,
};

static inline uint16_t typedef_get_size_variable(enum variable_type type)
{
  uint16_t size = 0;

  switch( type )
  {
    case eBOL: { size = sizeof(bool); break; }
    case eU08:
    case eS08: { size = sizeof(uint8_t); break; }
    case eU16:
    case eS16: { size = sizeof(uint16_t); break; }
    case eU32:
    case eS32: { size = sizeof(uint32_t); break; }
    case eF32: { size = sizeof(float); break; }
#if defined(TYPEDEF_ENABLE_VAR_B64)
    case eF64: { size = sizeof(double); break; }
    case eU64:
    case eS64: { size = sizeof(uint64_t); break; }
#endif
    default: { size = 0; break; }
  }

  return size;
}

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* __TYPEDEFS_H_ */


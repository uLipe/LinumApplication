/**
 * @file    database.c
 * @brief
 */

//==============================================================================
// Includes
//==============================================================================

#include "database.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LOG_MODULE_REGISTER(database);

//==============================================================================
// Private definitions
//==============================================================================

#define DB_LOCK_TIMEOUT_MS (0xFFFFFFFF)
#define DB_PRINT_SIZE_BUFFER (150)

//==============================================================================
// Private macros
//==============================================================================

#define DATABASE_BKPT()

//==============================================================================
// Private typedef
//==============================================================================

//==============================================================================
// Private function prototypes
//==============================================================================

static int db_lock(db_list_t *list, uint32_t timeout_ms);
static int db_unlock(db_list_t *list);
static struct db_group *db_group_search(const db_list_t *db,
                                        const db_group_id_t group_id);
static struct db_param *db_param_search(const struct db_group *group,
                                        const db_param_id_t param_id);
static int db_is_valid_number(const char *buf, int buflen);
static uint16_t db_mount_param_msg(const struct db_param *param, char *buf,
                                   uint16_t buflen);
static int db_parse_and_set_param(enum access_level access,
                                  struct db_param *param, char *buf);

static int db_shell_show_group(void *driver, db_group_id_t group_id);
static int db_shell_cmd_show_info(const struct shell *shell, size_t argc,
                                  char **argv);
static int db_shell_cmd_set_param(const struct shell *shell, size_t argc,
                                  char **argv);

//==============================================================================
// Extern variables
//==============================================================================

//==============================================================================
// Private variables
//==============================================================================

static db_list_t g_database_list = {
    .task_list = SYS_SLIST_STATIC_INIT(&g_database_list.task_list),
};

SHELL_STATIC_SUBCMD_SET_CREATE(
    db, SHELL_CMD(show, NULL, "database show status", db_shell_cmd_show_info),
    SHELL_CMD(set, NULL, "database set variable", db_shell_cmd_set_param),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(db, &db, "database commands", NULL);

//==============================================================================
// Private functions
//==============================================================================

static int db_lock(db_list_t *db, uint32_t timeout_ms) {
  ARG_UNUSED(db);
  ARG_UNUSED(timeout_ms);

  if (db == NULL) {
    return -EINVAL;
  }

  if (!k_is_in_isr()) {
    if (k_sem_take(&db->lock, K_MSEC(timeout_ms)) != 0) {
      return -ETIMEDOUT;
    }
  } else {
    if (k_sem_take(&db->lock, K_NO_WAIT) != 0) {
      return -ETIMEDOUT;
    }
  }

  return 0;
}

static int db_unlock(db_list_t *db) {
  if (db == NULL) {
    return -EINVAL;
  }

  k_sem_give(&db->lock);
  return 0;
}

static struct db_group *db_group_search(const db_list_t *db,
                                        const db_group_id_t group_id) {
  sys_snode_t *node = NULL;
  struct db_group *group = NULL;

  if (!sys_slist_is_empty((sys_slist_t *)&db->task_list)) {
    SYS_SLIST_FOR_EACH_NODE((sys_slist_t *)&db->task_list, node) {
      group = CONTAINER_OF(node, struct db_group, node);
      if (group->id == group_id) {
        return group;
      }
    }
  }

  return NULL;
}

static struct db_param *db_param_search(const struct db_group *group,
                                        const db_param_id_t param_id) {
  uint16_t index;
  struct db_param *param;

  for (index = 0; index < group->count; index++) {
    if (group->params[index].id == param_id) {
      param = (struct db_param *)&group->params[index];
      return param;
    }
  }

  return NULL;
}

static int db_is_valid_number(const char *buf, int buflen) {
  if (buf == NULL || *buf == '\0' || buflen <= 0) {
    return -EINVAL;
  }

  bool has_dot = false;
  bool has_sign = (buf[0] == '-' || buf[0] == '+');

  for (int i = has_sign ? 1 : 0; i < buflen; i++) {
    if (buf[i] == '.') {
      if (has_dot)
        return -EINVAL;
      has_dot = true;
    } else if (buf[i] < '0' || buf[i] > '9') {
      return -EINVAL;
    }
  }

  return 0;
}

static uint16_t db_mount_param_msg(const struct db_param *param, char *buf,
                                   uint16_t buflen) {
  uint16_t len = 0;

  len = snprintf(buf, buflen, "%30s - %.3d - ", param->name, (int)param->id);

  if (param->config.info.field == VAR_FIELD_PWD) {
    len += snprintf(&buf[len], buflen, "******** \n");
  } else {
    switch (param->config.info.type) {
    case eBOL:
      len += snprintf(&buf[len], buflen, "BOL - %s\n",
                      (*((uint8_t *)param->var) > 0 ? "ENABLE" : "DISABLE"));
      break;
    case eU08:
      len +=
          snprintf(&buf[len], buflen, "U08 - %d\n", *((uint8_t *)param->var));
      break;
    case eU16:
      len +=
          snprintf(&buf[len], buflen, "U16 - %u\n", *((uint16_t *)param->var));
      break;
    case eU32:
      len +=
          snprintf(&buf[len], buflen, "U32 - %u\n", *((uint32_t *)param->var));
      break;
    case eS08:
      len += snprintf(&buf[len], buflen, "S08 - %d\n", *((int8_t *)param->var));
      break;
    case eS16:
      len +=
          snprintf(&buf[len], buflen, "S16 - %d\n", *((int16_t *)param->var));
      break;
    case eS32:
      len +=
          snprintf(&buf[len], buflen, "S32 - %d\n", *((int32_t *)param->var));
      break;
    case eF32:
      len += snprintf(&buf[len], buflen, "F32 - %f\n", *((double *)param->var));
      break;
#if defined(TYPEDEF_ENABLE_VAR_B64)
    case eS64:
      len +=
          snprintf(&buf[len], buflen, "S64 - %llu\n", *((int64_t *)param->var));
      break;
    case eU64:
      len += snprintf(&buf[len], buflen, "U64 - %llu\n",
                      *((uint64_t *)param->var));
      break;
    case eF64:
      len += snprintf(&buf[len], buflen, "F64 - %g\n", *((double *)param->var));
      break;
#endif
    case eSTR:
      len += snprintf(&buf[len], buflen, "STR - \"%s\"\n", (char *)param->var);
      break;
    case eVOID:
      len += snprintf(&buf[len], buflen, "VOID - structure");
      break;
    default:
      LOG_ERR("Typevar %d not accepted! \n", param->config.info.type);
      break;
    }
  }
  return len;
}

static int db_parse_and_set_param(enum access_level access,
                                  struct db_param *param, char *buf) {
  int err = 0;
  uint16_t size_string;

  size_string = strlen(buf);

  if (param->config.info.type != eSTR) {
    err = db_is_valid_number(buf, size_string);
    if (err) {
      return err;
    }
  }

  switch (param->config.info.type) {
  case eBOL:
    if (size_string == VAR_NUN_DIG_BOL) {
      bool bvalue = atoi(buf);
      err = db_param_set_u8(access, param, bvalue);
    }
    break;
  case eU08:
    if (size_string <= VAR_NUN_DIG_U08) {
      uint8_t u8_value = atoi(buf);
      err = db_param_set_u8(access, param, u8_value);
    }
    break;
  case eU16:
    if (size_string <= VAR_NUN_DIG_U16) {
      uint16_t u16_value = atoi(buf);
      err = db_param_set_u16(access, param, u16_value);
    }
    break;
  case eU32:
    if (size_string <= VAR_NUN_DIG_U32) {
      uint32_t u32_value = atoi(buf);
      err = db_param_set_u32(access, param, u32_value);
    }
    break;
  case eS08:
    if (size_string <= VAR_NUN_DIG_S08) {
      int8_t s8_value = atoi(buf);
      err = db_param_set_s8(access, param, s8_value);
    }
    break;
  case eS16:
    if (size_string <= VAR_NUN_DIG_S16) {
      int16_t s16_value = atoi(buf);
      err = db_param_set_s16(access, param, s16_value);
    }
    break;
  case eS32:
    if (size_string <= VAR_NUN_DIG_S32) {
      int32_t s32_value = atoi(buf);
      err = db_param_set_s32(access, param, s32_value);
    }
    break;
  case eSTR: {
    err = db_param_set_str(access, param, buf, size_string);
  } break;
  case eF32:
    if (size_string <= VAR_NUN_DIG_F32) {
      float f32_value = atof(buf);
      err = db_param_set_float(access, param, f32_value);
    }
    break;
#if defined(TYPEDEF_ENABLE_VAR_B64)
  case eS64:
    if (size_string <= VAR_NUN_DIG_S64) {
      int64_t s64_value = atoll(buf);
      err = db_param_set_s64(access, param, s64_value);
    }
    break;
  case eU64:
    if (size_string <= VAR_NUN_DIG_U64) {
      uint64_t u64_value = atoll(buf);
      err = db_param_set_u64(access, param, u64_value);
    }
    break;
  case eF64:
    if (size_string <= VAR_NUN_DIG_F64) {
      double f64_value = atof(buf);
      err = db_param_set_double(access, param, f64_value);
    }
    break;
#endif
  default:
    err = -EINVAL;
  }

  return err;
}

static int db_shell_show_group(void *driver, db_group_id_t group_id) {
  uint16_t len;
  uint16_t index;
  int err;
  struct db_group *group = NULL;
  sys_snode_t *node;
  char string[DB_PRINT_SIZE_BUFFER];

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  err = -ENOENT;

  if (!sys_slist_is_empty(&g_database_list.task_list)) {
    SYS_SLIST_FOR_EACH_NODE(&g_database_list.task_list, node) {
      group = CONTAINER_OF(node, struct db_group, node);
      if ((group->id == group_id) || (group_id == DB_GROUP_SELECT_ALL)) {
        err = 0;

        shell_print(driver, "\nGroup: %s (ID: %.3d)", group->name, group->id);
        shell_print(driver,
                    "-------------------------------------------------------");

        for (index = 0; index < group->count; index++) {
          if (group->params[index].config.info.field != VAR_FIELD_HIDDEN) {
            len = db_mount_param_msg(&group->params[index], string,
                                     DB_PRINT_SIZE_BUFFER);
            if (len > 0) {
              shell_print(driver, "%s", string);
            }
          }
        }

        // Stop loop
        if (group_id != DB_GROUP_SELECT_ALL) {
          break;
        }
      }
    }
  }

  db_unlock(&g_database_list);
  return err;
}

static int db_shell_cmd_show_info(const struct shell *shell, size_t argc,
                                  char **argv) {
  int err;
  uint32_t group_id;

  if (strcmp("help", argv[0]) == 0) {
    shell_print(shell, "database set <GROUP_ID> <PARAM_ID> <VALUE> - Set a "
                       "parameter of database.");
    shell_print(shell, "\t GROUP_ID: Group ID of database.");
    shell_print(
        shell,
        "\t PARAM_ID: Parameter ID of database that will be configured.");
    shell_print(shell, "\t    VALUE: New value of parameter.");
    shell_print(shell, "database show | <GROUP_ID> - Get the value of "
                       "parameter from the database.");
    shell_print(shell, "\t     NULL: Show all the parameter of all the groups "
                       "from the database.");
    shell_print(shell,
                "\t GROUP_ID: Show all the parameters from group selected.");

    return 0;
  } else if (strcmp("show", argv[0]) == 0) {
    if (argc == 1) {
      return db_shell_show_group((void *)shell, DB_GROUP_SELECT_ALL);
    } else if (argc == 2) {
      group_id = atoi(argv[1]);
      err = db_shell_show_group((void *)shell, group_id);
      if (err < 0) {
        shell_print(shell, "Failed to show the group %d! Error: 0x%x", group_id,
                    err);
      }
    }
  }

  return 0;
}

static int db_shell_cmd_set_param(const struct shell *shell, size_t argc,
                                  char **argv) {
  uint32_t group_id;
  uint32_t param_id;
  int err;

  if ((strcmp("set", argv[0]) == 0) && (argc == 4)) {
    group_id = atoi(argv[1]);
    param_id = atoi(argv[2]);
    err = db_set_param_via_string(ACC_LEVEL_FACTORY, group_id, param_id,
                                  (char *)argv[3]);
    if (err == DB_UPDATED) {
      shell_print(shell, "Variable %d of group %d Updated!", param_id,
                  group_id);
    } else if (err == -EACCES) {
      shell_print(
          shell,
          "You do not have sufficient access privileges set this variable!");
    } else if (err < 0) {
      shell_print(shell,
                  "Update variable %d of group %d failure! Error: 0x%x !",
                  param_id, group_id, err);
    }
  }

  return 0;
}

//==============================================================================
// Exported functions
//==============================================================================

int db_init(void) {
  if (sys_slist_is_empty(&g_database_list.task_list)) {
    sys_slist_init(&g_database_list.task_list);
    k_sem_init(&g_database_list.lock, 1, 1);
  }

  return 0;
}

int db_group_add(struct db_group *group) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  } else if (db_group_search(&g_database_list, group->id) == NULL) {
    sys_slist_append(&g_database_list.task_list, &group->node);
    err = 0;
  } else {
    err = -EEXIST;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_group_remove(db_group_id_t group_id) {
  int err;
  struct db_group *group = NULL;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  group = db_group_search(&g_database_list, group_id);
  if (group != NULL) {
    sys_slist_find_and_remove(&g_database_list.task_list, &group->node);
  } else {
    err = -ENOENT;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_group_load_default(db_group_id_t group_id, enum access_level access) {
  uint16_t index;
  uint16_t var_size;
  int err;
  sys_snode_t *node = NULL;
  struct db_group *group = NULL;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  err = -ENOENT;

  if (!sys_slist_is_empty(&g_database_list.task_list)) {
    SYS_SLIST_FOR_EACH_NODE(&g_database_list.task_list, node) {
      group = CONTAINER_OF(node, struct db_group, node);
      if ((group->id == group_id) || (group_id == DB_GROUP_SELECT_ALL)) {
        err = 0;
        for (index = 0; index < group->count; index++) {
          // Check time access level
          if (access >= group->params[index].config.info.access) {
            if (group->params[index].config.info.type == eSTR) {
              memset(group->params[index].var, 0,
                     group->params[index].config.var_size);
              memcpy(group->params[index].var,
                     group->params[index].config._void_.standar,
                     strlen(group->params[index].config.str.standar));
            } else {
              var_size = typedef_get_size_variable(
                  group->params[index].config.info.type);
              memcpy(group->params[index].var,
                     &group->params[index].config._void_.standar, var_size);
            }
          }
        }

        // Stop loop
        if (group_id != DB_GROUP_SELECT_ALL) {
          break;
        }
      }
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_get_var_config(struct db_group **group, struct db_param **param,
                      db_group_id_t group_id, db_param_id_t param_id) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  *group = db_group_search(&g_database_list, group_id);
  if (*group == NULL) {
    LOG_ERR("Group ID %d not founded!\n", group_id);
    err = -ENOENT;
  } else {
    *param = db_param_search(*group, param_id);
    if (*param == NULL) {
      LOG_ERR("Param ID %d of Group %s not founded!\n", param_id,
              (*group)->name);
      err = -ENOENT;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_set_param_via_string(enum access_level access, db_group_id_t group_id,
                            db_param_id_t param_id, char *buf) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_parse_and_set_param(access, param, buf);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - Invalid string! \n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_param_set_str(enum access_level access, struct db_param *param,
                     uint8_t *buf, uint16_t buflen) {
  int err;
  int len_to_copy;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (param->config.info.type != eSTR) {
    err = -EINVAL;
  } else {
    len_to_copy =
        buflen < param->config.var_size ? buflen : param->config.var_size - 1;
    memset(param->var, 0, param->config.var_size);
    memcpy(param->var, buf, len_to_copy);
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_str(enum access_level access, struct db_param *param,
                     uint8_t *buf, uint16_t buflen) {
  int err;
  int len_to_copy;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (param->config.info.type != eSTR) {
    err = -EINVAL;
  } else {
    len_to_copy =
        buflen < param->config.var_size ? buflen : param->config.var_size;
    memset(buf, 0, buflen);
    snprintf(buf, len_to_copy, "%s", (char *)param->var);
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_set_u8(enum access_level access, struct db_param *param,
                    uint8_t value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(uint8_t) != param->config.var_size) {
    err = -EINVAL;
  } else if ((value >= param->config.u8.min) &&
             (value <= param->config.u8.max) && param->var) {
    if (*((uint8_t *)param->var) != value) {
      *((uint8_t *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_u8(enum access_level access, struct db_param *param,
                    uint8_t *value) {
  int err;
  uint8_t *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(uint8_t) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (uint8_t *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_set_s8(enum access_level access, struct db_param *param,
                    int8_t value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(uint8_t) != param->config.var_size) {
    err = -EINVAL;
  } else if ((value >= param->config.s8.min) &&
             (value <= param->config.s8.max) && param->var) {
    if (*((int8_t *)param->var) != value) {
      *((int8_t *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_s8(enum access_level access, struct db_param *param,
                    int8_t *value) {
  int err;
  int8_t *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(int8_t) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (int8_t *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_set_u16(enum access_level access, struct db_param *param,
                     uint16_t value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(uint16_t) != param->config.var_size) {
    err = -EINVAL;
  } else if ((value >= param->config.u16.min) &&
             (value <= param->config.u16.max) && param->var) {
    if (*((uint16_t *)param->var) != value) {
      *((uint16_t *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_u16(enum access_level access, struct db_param *param,
                     uint16_t *value) {
  int err;
  uint16_t *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(uint16_t) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (uint16_t *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_set_s16(enum access_level access, struct db_param *param,
                     int16_t value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(int16_t) != param->config.var_size) {
    err = -EINVAL;
  } else if ((value >= param->config.s16.min) &&
             (value <= param->config.s16.max) && param->var) {
    if (*((int16_t *)param->var) != value) {
      *((int16_t *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_s16(enum access_level access, struct db_param *param,
                     int16_t *value) {
  int err;
  int16_t *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(int16_t) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (int16_t *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_set_u32(enum access_level access, struct db_param *param,
                     uint32_t value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(uint32_t) != param->config.var_size) {
    err = -EINVAL;
  } else if ((value >= param->config.u32.min) &&
             (value <= param->config.u32.max) && param->var) {
    if (*((uint32_t *)param->var) != value) {
      *((uint32_t *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_u32(enum access_level access, struct db_param *param,
                     uint32_t *value) {
  int err;
  uint32_t *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(uint32_t) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (uint32_t *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_set_s32(enum access_level access, struct db_param *param,
                     int32_t value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(int32_t) != param->config.var_size) {
    err = -EINVAL;
  } else if ((value >= param->config.s32.min) &&
             (value <= param->config.s32.max) && param->var) {
    if (*((int32_t *)param->var) != value) {
      *((int32_t *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_s32(enum access_level access, struct db_param *param,
                     int32_t *value) {
  int err;
  int32_t *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(int32_t) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (int32_t *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_set_float(enum access_level access, struct db_param *param,
                       float value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(float) != param->config.var_size) {
    err = -EINVAL;
  } else if (isnanf(value)) {
    err = -EINVAL;
  } else if ((value >= param->config.f32.min) &&
             (value <= param->config.f32.max) && param->var) {
    if (*((float *)param->var) != value) {
      *((float *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_float(enum access_level access, struct db_param *param,
                       float *value) {
  int err;
  float *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(float) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (float *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

#if defined(TYPEDEF_ENABLE_VAR_B64)
int db_param_set_u64(enum access_level access, struct db_param *param,
                     uint64_t value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(uint64_t) != param->config.var_size) {
    err = -EINVAL;
  } else if ((value >= param->config.u64.min) &&
             (value <= param->config.u64.max) && param->var) {
    if (*((uint64_t *)param->var) != value) {
      *((uint64_t *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_u64(enum access_level access, struct db_param *param,
                     uint64_t *value) {
  int err;
  uint64_t *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(uint64_t) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (uint64_t *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_set_s64(enum access_level access, struct db_param *param,
                     int64_t value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(int64_t) != param->config.var_size) {
    err = -EINVAL;
  } else if ((value >= param->config.s64.min) &&
             (value <= param->config.s64.max) && param->var) {
    if (*((int64_t *)param->var) != value) {
      *((int64_t *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_s64(enum access_level access, struct db_param *param,
                     int64_t *value) {
  int err;
  int64_t *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(int64_t) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (int64_t *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_set_double(enum access_level access, struct db_param *param,
                        double value) {
  int err;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(double) != param->config.var_size) {
    err = -EINVAL;
  } else if (isnan(value)) {
    err = -EINVAL;
  } else if ((value >= param->config.f64.min) &&
             (value <= param->config.f64.max) && param->var) {
    if (*((double *)param->var) != value) {
      *((double *)param->var) = value;
      err = DB_UPDATED;
    }
  } else {
    err = -EINVAL;
  }

  db_unlock(&g_database_list);
  return err;
}

int db_param_get_double(enum access_level access, struct db_param *param,
                        double *value) {
  int err;
  double *value_ptr;

  err = db_lock(&g_database_list, DB_LOCK_TIMEOUT_MS);
  if (err) {
    return err;
  }

  if (access < param->config.info.access) {
    err = -EACCES;
  } else if (sizeof(double) != param->config.var_size) {
    err = -EINVAL;
  } else {
    value_ptr = (double *)param->var;
    if (*value_ptr != *value) {
      *value = *value_ptr;
      err = DB_UPDATED;
    }
  }

  db_unlock(&g_database_list);
  return err;
}

#endif

int db_acc_set_str(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, char *buf, int buflen) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_str(access, param, buf, buflen);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }
  return err;
}

int db_acc_get_str(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, char *buf, int buflen) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_str(access, param, buf, buflen);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

int db_acc_set_u8(enum access_level access, db_group_id_t group_id,
                  db_param_id_t param_id, uint8_t value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_u8(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_u8(enum access_level access, db_group_id_t group_id,
                  db_param_id_t param_id, uint8_t *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_u8(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

int db_acc_set_s8(enum access_level access, db_group_id_t group_id,
                  db_param_id_t param_id, int8_t value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_s8(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_s8(enum access_level access, db_group_id_t group_id,
                  db_param_id_t param_id, int8_t *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_s8(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

int db_acc_set_u16(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, uint16_t value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_u16(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_u16(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, uint16_t *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_u16(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

int db_acc_set_s16(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, int16_t value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_s16(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_s16(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, int16_t *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_s16(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

int db_acc_set_u32(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, uint32_t value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_u32(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_u32(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, uint32_t *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_u32(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

int db_acc_set_s32(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, int32_t value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_s32(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_s32(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, int32_t *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_s32(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

int db_acc_set_float(enum access_level access, db_group_id_t group_id,
                     db_param_id_t param_id, float value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_float(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_float(enum access_level access, db_group_id_t group_id,
                     db_param_id_t param_id, float *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_float(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

#if defined(TYPEDEF_ENABLE_VAR_B64)
int db_acc_set_u64(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, uint64_t value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_u64(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_u64(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, uint64_t *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_u64(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

int db_acc_set_s64(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, int64_t value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_s64(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_s64(enum access_level access, db_group_id_t group_id,
                   db_param_id_t param_id, int64_t *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_s64(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

int db_acc_set_double(enum access_level access, db_group_id_t group_id,
                      db_param_id_t param_id, double value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_set_double(access, param, value);
    if (err < 0) {
      LOG_ERR("Group: %s - Param: %s - update value fail!\n ", group->name,
              param->name);
    }
  }

  return err;
}

int db_acc_get_double(enum access_level access, db_group_id_t group_id,
                      db_param_id_t param_id, double *value) {
  int err;
  struct db_group *group = NULL;
  struct db_param *param = NULL;

  err = db_get_var_config(&group, &param, group_id, param_id);
  if (!err) {
    err = db_param_get_double(access, param, value);
    if (err < 0) {
      LOG_ERR(
          "Param ID %d of Group %s New value has different parameter size!\n",
          param_id, group->name);
    }
  }

  return err;
}

#endif

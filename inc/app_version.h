/*
* Copyright (c) 2022 Circuit Dojo LLC
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _APP_VERSION_H_
#define _APP_VERSION_H_

/* C++ detection */
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

enum app_version_build_type
{
  APP_VERSION_BUILD_DEBUG,
  APP_VERSION_BUILD_RELEASE
};

struct app_version
{
  uint32_t major;
  uint32_t minior;
  uint32_t patch;
  uint32_t commit;
  char hash[8];
  char build_date_time[30];
  enum app_version_build_type build_type;
};

// struct app_versioneapp_version_get(void);
void app_version_get(struct app_version *version);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /*_APP_VERSION_H_*/
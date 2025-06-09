/*
 * Copyright (c) 2022 Circuit Dojo LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_version.h"
#include <string.h>
#include <zephyr/bindesc.h>
#include <zephyr/kernel.h>

void app_version_get(struct app_version *version) {
  enum app_version_build_type build_type = APP_VERSION_BUILD_DEBUG;

  // Determines if it's a release build
  // if (strcmp(CONFIG_APP_RELEASE_TYPE, "release") == 0)
  // {
  // build_type = app_version_build_release;
  // }

  // Then creates the app_version
  version->major = BINDESC_GET_UINT(app_version_major);
  version->minior = BINDESC_GET_UINT(app_version_minor);
  version->patch = BINDESC_GET_UINT(app_version_patchlevel);
  // version->commit = CONFIG_APP_VERSION_COMMIT;
  version->build_type = build_type;

  // Copy the hash
  // memcpy(version->hash, CONFIG_APP_VERSION_HASH, sizeof(version->hash));
  memcpy(version->build_date_time, BINDESC_GET_STR(build_date_time_string),
         sizeof(version->build_date_time));
}

/*
 * playos_power_internal.h — internal hwmon sensor classification helpers
 *
 * Sprint 13 T5: hwmon `name` strings are vendor-specific. These matchers keep
 * the vendor knowledge in one place so the sysfs readers in playos_power.c
 * stay portable across AMD (amdgpu/k10temp) and Intel (i915/xe/coretemp).
 *
 * Internal API only — not part of the public libplayos headers.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef PLAYOS_POWER_INTERNAL_H
#define PLAYOS_POWER_INTERNAL_H

/* GPU temperature hwmon names: amdgpu (AMD), i915/xe (Intel). */
int playos__hwmon_name_is_gpu(const char *name);

/* CPU temperature hwmon names: k10temp (AMD), coretemp (Intel). */
int playos__hwmon_name_is_cpu(const char *name);

#endif /* PLAYOS_POWER_INTERNAL_H */

#ifndef LIBXSMM_CONFIG_H
#define LIBXSMM_CONFIG_H

#if !defined(LIBXSMM_DEFAULT_CONFIG) && (defined(_WIN32) || (defined(LIBXSMM_SOURCE_H) && !defined(LIBXSMM_CONFIGURED)))
#define LIBXSMM_DEFAULT_CONFIG
#endif

#if !defined(LIBXSMM_DEFAULT_CONFIG) && (!defined(LIBXSMM_SOURCE_H) || defined(LIBXSMM_CONFIGURED))

#include "libxsmm_version.h"



#else

#define LIBXSMM_CONFIG_VERSION "unconfigured"
#define LIBXSMM_CONFIG_BRANCH  "unconfigured"
#define LIBXSMM_CONFIG_VERSION_MAJOR  INT_MAX
#define LIBXSMM_CONFIG_VERSION_MINOR  INT_MAX
#define LIBXSMM_CONFIG_VERSION_UPDATE INT_MAX
#define LIBXSMM_CONFIG_VERSION_PATCH  INT_MAX
#define LIBXSMM_CONFIG_BUILD_DATE INT_MAX

#endif

#define LIBXSMM_CONFIG_CACHELINE 64
#define LIBXSMM_CONFIG_ALIGNMENT 64
#define LIBXSMM_CONFIG_MALLOC 0
#define LIBXSMM_CONFIG_ILP64 0
#define LIBXSMM_CONFIG_SYNC 1
#define LIBXSMM_CONFIG_JIT 1

#define LIBXSMM_CONFIG_PREFETCH -1
#define LIBXSMM_CONFIG_MAX_MNK 262144
#define LIBXSMM_CONFIG_MAX_DIM 64
#define LIBXSMM_CONFIG_AVG_DIM 32
#define LIBXSMM_CONFIG_MAX_M 64
#define LIBXSMM_CONFIG_MAX_N 64
#define LIBXSMM_CONFIG_MAX_K 64
#define LIBXSMM_CONFIG_FLAGS 0
#define LIBXSMM_CONFIG_ALPHA 1
#define LIBXSMM_CONFIG_BETA 1
#define LIBXSMM_CONFIG_WRAP 1

#endif


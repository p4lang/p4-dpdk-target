#ifndef _P4_SDE_OSDEP_UTILS_H
#define _P4_SDE_OSDEP_UTILS_H

#include <target_utils/map/map.h>
#include <target_utils/id/id.h>
#include <target_utils/hashtbl/bf_hashtbl.h>

typedef bf_map_t p4_sde_map;

typedef bf_map_sts_t p4_sde_map_sts;

typedef unsigned long p4_sde_map_key;

#define P4_SDE_MAP_ADD(MAP,key,data) \
		bf_map_add((MAP),(key),(data))

#define P4_SDE_MAP_INIT(MAP) \
		bf_map_init((MAP))

#define P4_SDE_MAP_GET(MAP,key,data) \
		bf_map_get((MAP),(key),(data))

#define P4_SDE_MAP_RMV(MAP,KEY) \
		bf_map_rmv((MAP),(KEY))

#define P4_SDE_MAP_DESTROY(MAP) \
		bf_map_destroy((MAP))

typedef bf_id_allocator p4_sde_id;

#define P4_SDE_ID_INIT(SIZE,ZERO_BASED) \
		bf_id_allocator_new((SIZE),(ZERO_BASED))

#define P4_SDE_ID_ADD(ID_HDL) \
		bf_id_allocator_allocate((ID_HDL))

#define P4_SDE_ID_RMV(ID_HDL,ID) \
		bf_id_allocator_release((ID_HDL),(ID))

#define P4_SDE_ID_DESTROY(ID_HDL) \
		bf_id_allocator_destroy((ID_HDL))
#endif

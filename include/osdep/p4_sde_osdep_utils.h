#ifndef _P4_SDE_OSDEP_UTILS_H
#define _P4_SDE_OSDEP_UTILS_H

#include <target-utils/map/map.h>
#include <target-utils/id/id.h>
#include <target-utils/hashtbl/bf_hashtbl.h>

typedef bf_map_t p4_sde_map;

typedef bf_map_sts_t p4_sde_map_sts;

typedef unsigned long p4_sde_map_key;

#define P4_SDE_MAP_ADD(MAP,key,data) \
		bf_map_add((MAP),(key),(data))

#define P4_SDE_MAP_INIT(MAP) \
		bf_map_init((MAP))

#define P4_SDE_MAP_GET(MAP,key,data) \
		bf_map_get((MAP),(key),(data))

#define P4_SDE_MAP_GET_FIRST(MAP,key,data) \
		bf_map_get_first((MAP),(key),(data))

#define P4_SDE_MAP_GET_NEXT(MAP,key,data) \
		bf_map_get_next((MAP),(key),(data))

#define P4_SDE_MAP_RMV(MAP,KEY) \
		bf_map_rmv((MAP),(KEY))

#define P4_SDE_MAP_GET_FIRST_RMV(MAP, key, data) \
		bf_map_get_first_rmv((MAP), (key), (data))

#define P4_SDE_MAP_DESTROY(MAP) \
		bf_map_destroy((MAP))

typedef bf_id_allocator p4_sde_id;

#define P4_SDE_ID_INIT(SIZE,ZERO_BASED) \
		bf_id_allocator_new((SIZE),(ZERO_BASED))

#define P4_SDE_ID_ALLOC(ID_HDL) \
		bf_id_allocator_allocate((ID_HDL))

#define P4_SDE_ID_FREE(ID_HDL, ID) \
		bf_id_allocator_release((ID_HDL), (ID))

#define P4_SDE_ID_DESTROY(ID_HDL) \
		bf_id_allocator_destroy((ID_HDL))

#define P4_SDE_ID_GET_FIRST(ID_HDL) \
		bf_id_allocator_get_first((ID_HDL))

#define P4_SDE_ID_GET_NEXT(ID_HDL, ID) \
		bf_id_allocator_get_next((ID_HDL), (ID))

#define P4_SDE_ID_IS_SET(ID_HDL, ID) \
		bf_id_allocator_is_set((ID_HDL), (ID))

#define P4_SDE_ID_SET(ID_HDL, ID) \
		bf_id_allocator_set((ID_HDL), (ID))
#endif

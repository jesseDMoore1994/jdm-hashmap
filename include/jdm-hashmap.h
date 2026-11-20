#ifndef JDM_HASHMAP_H
#define JDM_HASHMAP_H

#include <jdm-alloc.h>
#include <jdm-array.h>
#include <jdm-defs.h>

typedef enum {
  JDM_HASHMAP_OK = 0,
  JDM_HASHMAP_ERR_ALLOC,
  JDM_HASHMAP_ERR_MAX_CAP,
  JDM_HASHMAP_ERR_BAD_KEY,
  JDM_HASHMAP_ERR_KEY_NOT_FOUND,
  JDM_HASHMAP_ERR_ARRAY,
} jdm_hashmap_status_t;

typedef struct jdm_hashmap_entry_t {
  u8 *key;
  u8 *value;
  sz hash;
  struct jdm_hashmap_entry_t *next;
} jdm_hashmap_entry_t;

typedef struct jdm_hashmap_t jdm_hashmap_t;
typedef sz (*jdm_hashmap_hash_func_t)(const u8 *key, s32 len, u32 seed);
typedef s8 (*jdm_hashmap_key_compare_func_t)(const u8 *key1, const u8 *key2);
typedef void (*jdm_hashmap_for_each_key_func_t)(const u8 *key);
typedef void (*jdm_hashmap_for_each_value_func_t)(u8 *value);

typedef struct {
  jdm_alloc_t *alloc;
  f32 load_factor;
  sz initial_capacity;
  s32 key_length;
  s32 value_length;
  u32 hash_seed;
  jdm_hashmap_hash_func_t hash;
  jdm_hashmap_key_compare_func_t key_compare;
} jdm_hashmap_config_t;

s8 jdm_hashmap_str_key_compare(const u8 *key1, const u8 *key2);
jdm_hashmap_t *jdm_hashmap_create(const jdm_hashmap_config_t cfg);
void jdm_hashmap_destroy(jdm_hashmap_t *map);
jdm_hashmap_status_t jdm_hashmap_add(jdm_hashmap_t *map, u8 *key, u8 *val);
jdm_hashmap_status_t jdm_hashmap_get(jdm_hashmap_t *map, u8 *key, u8 **out);
jdm_hashmap_status_t jdm_hashmap_remove(jdm_hashmap_t *map, u8 *key, u8 **out);
void jdm_hashmap_remove_all(jdm_hashmap_t *map);
b8 jdm_hashmap_contains_key(jdm_hashmap_t* map, u8* key);
sz jdm_hashmap_size(jdm_hashmap_t *map);
sz jdm_hashmap_capacity(jdm_hashmap_t *map);
jdm_hashmap_status_t jdm_hashmap_get_keys(jdm_hashmap_t *map,
                                          jdm_array_t **out);
jdm_hashmap_status_t jdm_hashmap_get_values(jdm_hashmap_t *map,
                                            jdm_array_t **out);
void jdm_hashmap_for_each_key(jdm_hashmap_t *map,
                              jdm_hashmap_for_each_key_func_t f);
void jdm_hashmap_for_each_value(jdm_hashmap_t *map,
                                jdm_hashmap_for_each_value_func_t f);
sz jdm_hashmap_hash_str(const u8 *key, s32 len, u32 seed);
sz jdm_hashmap_hash(const u8 *key, s32 len, u32 seed);
sz jdm_hashmap_hash_ptr(const u8 *key, s32 len, u32 seed);

typedef struct {
  jdm_hashmap_t *map;
  sz bucket_idx;
  jdm_hashmap_entry_t *next;
  jdm_hashmap_entry_t *prev;
} jdm_hashmap_iterator_t;

jdm_hashmap_iterator_t jdm_hashmap_iterator_create(jdm_hashmap_t *map);
jdm_hashmap_status_t jdm_hashmap_iterator_next(jdm_hashmap_iterator_t it,
                                               jdm_hashmap_entry_t **out);
jdm_hashmap_status_t jdm_hashmap_iterator_remove(jdm_hashmap_iterator_t it,
                                                 u8 **out);
void jdm_hashmap_iterator_reset(jdm_hashmap_iterator_t it);

#endif // JDM_HASHMAP_H

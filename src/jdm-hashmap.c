#include "jdm-hashmap.h"
#include <jdm-alloc.h>
#include <jdm-log.h>
#include <string.h>

#define SIZE_T_MAX_POWER_OF_TWO ((SIZE_MAX >> 1) + 1)

struct jdm_hashmap_t {
  jdm_alloc_t *alloc;
  f32 load_factor;
  sz size;
  sz capacity;
  sz threshold;
  u32 hash_seed;
  s32 key_length;
  s32 value_length;
  jdm_hashmap_hash_func_t hash;
  jdm_hashmap_key_compare_func_t key_compare;
  jdm_hashmap_entry_t **buckets;
};

s8 jdm_hashmap_str_key_compare(const u8 *key1, const u8 *key2) {
  return strcmp(key1, key2);
}

jdm_hashmap_t *jdm_hashmap_create(const jdm_hashmap_config_t cfg) {
  if (!JDM_IS_ALIGNED(cfg.initial_capacity, 2)) {
    JDM_LOG_ERROR("Hashmap initial capacity must be power of two");
    return JDM_NULL;
  }
  jdm_hashmap_t *m = jdm_alloc(cfg.alloc, sizeof(jdm_hashmap_t));
  if (m == JDM_NULL) {
    JDM_LOG_ERROR("Error allocating hashmap");
    return JDM_NULL;
  }

  m->alloc = cfg.alloc;
  m->load_factor = cfg.load_factor;
  m->size = 0;
  m->capacity = cfg.initial_capacity;
  m->hash_seed = cfg.hash_seed;
  m->key_length = cfg.key_length;
  m->value_length = cfg.value_length;
  m->threshold = (sz)(m->capacity * m->load_factor);
  m->hash = cfg.hash;
  m->key_compare = cfg.key_compare;
  m->buckets =
      jdm_calloc(cfg.alloc, m->capacity, sizeof(jdm_hashmap_entry_t *));
  if (m->buckets == JDM_NULL) {
    JDM_LOG_ERROR("Error allocating hashmap buckets");
    jdm_free(cfg.alloc, m);
    return JDM_NULL;
  }

  return m;
}

void jdm_hashmap_destroy(jdm_hashmap_t *map) {
  jdm_alloc_t *alloc = map->alloc;
  for (sz i = 0; i < map->capacity; i++) {
    jdm_hashmap_entry_t *n = map->buckets[i];
    while (n) {
      jdm_hashmap_entry_t *tmp = n->next;
      jdm_free(alloc, n);
      n = tmp;
    }
  }
  jdm_free(alloc, map->buckets);
  jdm_free(alloc, map);
}

static void move_entries(jdm_hashmap_entry_t **src, sz src_size,
                         jdm_hashmap_entry_t **dest, sz dest_size) {
  for (sz i = 0; i < src_size; i++) {
    jdm_hashmap_entry_t *e = src[i];
    while (e) {
      jdm_hashmap_entry_t *n = e->next;
      sz idx = e->hash & (dest_size - 1);
      e->next = dest[idx];
      dest[idx] = e;
      e = n;
    }
  }
}

static jdm_hashmap_status_t resize(jdm_hashmap_t *map, sz new_capacity) {
  if (map->capacity == SIZE_T_MAX_POWER_OF_TWO) {
    return JDM_HASHMAP_ERR_MAX_CAP;
  }

  jdm_alloc_t *alloc = map->alloc;
  jdm_hashmap_entry_t **new_buckets =
      jdm_calloc(alloc, new_capacity, sizeof(jdm_hashmap_entry_t *));

  if (!new_buckets) {
    return JDM_HASHMAP_ERR_ALLOC;
  }
  jdm_hashmap_entry_t **old_buckets = map->buckets;
  move_entries(map->buckets, map->capacity, new_buckets, new_capacity);

  map->buckets = new_buckets;
  map->capacity = new_capacity;
  map->threshold = (size_t)(map->load_factor * new_capacity);

  jdm_free(alloc, old_buckets);

  return JDM_HASHMAP_OK;
}

static sz get_map_idx(jdm_hashmap_t *map, u8 *key) {
  const sz hash = map->hash(key, map->key_length, map->hash_seed);
  return hash & (map->capacity - 1);
}

jdm_hashmap_status_t jdm_hashmap_add(jdm_hashmap_t *map, u8 *key, u8 *val) {
  jdm_hashmap_status_t s;
  if (map->size >= map->threshold) {
    if ((s = resize(map, map->capacity << 1)) != JDM_HASHMAP_OK) {
      return s;
    }
  }

  if (key == JDM_NULL) {
    JDM_LOG_ERROR("Cannot use a null key in jdm_hashmap");
    return JDM_HASHMAP_ERR_BAD_KEY;
  }

  const sz hash = map->hash(key, map->key_length, map->hash_seed);
  const sz idx = hash & (map->capacity - 1);

  jdm_hashmap_entry_t *candidate_target = map->buckets[idx];

  while (candidate_target) {
    u8 *candidate_key = candidate_target->key;
    if (candidate_key && map->key_compare(candidate_key, key) == 0) {
      candidate_target->value = val;
      return JDM_HASHMAP_OK;
    }
    candidate_target = candidate_target->next;
  }

  jdm_hashmap_entry_t *new_entry =
      jdm_alloc(map->alloc, sizeof(jdm_hashmap_entry_t));
  if (!new_entry) {
    return JDM_HASHMAP_ERR_ALLOC;
  }

  new_entry->key = key;
  new_entry->value = val;
  new_entry->hash = hash;
  new_entry->next = map->buckets[idx];

  map->buckets[idx] = new_entry;
  map->size++;

  return JDM_HASHMAP_OK;
}

jdm_hashmap_status_t jdm_hashmap_get(jdm_hashmap_t *map, u8 *key, u8 **out) {
  if (key == JDM_NULL) {
    JDM_LOG_ERROR("Cannot use a null key in jdm_hashmap");
    return JDM_HASHMAP_ERR_BAD_KEY;
  }

  sz idx = get_map_idx(map, key);
  jdm_hashmap_entry_t *e = map->buckets[idx];

  while (e) {
    if (e->key && map->key_compare(e->key, key) == 0) {
      *out = e->value;
      return JDM_HASHMAP_OK;
    }
  }

  JDM_LOG_WARNING("Key to get not found in jdm_hashmap");
  return JDM_HASHMAP_ERR_KEY_NOT_FOUND;
}

jdm_hashmap_status_t jdm_hashmap_remove(jdm_hashmap_t *map, u8 *key, u8 **out) {
  if (key == JDM_NULL) {
    JDM_LOG_ERROR("Cannot use a null key in jdm_hashmap");
    return JDM_HASHMAP_ERR_BAD_KEY;
  }

  const sz idx = get_map_idx(map, key);
  jdm_hashmap_entry_t *e = map->buckets[idx];
  jdm_hashmap_entry_t *p = JDM_NULL;
  jdm_hashmap_entry_t *n = JDM_NULL;

  while (e) {
    n = e->next;
    if (e->key && map->key_compare(key, e->key) == 0) {
      u8 *val = e->value;
      if (p == JDM_NULL) {
        map->buckets[idx] = n;
      } else {
        p->next = n;
      }

      jdm_free(map->alloc, e);
      map->size--;
      if (out) {
        *out = val;
      }
      return JDM_HASHMAP_OK;
    }
    p = e;
    e = n;
  }

  JDM_LOG_WARNING("Key to remove not found in jdm_hashmap");
  return JDM_HASHMAP_ERR_KEY_NOT_FOUND;
}

void jdm_hashmap_remove_all(jdm_hashmap_t *map) {
  for (sz i = 0; i < map->capacity; i++) {
    jdm_hashmap_entry_t *e = map->buckets[i];
    while (e != JDM_NULL) {
      jdm_hashmap_entry_t *n = e->next;
      jdm_free(map->alloc, e);
      map->size--;
      e = n;
    }
    map->buckets[i] = JDM_NULL;
  }
}

b8 jdm_hashmap_contains_key(jdm_hashmap_t *map, u8 *key) {
  jdm_hashmap_entry_t *e = map->buckets[get_map_idx(map, key)];

  while(e) {
    if(map->key_compare(key, e->key) == 0) {
      return JDM_TRUE;
    };

    e = e->next;
  }

  return JDM_FALSE;
}

sz jdm_hashmap_size(jdm_hashmap_t *map) { return map->size; }

sz jdm_hashmap_capacity(jdm_hashmap_t *map) { return map->capacity; }

jdm_hashmap_status_t jdm_hashmap_get_keys(jdm_hashmap_t *map,
                                          jdm_array_t **out) {
  jdm_array_t *keys = jdm_array_create((jdm_array_config_t){
      .alloc = map->alloc,
      .capacity = map->size,
      .element_size = sizeof(
          char *), // FIXME: come back and handle fixed sizes, not just strings
      .exp_factor = 2,
  });

  if (!keys) {
    JDM_LOG_ERROR("Error creating return array");
    return JDM_HASHMAP_ERR_ARRAY;
  }

  for (sz i = 0; i < map->capacity; i++) {
    jdm_hashmap_entry_t *e = map->buckets[i];
    while (e) {
      jdm_array_status_t s;
      if ((s = jdm_array_add(keys, (u8 *)&e->key)) ==
          JDM_ARRAY_OK) { // BAD, assumes string key
        e = e->next;
      } else {
        JDM_LOG_ERROR("Error adding key to array: %d", s);
        jdm_array_destroy(keys);
        return JDM_HASHMAP_ERR_ARRAY;
      }
    }
  }

  *out = keys;
  return JDM_HASHMAP_OK;
}

jdm_hashmap_status_t jdm_hashmap_get_values(jdm_hashmap_t *map,
                                            jdm_array_t **out) {
  jdm_array_t *vals = jdm_array_create((jdm_array_config_t){
      .alloc = map->alloc,
      .capacity = map->size,
      .element_size = map->value_length,
      .exp_factor = 2,
  });

  if (!vals) {
    JDM_LOG_ERROR("Error creating return array");
    return JDM_HASHMAP_ERR_ARRAY;
  }

  for (sz i = 0; i < map->capacity; i++) {
    jdm_hashmap_entry_t *e = map->buckets[i];
    while (e) {
      jdm_array_status_t s;
      if ((s = jdm_array_add(vals, e->value)) == JDM_ARRAY_OK) {
        e = e->next;
      } else {
        JDM_LOG_ERROR("Error adding key to array: %d", s);
        jdm_array_destroy(vals);
        return JDM_HASHMAP_ERR_ARRAY;
      }
    }
  }

  *out = vals;
  return JDM_HASHMAP_OK;
}

void jdm_hashmap_for_each_key(jdm_hashmap_t *map,
                              jdm_hashmap_for_each_key_func_t f) {
  for (sz i = 0; i < map->capacity; i++) {
    jdm_hashmap_entry_t *e = map->buckets[i];
    while (e) {
      f(e->key);
      e = e->next;
    }
  }
}

void jdm_hashmap_for_each_value(jdm_hashmap_t *map,
                                jdm_hashmap_for_each_value_func_t f) {
  for (sz i = 0; i < map->capacity; i++) {
    jdm_hashmap_entry_t *e = map->buckets[i];
    while (e) {
      f(e->value);
      e = e->next;
    }
  }
}

// djb2 hash
sz jdm_hashmap_hash_str(const u8 *key, s32 len, u32 seed) {
  const u8 *str = key;
  register sz hash = seed + 5381 + len + 1;

  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) ^ c;
  }
  return hash;
}

u64 rotl32(u32 x, s8 r) { return (x << r) | (x >> (32 - r)); }

u64 rotl64(u64 x, s8 r) { return (x << r) | (x >> (64 - r)); }

#define BIG_CONSTANT(x) (x##LLU)

u64 fmix64(u64 k) {
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

// murmurhash3
sz jdm_hashmap_hash(const u8 *key, s32 len, u32 seed) {
  const u8 *data = (const u8 *)key;
  const s32 nblocks = len / 16;

  u64 h1 = seed;
  u64 h2 = seed;

  const u64 c1 = BIG_CONSTANT(0x87c37b91114253d5);
  const u64 c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  const u64 *blocks = (const u64 *)(data);

  for (s32 i = 0; i < nblocks; i++) {
    u64 k1 = blocks[i * 2 + 0];
    u64 k2 = blocks[i * 2 + 1];

    k1 *= c1;
    k1 = rotl64(k1, 31);
    k1 *= c2;
    h1 ^= k1;
    h1 = rotl64(h1, 27);
    h1 += h2;
    h1 = h1 * 5 + 0x52dce729;

    k2 *= c2;
    k2 = rotl64(k2, 33);
    k2 *= c1;
    h2 ^= k2;
    h2 = rotl64(h2, 31);
    h2 += h1;
    h2 = h2 * 5 + 0x38495ab5;
  }

  const u8 *tail = (const u8 *)(data + nblocks * 16);

  u64 k1 = 0;
  u64 k2 = 0;

  switch (len & 15) {
  case 15:
    k2 ^= ((u64)tail[14]) << 48;
  case 14:
    k2 ^= ((u64)tail[13]) << 40;
  case 13:
    k2 ^= ((u64)tail[12]) << 32;
  case 12:
    k2 ^= ((u64)tail[11]) << 24;
  case 11:
    k2 ^= ((u64)tail[10]) << 16;
  case 10:
    k2 ^= ((u64)tail[9]) << 8;
  case 9:
    k2 ^= ((u64)tail[8]) << 0;
    k2 *= c2;
    k2 = rotl64(k2, 33);
    k2 *= c1;
    h2 ^= k2;
  case 8:
    k1 ^= ((u64)tail[7]) << 56;
  case 7:
    k1 ^= ((u64)tail[6]) << 48;
  case 6:
    k1 ^= ((u64)tail[5]) << 40;
  case 5:
    k1 ^= ((u64)tail[4]) << 32;
  case 4:
    k1 ^= ((u64)tail[3]) << 24;
  case 3:
    k1 ^= ((u64)tail[2]) << 16;
  case 2:
    k1 ^= ((u64)tail[1]) << 8;
  case 1:
    k1 ^= ((u64)tail[0]) << 0;
    k1 *= c1;
    k1 = rotl64(k1, 31);
    k1 *= c2;
    h1 ^= k1;
  };

  h1 ^= len;
  h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  return h1;
}

// murmurhash3
u64 jdm_hashmap_hash_ptr(const u8 *key, s32 len, u32 seed) {
  u32 h1 = seed;
  u32 h2 = seed;

  const u64 c1 = BIG_CONSTANT(0x87c37b91114253d5);
  const u64 c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  u32 k1 = (u32)(uintptr_t)key;
  u32 k2 = (u32)((uintptr_t)key >> (u64)32);

  k1 *= c1;
  k1 = rotl32(k1, 31);
  k1 *= c2;
  h1 ^= k1;
  h1 = rotl32(h1, 27);
  h1 += h2;
  h1 = h1 * 5 + 0x52dce729;

  k2 *= c2;
  k2 = rotl32(k2, 33);
  k2 *= c1;
  h2 ^= k2;
  h2 = rotl32(h2, 31);
  h2 += h1;
  h2 = h2 * 5 + 0x38495ab5;

  h1 ^= len;
  h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  u64 result = ((u64)h1 << 32) | (u64)h2;

  return result;
}

jdm_hashmap_iterator_t jdm_hashmap_iterator_create(jdm_hashmap_t *map) {
  jdm_hashmap_iterator_t it;
  it.map = map;
  for (sz i = 0; i < map->capacity; i++) {
    jdm_hashmap_entry_t *e = map->buckets[i];
    if (e) {
      it.bucket_idx = i;
      it.next = e;
      it.prev = JDM_NULL;
      break;
    }
  }
}

jdm_hashmap_status_t jdm_hashmap_iterator_next(jdm_hashmap_iterator_t it,
                                               jdm_hashmap_entry_t **out) {
  if (!it.next) {
    return JDM_HASHMAP_ERR_KEY_NOT_FOUND;
  }

  it.prev = it.next;
  it.next = it.next->next; // iterator next then entry next

  if (it.next) {
    *out = it.prev;
    return JDM_HASHMAP_OK;
  }

  for (sz i = it.bucket_idx + 1; i < it.map->capacity; i++) {
    it.next = it.map->buckets[i];
    if (it.next) {
      it.bucket_idx = i;
      break;
    }
  }

  *out = it.prev;
  return JDM_HASHMAP_OK;
}

jdm_hashmap_status_t jdm_hashmap_iterator_remove(jdm_hashmap_iterator_t it,
                                                 u8 **out) {
  return jdm_hashmap_remove(it.map, it.prev->key, out);
}

void jdm_hashmap_iterator_reset(jdm_hashmap_iterator_t it) {
  for (sz i = 0; i < it.map->capacity; i++) {
    jdm_hashmap_entry_t *e = it.map->buckets[i];
    if (e) {
      it.bucket_idx = i;
      it.next = e;
      it.prev = JDM_NULL;
      break;
    }
  }
}

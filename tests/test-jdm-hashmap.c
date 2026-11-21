#include "jdm-hashmap.h"
#include <jdm-alloc.h>
#include <jdm-log.h>

typedef struct {
  jdm_alloc_t *test_alloc;
  jdm_hashmap_t *test_map;
} test_ctx;

static test_ctx get_test_ctx(void) {
  jdm_alloc_t *alloc =
      jdm_alloc_create((jdm_alloc_config_t){.type = JDM_ALLOC_SYSTEM});
  jdm_hashmap_t *map = jdm_hashmap_create((jdm_hashmap_config_t){
      .alloc = alloc,
      .load_factor = 0.75f,
      .initial_capacity = 16,
      .key_length = -1,
      .value_length = sizeof(s32),
      .hash_seed = 0,
      .hash = jdm_hashmap_hash_str,
      .key_compare = jdm_hashmap_str_key_compare,
  });
  return (test_ctx){
      .test_alloc = alloc,
      .test_map = map,
  };
}

static void destroy_test_ctx(test_ctx ctx) {
  jdm_hashmap_destroy(ctx.test_map);
  jdm_alloc_destroy(ctx.test_alloc);
}

static b8 test_jdm_hashmap_create_destroy(void) {
  test_ctx ctx = get_test_ctx();
  destroy_test_ctx(ctx);
  return JDM_TRUE;
}

static b8 test_jdm_hashmap_element_ops(void) {
  test_ctx ctx = get_test_ctx();
  jdm_alloc_t *alloc = ctx.test_alloc;
  jdm_hashmap_t *map = ctx.test_map;
  s32 *i = NULL;
  jdm_hashmap_status_t s;

  jdm_hashmap_add(map, "a", (u8 *)&(s32){42});
  if (!jdm_hashmap_contains_key(map, "a")) {
    JDM_LOG_ERROR("hashmap should contain key 'a'.");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  jdm_hashmap_add(map, "b", (u8 *)&(s32){43});
  if (!jdm_hashmap_contains_key(map, "b")) {
    JDM_LOG_ERROR("hashmap should contain key 'b'.");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  jdm_hashmap_add(map, "c", (u8 *)&(s32){44});
  if (!jdm_hashmap_contains_key(map, "c")) {
    JDM_LOG_ERROR("hashmap should contain key 'c'.");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }

  if (jdm_hashmap_size(map) != 3) {
    JDM_LOG_ERROR("hashmap size is not expected size.");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }

  if (jdm_hashmap_capacity(map) != 16) {
    JDM_LOG_ERROR("hashmap capacity is not expected size.");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }

  s = jdm_hashmap_get(map, "a", (u8 **)&i);
  if (s != JDM_HASHMAP_OK) {
    JDM_LOG_ERROR("Get failed %d", s);
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  if (*i != 42) {
    JDM_LOG_ERROR("expected to get value 42 at key \"a\"");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  jdm_hashmap_get(map, "b", (u8 **)&i);
  if (*i != 43) {
    JDM_LOG_ERROR("expected to get value 43 at key \"b\"");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  jdm_hashmap_get(map, "c", (u8 **)&i);
  if (*i != 44) {
    JDM_LOG_ERROR("expected to get value 44 at key \"c\"");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  if ((s = jdm_hashmap_get(map, "d", (u8 **)&i)) !=
      JDM_HASHMAP_ERR_KEY_NOT_FOUND) {
    JDM_LOG_ERROR("expected key \"d\" to not be found");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }

  jdm_hashmap_remove(map, "a", (u8 **)&i);
  if (*i != 42) {
    JDM_LOG_ERROR("expected to remove value 42 at key \"a\"");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  jdm_hashmap_remove(map, "b", (u8 **)&i);
  if (*i != 43) {
    JDM_LOG_ERROR("expected to remove value 43 at key \"b\"");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  jdm_hashmap_remove(map, "c", (u8 **)&i);
  if (*i != 44) {
    JDM_LOG_ERROR("expected to remove value 44 at key \"c\"");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  if ((s = jdm_hashmap_remove(map, "d", (u8 **)&i)) !=
      JDM_HASHMAP_ERR_KEY_NOT_FOUND) {
    JDM_LOG_ERROR("expected key \"d\" to not be found");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }

  if (jdm_hashmap_size(map) != 0) {
    JDM_LOG_ERROR("hashmap size is not expected size.");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }

  jdm_hashmap_add(map, "a", (u8 *)&(s32){42});
  jdm_hashmap_add(map, "b", (u8 *)&(s32){43});
  jdm_hashmap_add(map, "c", (u8 *)&(s32){44});
  if (jdm_hashmap_size(map) != 3) {
    JDM_LOG_ERROR("hashmap size is not expected size.");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }

  jdm_hashmap_remove_all(map);
  if (jdm_hashmap_size(map) != 0) {
    JDM_LOG_ERROR("hashmap size is not expected size.");
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }

  destroy_test_ctx(ctx);
  return JDM_TRUE;
}

static b8 test_jdm_hashmap_bulk_ops(void) {
  test_ctx ctx = get_test_ctx();
  jdm_alloc_t *alloc = ctx.test_alloc;
  jdm_hashmap_t *map = ctx.test_map;
  jdm_hashmap_status_t s;

  jdm_hashmap_add(map, "a", (u8 *)&(s32){42});
  jdm_hashmap_add(map, "b", (u8 *)&(s32){43});
  jdm_hashmap_add(map, "c", (u8 *)&(s32){44});

  const char *expected_keys[] = {"a", "b", "c"};
  jdm_array_t *keys = JDM_NULL;
  jdm_hashmap_get_keys(map, &keys);
  for (sz i = 0; i < 3; i++) {
    jdm_array_status_t as;
    char **k = jdm_alloc(alloc, sizeof(char *));
    as = jdm_array_get_at(keys, (u8 **)&k, i);
    b8 key_found = JDM_FALSE;
    for (sz j = 0; j < 3; j++) {
      if (strcmp(*k, expected_keys[j]) == 0) {
        key_found = JDM_TRUE;
      }
    }
    if (!key_found) {
      JDM_LOG_ERROR("Keys not found, '%s'", *k);
      jdm_free(alloc, k);
      jdm_array_destroy(keys);
      destroy_test_ctx(ctx);
      return JDM_FALSE;
    }
    jdm_free(alloc, k);
  }
  jdm_array_destroy(keys);

  const s32 expected_values[] = {42, 43, 44};
  jdm_array_t *vals = JDM_NULL;
  jdm_hashmap_get_values(map, &vals);
  for (sz i = 0; i < 3; i++) {
    jdm_array_status_t as;
    s32 *v = jdm_alloc(alloc, sizeof(s32));
    as = jdm_array_get_at(vals, (u8 **)&v, i);
    b8 val_found = JDM_FALSE;
    for (sz j = 0; j < 3; j++) {
      if (*v == expected_values[j]) {
        val_found = JDM_TRUE;
      }
    }
    if (!val_found) {
      JDM_LOG_ERROR("val not found, '%d'", *v);
      jdm_free(alloc, v);
      jdm_array_destroy(vals);
      destroy_test_ctx(ctx);
      return JDM_FALSE;
    }
    jdm_free(alloc, v);
  }
  jdm_array_destroy(vals);

  destroy_test_ctx(ctx);
  return JDM_TRUE;
}

static void log_key(const u8 *k) { JDM_LOG_INFO("Key %s", k); }

static void log_value(u8 *v) { JDM_LOG_INFO("Value %d", *(s32 *)v); }

static b8 test_jdm_hashmap_for_each(void) {
  test_ctx ctx = get_test_ctx();
  jdm_alloc_t *alloc = ctx.test_alloc;
  jdm_hashmap_t *map = ctx.test_map;
  jdm_hashmap_status_t s;

  jdm_hashmap_add(map, "a", (u8 *)&(s32){42});
  jdm_hashmap_add(map, "b", (u8 *)&(s32){43});
  jdm_hashmap_add(map, "c", (u8 *)&(s32){44});

  jdm_hashmap_for_each_key(map, log_key);
  jdm_hashmap_for_each_value(map, log_value);

  destroy_test_ctx(ctx);
  return JDM_TRUE;
}

static b8 test_jdm_hashmap_iterator(void) {
  test_ctx ctx = get_test_ctx();
  jdm_alloc_t *alloc = ctx.test_alloc;
  jdm_hashmap_t *map = ctx.test_map;
  jdm_hashmap_status_t s;

  jdm_hashmap_add(map, "a", (u8 *)&(s32){42});
  jdm_hashmap_add(map, "b", (u8 *)&(s32){43});
  jdm_hashmap_add(map, "c", (u8 *)&(s32){44});

  jdm_hashmap_iterator_t it = jdm_hashmap_iterator_create(map);

  jdm_hashmap_entry_t *kv = JDM_NULL;
  s32 *v = jdm_alloc(alloc, sizeof(s32));
  jdm_hashmap_iterator_next(&it, &kv);
  jdm_hashmap_iterator_remove(&it, (u8 **)&v);
  if (*v != 43) {
    JDM_LOG_ERROR("v not equal to 43");
    jdm_free(alloc, v);
    destroy_test_ctx(ctx);
    return JDM_FALSE;
  }
  jdm_hashmap_iterator_next(&it, &kv);
  jdm_free(alloc, v);

  destroy_test_ctx(ctx);
  return JDM_TRUE;
}

int main() {
  jdm_log_init();
  if (test_jdm_hashmap_create_destroy() != JDM_TRUE) {
    JDM_LOG_ERROR("Error in test_jdm_hashmap_create_destroy");
    return 1;
  }
  if (test_jdm_hashmap_element_ops() != JDM_TRUE) {
    JDM_LOG_ERROR("Error in test_jdm_hashmap_element_ops");
    return 1;
  }
  if (test_jdm_hashmap_bulk_ops() != JDM_TRUE) {
    JDM_LOG_ERROR("Error in test_jdm_hashmap_bulk_ops");
    return 1;
  }
  if (test_jdm_hashmap_for_each() != JDM_TRUE) {
    JDM_LOG_ERROR("Error in test_jdm_hashmap_for_each");
    return 1;
  }
  jdm_log_cleanup();
}

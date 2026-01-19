/*
 * Copyright (c) 2026 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>

#define __PANDORA_CB_NAME(domain, name, suffix) CONCAT(domain, _, name, _, suffix)

// Helper to extract just argument names from type-name pairs
// Example: (int, x, float, y) -> x, y
#define _CB_ARG_NAME_2(t1, a1)                                  a1
#define _CB_ARG_NAME_3(t1, a1, n)                               a1
#define _CB_ARG_NAME_4(t1, a1, t2, a2)                          a1, a2
#define _CB_ARG_NAME_6(t1, a1, t2, a2, t3, a3)                  a1, a2, a3
#define _CB_ARG_NAME_8(t1, a1, t2, a2, t3, a3, t4, a4)          a1, a2, a3, a4
#define _CB_ARG_NAME_10(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5) a1, a2, a3, a4, a5
#define _CB_ARG_NAME(...)                                       IDENTITY(_CONCAT(_CB_ARG_NAME_, NUM_VA_ARGS(__VA_ARGS__)))(__VA_ARGS__)

// Helper to build function parameter list
// Example: (int, x, float, y) -> int x, float y
#define _CB_PARAMS_2(t1, a1)                                  t1 a1
#define _CB_PARAMS_3(t1, a1, n)                               t1 a1
#define _CB_PARAMS_4(t1, a1, t2, a2)                          t1 a1, t2 a2
#define _CB_PARAMS_6(t1, a1, t2, a2, t3, a3)                  t1 a1, t2 a2, t3 a3
#define _CB_PARAMS_8(t1, a1, t2, a2, t3, a3, t4, a4)          t1 a1, t2 a2, t3 a3, t4 a4
#define _CB_PARAMS_10(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5) t1 a1, t2 a2, t3 a3, t4 a4, t5 a5
#define _CB_PARAMS(...)                                       IDENTITY(_CONCAT(_CB_PARAMS_, NUM_VA_ARGS(__VA_ARGS__)))(__VA_ARGS__)

#define _DEFINE_PANDORA_CB_REGISTER(name)                                                          \
	static inline void name##_register(struct name##_cb *cb)                                   \
	{                                                                                          \
		__ASSERT_NO_MSG(cb != NULL);                                                       \
		sys_slist_append(&name##_list, &cb->node);                                         \
	}

#define _DEFINE_PANDORA_CB_UNREGISTER(name)                                                        \
	static inline void name##_unregister(struct name##_cb *cb)                                 \
	{                                                                                          \
		__ASSERT_NO_MSG(cb != NULL);                                                       \
		(void)sys_slist_find_and_remove(&name##_list, &cb->node);                          \
	}

#define _DEFINE_PANDORA_CB_ENABLED(name)                                                           \
	static inline int name##_enabled(struct name##_cb *cb)                                     \
	{                                                                                          \
		sys_snode_t *tmp;                                                                  \
		__ASSERT_NO_MSG(cb != NULL);                                                       \
		return sys_slist_find(&name##_list, &cb->node, &tmp);                              \
	}

#define _DEFINE_PANDORA_CB_CALL(name, ...)                                                         \
	static inline void name##_call(_CB_PARAMS(__VA_ARGS__))                                    \
	{                                                                                          \
		struct _CONCAT(name, _cb) * cb;                                                    \
		SYS_SLIST_FOR_EACH_CONTAINER(&name##_list, cb, node) {                             \
			if (cb->dev == dev || cb->dev == NULL) {                                   \
				cb->handler(_CB_ARG_NAME(__VA_ARGS__));                            \
			}                                                                          \
		}                                                                                  \
	}

#define _DECLARE_PANDORA_CB(name, ...)                                                             \
	extern sys_slist_t name##_list;                                                            \
	struct _CONCAT(name, _cb) {                                                                \
		const struct device *dev;                                                          \
		sys_snode_t node;                                                                  \
		void (*handler)(_CB_PARAMS(__VA_ARGS__));                                          \
	};                                                                                         \
	_DEFINE_PANDORA_CB_REGISTER(name);                                                         \
	_DEFINE_PANDORA_CB_UNREGISTER(name);                                                       \
	_DEFINE_PANDORA_CB_ENABLED(name);                                                          \
	_DEFINE_PANDORA_CB_CALL(name, __VA_ARGS__);

#define DECLARE_PANDORA_CB_PARAMS(domain, name, ...)                                               \
	_DECLARE_PANDORA_CB(domain##_##name, const struct device *, dev, __VA_ARGS__)

#define _PANDORA_CB_PARAMS(name, instance_name, _dev, ...)                                         \
	static void instance_name##_handler(_CB_PARAMS(__VA_ARGS__));                              \
	struct name##_cb instance_name = {.handler = instance_name##_handler, .dev = _dev};        \
	static void instance_name##_handler(_CB_PARAMS(__VA_ARGS__))

#define PANDORA_CB_PARAMS(domain, name, instance_name, _dev, ...)                                  \
	_PANDORA_CB_PARAMS(domain##_##name, instance_name, _dev, const struct device *, dev,       \
			   __VA_ARGS__)

#define DEFINE_PANDORA_CB(domain, name)                                                            \
	sys_slist_t __PANDORA_CB_NAME(domain, name, list) =                                        \
		SYS_SLIST_STATIC_INIT(&__PANDORA_CB_NAME(domain, name, list));

#define __PANDORA_CB_PARAMS(domain, name) IDENTITY(__PANDORA_CB_NAME(domain, name, params))

#define DECLARE_PANDORA_CB(domain, name)                                                           \
	DECLARE_PANDORA_CB_PARAMS(domain, name, __PANDORA_CB_PARAMS(domain, name))

#define PANDORA_CB(domain, name, instance_name, dev)                                               \
	PANDORA_CB_PARAMS(domain, name, instance_name, dev, __PANDORA_CB_PARAMS(domain, name))

#define DT_PANDORA_CB(_node, domain, name)                                                         \
	PANDORA_CB(domain, name, CONCAT(domain, _, name, _, DT_NODE_FULL_NAME_TOKEN(_node)),       \
		   DEVICE_DT_GET(_node))

#define pandora_cb_call(domain, name, ...)                                                         \
	IDENTITY(__PANDORA_CB_NAME(domain, name, call))(__VA_ARGS__)

#define pandora_cb_enable(domain, name, ...)                                                       \
	IDENTITY(__PANDORA_CB_NAME(domain, name, register))(__VA_ARGS__)

#define pandora_cb_disable(domain, name, ...)                                                      \
	IDENTITY(__PANDORA_CB_NAME(domain, name, unregister))(__VA_ARGS__)

#define pandora_cb_enabled(domain, name, ...)                                                      \
	IDENTITY(__PANDORA_CB_NAME(domain, name, enabled))(__VA_ARGS__)

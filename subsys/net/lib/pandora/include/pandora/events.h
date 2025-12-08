/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#ifndef PANDORA_EVENTS_H
#define PANDORA_EVENTS_H

/* List of events */
#define PANDORA_EVENT_CONNECTED           BIT(0)
#define PANDORA_EVENT_DISCONNECTED        BIT(1)
#define PANDORA_EVENT_DISCOVERY_STARTED   BIT(2)
#define PANDORA_EVENT_DISCOVERY_COMPLETED BIT(3)
#define PANDORA_EVENT_DISCOVERY_FAILED    BIT(4)

#define PANDORA_EVENT_ALL 0x1F

void pandora_event_init(void);
void pandora_event_post_discovery_completed(void);
void pandora_event_post_discovery_failed(void);
void pandora_event_post_connected(void);
void pandora_event_post_disconnected(void);
int pandora_event_wait_any(void);

#endif /* PANDORA_EVENTS_H */

/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2025 Alexandre Bailon
 */

#ifndef HERMES_SERVICE_H
#define HERMES_SERVICE_H

void hermes_run(void);

void hermes_state_discovery_completed(void);
void hermes_state_discovery_failed(void);
void hermes_state_disconnected(void);

#endif /* HERMES_SERVICE_H */

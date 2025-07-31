#ifndef NARADA_STATE
#define NARADA_STATE

void narada_run(void);
void narada_pairing_started();
void narada_pairing_ended();

/* Discovery state notifications */
void narada_state_discovery_completed(void);
void narada_state_discovery_failed(void);

#endif /* NARADA_STATE */

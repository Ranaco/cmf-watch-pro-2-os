#ifndef CMF_TRANSPORT_H
#define CMF_TRANSPORT_H
#include <stdbool.h>
void transport_init(void);
void transport_poll(void);
bool transport_connected(void);
#endif

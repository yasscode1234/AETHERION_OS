#ifndef MOUSE_H
#define MOUSE_H
#include "types.h"
void mouse_init(void);
int mouse_get_event(int* dx, int* dy, int* buttons);
#endif

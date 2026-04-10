#ifndef _ALTMODE_H
#define _ALTMODE_H

#include <stdint.h>

bool altmode_enable_dp(int flipped);
bool altmode_set(uint8_t conf);
int tmuxhs4446_read(uint8_t* buf);

#endif

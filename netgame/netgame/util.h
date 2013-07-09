#ifndef _NETGAME_UTIL_H
#define _NETGAME_UTIL_H

inline void NETGAME_ASSERT(bool b) {
	if (!b)
		__asm int 3;
}

#endif
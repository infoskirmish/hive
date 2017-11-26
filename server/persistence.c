#include "compat.h"
#include "persistence.h"

#if defined LINUX

int EnablePersistence(char* beaconIP, int beaconPort)
{
//TODO: just to silence the compiler warning
beaconIP++;
beaconPort++;

	return 0;
}

#elif defined SOLARIS

int EnablePersistence(char* beaconIP, int beaconPort)
{
//TODO: just to silence the compiler warning
beaconIP++;
beaconPort++;

	return 0;
}

#endif

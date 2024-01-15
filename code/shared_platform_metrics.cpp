
#include <windows.h>
#include <stdint.h>

static uint64_t GetOSTimerFreq(void)
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

static uint64_t ReadOSTimer(void)
{
	LARGE_INTEGER Value;
	QueryPerformanceCounter(&Value);
	return Value.QuadPart;
}

inline uint64_t ReadCPUTimer(void)
{
	return __rdtsc();
}

static uint64_t EstimateCPUTimerFreq(void)
{
	uint64_t MillisecondsToWait = 100;
	uint64_t OSFreq = GetOSTimerFreq();

	uint64_t CPUStart = ReadCPUTimer();
	uint64_t OSStart = ReadOSTimer();
	uint64_t OSEnd = 0;
	uint64_t OSElapsed = 0;
	uint64_t OSWaitTime = OSFreq * MillisecondsToWait / 1000;

	while(OSElapsed < OSWaitTime)
	{
		OSEnd = ReadOSTimer();
		OSElapsed = OSEnd - OSStart;
	}
	
	uint64_t CPUEnd = ReadCPUTimer();
	uint64_t CPUElapsed = CPUEnd - CPUStart;
	
	uint64_t CPUFreq = 0;
	if(OSElapsed)
	{
		CPUFreq = OSFreq * CPUElapsed / OSElapsed;
	}
	
	return CPUFreq;
}

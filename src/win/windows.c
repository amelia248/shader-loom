#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#ifdef _DEBUG

#pragma comment(lib, "psapi.lib")

size_t getProcessMemoryKilobytes() {
	HANDLE  proc;
	PROCESS_MEMORY_COUNTERS_EX counter;
	WINBOOL success;

	proc = GetCurrentProcess();
	success = K32GetProcessMemoryInfo(proc, (PROCESS_MEMORY_COUNTERS*)&counter, sizeof(counter));
	if (success) return counter.PrivateUsage / 1024;

	return 0;
}
#else
#define getProcessMemoryKilobytes() 0
#endif

void sleep(int milliseconds) {
	Sleep(milliseconds);
}

/* Windows sleep in 100ns units */
BOOLEAN nanosleep(LONGLONG ns) {
	/* Declarations */
	HANDLE timer;   /* Timer handle */
	LARGE_INTEGER li;   /* Time defintion */
	/* Create timer */
	if(!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
		return FALSE;
	/* Set timer properties */
	li.QuadPart = -ns;
	if(!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)){
		CloseHandle(timer);
		return FALSE;
	}
	/* Start & wait for timer */
	WaitForSingleObject(timer, INFINITE);
	/* Clean resources */
	CloseHandle(timer);
	/* Slept without problems */
	return TRUE;
}
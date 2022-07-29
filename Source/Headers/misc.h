//
// misc.h
//

// Externals
#include "game.h"

#define SERIAL_LENGTH      12


extern	void ShowSystemErr(long err);
void	DoAlert(const char* format, ...);
POMME_NORETURN void DoFatalAlert(const char* format, ...);
extern void	Wait(long);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	void FloatToString(float num, Str255 string);
extern	Handle	AllocHandle(long size);
extern	void *AllocPtr(long size);
void *AllocPtrClear(long size);
extern	void VerifySystem(void);
extern	float RandomFloat(void);
u_short	RandomRange(unsigned short min, unsigned short max);
extern	void ShowSystemErr_NonFatal(long err);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);
void SafeDisposePtr(void *ptr);
void MyFlushEvents(void);

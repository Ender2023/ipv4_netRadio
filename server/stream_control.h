#ifndef __STREAM_CONTROL_H__
#define __STREAM_CONTROL_H__

#include <stdbool.h>

int tokenBucket_Create(int CPS, int burst, bool runnow);
int tokenBucket_Destruct(int tbd);

int tokenBucket_fetchToken(int tbd, int size);
int tokenBucket_returnToken(int tbd, int size);
int tokenBucket_Pause(int tbd);
int tokenBucket_Resume(int tbd);

#endif


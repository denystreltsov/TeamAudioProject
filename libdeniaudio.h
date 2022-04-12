#ifndef LIB_DENI_AUDIO_USE
#define LIB_DENI_AUDIO_USE
#include <sndfile.h>
#include <portaudio.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#define CHANNEL_COUNT 2
#define SAMPLE_RATE 44100
#define BUFFER_SIZE 128


int AudioInit();
int AudioEnd();

int LoadAudio(const char*);
int LoadTwoAudio(const char*, const char*);
int MixingTwoAudio(const char*, const char*);
int PlayAudio();
void StopAudio();
void TerminateAudio();


const char* AudioHostApiGet();

int CallBack(
    const void *input, void *output,
    unsigned long bufferSize,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData );

typedef struct AudioManager AudioManager;
#endif
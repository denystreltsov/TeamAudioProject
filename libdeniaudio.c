#include "libdeniaudio.h"

#define PA_ERROR_CHECK(pa) if(pa != paNoError) {printf("%s\n",Pa_GetErrorText(pa)); return 1;}
#define SND_ERROR_CHECK(snd) if(snd == NULL) {printf("%s\n",sf_strerror(snd)); return 1;}
 

typedef float SAMPLE_TYPE;

enum STATE_AUDIO
{
    PLAY = 0,
    PAUSE = 1
}; 

struct AudioManager
{
    float* buffer;
    PaStream* mainStream;
    PaStreamParameters outputStream;
    PaError errorCheck;
    enum STATE_AUDIO stateAudio;
    size_t bufferCountLeft;
};




static AudioManager audioManager;
static float* inner_buffer;
static float* tmp_buffer;



int AudioInit()
{
    audioManager.errorCheck = Pa_Initialize();
    PA_ERROR_CHECK(audioManager.errorCheck)
    return 0;
}

int AudioEnd()
{
    free(tmp_buffer);
    free(audioManager.buffer);
    Pa_Terminate();
    return 0;

}

int LoadAudio(const char* path1)
{
    SF_INFO sfInfo1;
    sfInfo1.format = 0;

    SNDFILE* sndFile1 = sf_open(path1,SFM_READ, &sfInfo1);
    SND_ERROR_CHECK(sndFile1)

    size_t firstDuration = sfInfo1.frames * sfInfo1.channels;

    size_t bufferSize = firstDuration;

    audioManager.bufferCountLeft = bufferSize / CHANNEL_COUNT;

    audioManager.buffer = (float*) malloc(bufferSize * sizeof(float) * 2);

    sf_read_float(sndFile1,audioManager.buffer, firstDuration);
    SND_ERROR_CHECK(sndFile1)


    int err1 = sf_close(sndFile1);
    if(err1 != 0 )
    {
        printf("Error with close file:%d", err1);
    }

    return 0;
}




int LoadTwoAudio(const char* path1, const char* path2)
{
    SF_INFO sfInfo1, sfInfo2;
    sfInfo1.format = 0;
    sfInfo2.format = 0;

    SNDFILE* sndFile1 = sf_open(path1,SFM_READ, &sfInfo1);
    SND_ERROR_CHECK(sndFile1)

    SNDFILE* sndFile2 = sf_open(path2,SFM_READ, &sfInfo2);
    SND_ERROR_CHECK(sndFile2)


    size_t firstDuration = sfInfo1.frames * sfInfo1.channels;
    size_t secondDuration = sfInfo2.frames * sfInfo2.channels;

    size_t bufferSize = firstDuration + secondDuration;

    audioManager.bufferCountLeft = bufferSize / CHANNEL_COUNT;

    audioManager.buffer = (float*) malloc(bufferSize * sizeof(float));

    sf_read_float(sndFile1,audioManager.buffer, firstDuration);
    SND_ERROR_CHECK(sndFile1)
    sf_read_float(sndFile2, audioManager.buffer + firstDuration + 1, secondDuration);
    SND_ERROR_CHECK(sndFile2)


    int err1 = sf_close(sndFile1);
    if(err1 != 0 )
    {
        printf("Error with close file:%d", err1);
    }

    int err2 = sf_close(sndFile2);
    if(err2 != 0 )
    {
        printf("Error with close file:%d", err2);
    }

    return 0;

}

int MixingTwoAudio(const char* path1, const char* path2)
{
    SF_INFO sfInfo1, sfInfo2;
    sfInfo1.format = 0;
    sfInfo2.format = 0;
    size_t bufferSize = 0;

    SNDFILE* sndFile1 = sf_open(path1,SFM_READ, &sfInfo1);
    SND_ERROR_CHECK(sndFile1)

    SNDFILE* sndFile2 = sf_open(path2,SFM_READ, &sfInfo2);
    SND_ERROR_CHECK(sndFile2)

    size_t smallestBuff = 0;
    size_t firstDuration = sfInfo1.frames * sfInfo1.channels;
    size_t secondDuration = sfInfo2.frames * sfInfo2.channels;

    if(secondDuration > firstDuration)
    {
        bufferSize = secondDuration;
        smallestBuff = firstDuration;
    }
    else if(secondDuration == firstDuration)
    {
        bufferSize = secondDuration;
        smallestBuff = bufferSize;
    }
    else if(secondDuration < firstDuration)
    {
        bufferSize = firstDuration;
        smallestBuff = secondDuration;
    }

   

    audioManager.bufferCountLeft = bufferSize / CHANNEL_COUNT;

    audioManager.buffer = (float*) malloc(bufferSize * sizeof(float) * CHANNEL_COUNT);
    tmp_buffer = (float*) malloc(smallestBuff * sizeof(float) * CHANNEL_COUNT);

    sf_read_float(sndFile1,audioManager.buffer, bufferSize);
    SND_ERROR_CHECK(sndFile1)
    sf_read_float(sndFile2, tmp_buffer, secondDuration);
    SND_ERROR_CHECK(sndFile2)

    for(int i = 0; i < smallestBuff; ++i)
    {
        audioManager.buffer[i] += tmp_buffer[i];
    }

    int err1 = sf_close(sndFile1);
    if(err1 != 0 )
    {
        printf("Error with close file:%d", err1);
    }

    int err2 = sf_close(sndFile2);
    if(err2 != 0 )
    {
        printf("Error with close file:%d", err2);
    }

    return 0;

}



int PlayAudio()
{
    audioManager.mainStream = NULL;
    
    audioManager.outputStream.device = Pa_GetDefaultOutputDevice();
    audioManager.outputStream.channelCount = CHANNEL_COUNT;
    audioManager.outputStream.sampleFormat = paFloat32;
    audioManager.outputStream.suggestedLatency = Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultLowOutputLatency;
    audioManager.outputStream.hostApiSpecificStreamInfo = NULL;

    audioManager.errorCheck = Pa_OpenStream(&audioManager.mainStream,NULL,&audioManager.outputStream, SAMPLE_RATE, BUFFER_SIZE, paNoFlag, CallBack, NULL);
    PA_ERROR_CHECK(audioManager.errorCheck)


    inner_buffer = audioManager.buffer;
    audioManager.errorCheck = Pa_StartStream(audioManager.mainStream);
    PA_ERROR_CHECK(audioManager.errorCheck)
    audioManager.stateAudio = PLAY;

    while(Pa_IsStreamActive(audioManager.mainStream))
        continue;

    audioManager.errorCheck = Pa_CloseStream(audioManager.mainStream);
    PA_ERROR_CHECK(audioManager.errorCheck)

    return 0;

}    



const char* AudioHostApiGet()
{
    return Pa_GetHostApiInfo(Pa_GetDefaultHostApi())->name;
}



int CallBack(
    const void *input, void *output,
    unsigned long bufferSize,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData )
    {
        SAMPLE_TYPE* outBuff = (SAMPLE_TYPE*) output;

        unsigned long sizeToFill = (audioManager.bufferCountLeft <= bufferSize) ? 
            audioManager.bufferCountLeft : bufferSize;


        for(int i = 0; i < sizeToFill; ++i)
        {
            *outBuff++ =  *inner_buffer++;
            *outBuff++ =  *inner_buffer++;
        }

        audioManager.bufferCountLeft -= bufferSize;
        if(audioManager.bufferCountLeft <= 0)
        {
            return paComplete;
        }

        else if(audioManager.stateAudio == PLAY)
            return paContinue;
        
        else if(audioManager.stateAudio == PAUSE)
            return paComplete;
        

        return 0;
    }

#ifndef PLATFORM_AUDIO_H
#define PLATFORM_AUDIO_H

#include <alsa/asoundlib.h>
#include <stdint.h>

#define AUDIO_GENERATE_BUFFER_FRAMES 4096
#define AUDIO_OUTPUT_BUFFER_FRAMES 4096

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2

typedef struct AudioState AudioState;

AudioState *audioInit(void);
void audioShutdown(AudioState *audio);
int linuxAudioRequestedFrames(AudioState *audio);
void audioUpdate(AudioState *audio, int16_t *tempBuffer,
                 int16_t *tempOutputBuffer, int framesToGenerate);

#endif

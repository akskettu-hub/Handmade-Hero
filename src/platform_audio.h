#ifndef PLATFORM_AUDIO_H
#define PLATFORM_AUDIO_H

#include <alsa/asoundlib.h>
#include <stdint.h>

#define AUDIO_GENERATE_BUFFER_FRAMES 4096
#define AUDIO_OUTPUT_BUFFER_FRAMES 4096

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2

// NOTE: These typedefs were moved here temporarily to give game audio info
// about sound setup

typedef struct AudioState AudioState;

AudioState *audio_init(void);
void audio_shutdown(AudioState *audio);
int linuxAudioRequestedFrames(AudioState *audio);
void audio_update(AudioState *audio, int16_t *temp_buffer,
                  int16_t *temp_output_buffer, int frames_to_generate);

#endif

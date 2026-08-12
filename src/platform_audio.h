#ifndef PLATFORM_AUDIO_H
#define PLATFORM_AUDIO_H

// #include <cstdint>
#include <stdint.h>

#define AUDIO_GENERATE_BUFFER_FRAMES 4096
#define AUDIO_OUTPUT_BUFFER_FRAMES 4096

typedef struct AudioState AudioState;

AudioState *audio_init(void);
void audio_generate(AudioState *audio, int16_t *buffer, int frames);
int audio_output(AudioState *audio, int16_t *output_buffer);
void audio_update(AudioState *audio, int16_t *temp_buffer,
                  int16_t *temp_output_buffer);
void audio_shutdown(AudioState *audio);

#endif

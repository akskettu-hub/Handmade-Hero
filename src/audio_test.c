#include <alsa/asoundlib.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  snd_pcm_t *pcm;

  int result = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);

  if (result < 0) {
    fprintf(stderr, "Could not open audio device %s\n", snd_strerror(result));
    return 1;
  }

  int sample_rate = 48000;
  int channels = 2;

  result = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE,
                              SND_PCM_ACCESS_RW_INTERLEAVED, channels,
                              sample_rate, 1, 500000);

  if (result < 0) {
    fprintf(stderr, "Could not configure audio device: %s\n",
            snd_strerror(result));
    snd_pcm_close(pcm);
    return 1;
  }

  const int frames = 1024;
  int16_t *buffer = malloc(frames * channels * sizeof(int16_t));

  if (!buffer) {
    snd_pcm_close(pcm);
    fprintf(stderr, "Could not allocate buffer\n");
    return 1;
  }

  double phase = 0.0;
  double frequency = 440.0;

  for (int i = 0; i < 48000 * 3;) {
    for (int frame = 0; frame < frames; frame++) {
      double value = sin(phase * 2.0 * M_PI);

      int16_t sample = (int16_t)(value * 3000);

      buffer[frame * 2 + 0] = sample;
      buffer[frame * 2 + 1] = sample;

      phase += frequency / sample_rate;

      if (phase >= 1.0) {
        phase -= 1.0;
      }
    }
    snd_pcm_sframes_t written = snd_pcm_writei(pcm, buffer, frames);

    if (written < 0) {
      written = snd_pcm_recover(pcm, written, 0);
    }

    if (written < 0) {
      fprintf(stderr, "Audio write failed %s\n", snd_strerror(written));
      break;
    }
    i += written;
  }
  free(buffer);

  printf("Got here with no issues!");
  return 0;
}

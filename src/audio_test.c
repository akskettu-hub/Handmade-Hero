#include <alsa/asoundlib.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct { // represents audio device and its current state
  snd_pcm_t *pcm;

  int sample_rate;
  int channels;

  double phase; // phase lives between calls and so belongs in persistent audio
                // state
  double frequency;
} LinuxAudio;

static int linux_audio_init(LinuxAudio *audio) {
  int result = snd_pcm_open(&audio->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);

  if (result < 0) {
    fprintf(stderr, "Could not open audio device %s\n", snd_strerror(result));
    return 0;
  }

  audio->sample_rate = 48000;
  audio->channels = 2;
  audio->phase = 0.0;
  audio->frequency = 440.0;

  result = snd_pcm_set_params(audio->pcm, SND_PCM_FORMAT_S16_LE,
                              SND_PCM_ACCESS_RW_INTERLEAVED, audio->channels,
                              audio->sample_rate, 1, 500000);

  if (result < 0) {
    fprintf(stderr, "Could not configure audio device: %s\n",
            snd_strerror(result));
    snd_pcm_close(audio->pcm);
    audio->pcm = NULL;
    return 0;
  }

  return 1;
}

static void linux_audio_shutdown(LinuxAudio *audio) {
  if (audio->pcm) {
    snd_pcm_drain(audio->pcm);
    snd_pcm_close(audio->pcm);
    audio->pcm = NULL;
  }
}

static void generate_audio_sine(LinuxAudio *audio, int16_t *buffer,
                                int frames) {
  for (int frame = 0; frame < frames; frame++) {
    double value = sin(audio->phase * 2.0 * M_PI);

    int16_t sample = (int16_t)(value * 3000);

    buffer[frame * 2 + 0] = sample;
    buffer[frame * 2 + 1] = sample;

    audio->phase += audio->frequency / audio->sample_rate;

    if (audio->phase >= 1.0) {
      audio->phase -= 1.0;
    }
  }
}
static int linux_audio_output(LinuxAudio *audio, int16_t *buffer, int frames) {
  snd_pcm_sframes_t written = snd_pcm_writei(audio->pcm, buffer, frames);

  if (written < 0) {
    written = snd_pcm_recover(audio->pcm, written, 0);
  }

  if (written < 0) {
    fprintf(stderr, "Audio write failed %s\n", snd_strerror(written));
    return 0;
  }
  return 1;
}

int main(void) {
  LinuxAudio audio = {0};

  if (!linux_audio_init(&audio)) {
    fprintf(stderr, "Could not initialise linux audio.\n");
    return 1;
  }

  const int frames = 1024;
  int16_t *buffer = malloc(frames * audio.channels * sizeof(int16_t));

  if (!buffer) {
    linux_audio_shutdown(&audio);
    fprintf(stderr, "Could not allocate buffer\n");
    return 1;
  }

  generate_audio_sine(&audio, buffer, frames);
  linux_audio_output(&audio, buffer, frames);

  /*
  for (int i = 0; i < 48000 * 3;) {
    generate_audio_sine(&audio, buffer, frames);

    snd_pcm_sframes_t written = snd_pcm_writei(audio.pcm, buffer, frames);

    if (written < 0) {
      written = snd_pcm_recover(audio.pcm, written, 0);
    }

    if (written < 0) {
      fprintf(stderr, "Audio write failed %s\n", snd_strerror(written));
      break;
    }
    i += written;
  }
  */
  free(buffer);
  linux_audio_shutdown(&audio);

  printf("Got here with no issues!");
  return 0;
}

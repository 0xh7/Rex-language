#include "rex_audio.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define REX_AUDIO_MAYBE_UNUSED __attribute__((unused))
#else
#define REX_AUDIO_MAYBE_UNUSED
#endif

static REX_AUDIO_MAYBE_UNUSED int rex_audio_has_ext(const char* path, const char* ext) {
  if (!path || !ext) {
    return 0;
  }
  size_t plen = strlen(path);
  size_t elen = strlen(ext);
  if (plen < elen) {
    return 0;
  }
  const char* p = path + (plen - elen);
  for (size_t i = 0; i < elen; i++) {
    unsigned char a = (unsigned char)p[i];
    unsigned char b = (unsigned char)ext[i];
    if (tolower(a) != tolower(b)) {
      return 0;
    }
  }
  return 1;
}

static int rex_audio_is_sep(char c) {
  return c == '/' || c == '\\';
}

static REX_AUDIO_MAYBE_UNUSED int rex_audio_extract_ext(const char* value, char* out, size_t out_len) {
  if (!value || !out || out_len == 0) {
    return 0;
  }
  const char* last_sep = NULL;
  const char* p = value;
  while (*p) {
    if (rex_audio_is_sep(*p)) {
      last_sep = p;
    }
    p++;
  }
  const char* last_dot = NULL;
  p = value;
  while (*p) {
    if (*p == '.') {
      if (!last_sep || p > last_sep) {
        last_dot = p;
      }
    }
    p++;
  }
  if (last_dot && last_dot[1] != '\0') {
    size_t len = strlen(last_dot + 1);
    if (len + 1 > out_len) {
      return 0;
    }
    for (size_t i = 0; i < len; i++) {
      out[i] = (char)tolower((unsigned char)last_dot[1 + i]);
    }
    out[len] = '\0';
    return 1;
  }
  if (value[0] == '.' && value[1] != '\0') {
    size_t len = strlen(value + 1);
    if (len + 1 > out_len) {
      return 0;
    }
    for (size_t i = 0; i < len; i++) {
      out[i] = (char)tolower((unsigned char)value[1 + i]);
    }
    out[len] = '\0';
    return 1;
  }
  return 0;
}

static REX_AUDIO_MAYBE_UNUSED double rex_audio_clamp_volume(double v) {
  if (v < 0.0) {
    return 0.0;
  }
  if (v > 1.0) {
    return 1.0;
  }
  return v;
}

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <wchar.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

#define REX_AUDIO_WIN_VOICES 8
#define REX_AUDIO_WIN_PCM_VOICES 8

typedef struct RexAudioWinVoice {
  int opened;
  wchar_t alias[32];
  wchar_t* path;
} RexAudioWinVoice;

typedef struct RexAudioWinPcmVoice {
  int active;
  HWAVEOUT handle;
  WAVEHDR header;
  int16_t* data;
} RexAudioWinPcmVoice;

static RexAudioWinVoice rex_audio_voices[REX_AUDIO_WIN_VOICES];
static RexAudioWinVoice rex_audio_loop_voice;
static RexAudioWinPcmVoice rex_audio_pcm_voices[REX_AUDIO_WIN_PCM_VOICES];
static int rex_audio_win_inited = 0;
static double rex_audio_volume = 1.0;

static wchar_t* rex_audio_widen_path(const char* path) {
  if (!path) {
    return NULL;
  }
  int len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
  UINT cp = CP_UTF8;
  if (len <= 0) {
    cp = CP_ACP;
    len = MultiByteToWideChar(cp, 0, path, -1, NULL, 0);
  }
  if (len <= 0) {
    return NULL;
  }
  wchar_t* wpath = (wchar_t*)malloc((size_t)len * sizeof(wchar_t));
  if (!wpath) {
    return NULL;
  }
  if (MultiByteToWideChar(cp, 0, path, -1, wpath, len) <= 0) {
    free(wpath);
    return NULL;
  }
  return wpath;
}

static wchar_t* rex_audio_wstrdup(const wchar_t* value) {
  if (!value) {
    return NULL;
  }
  size_t len = wcslen(value);
  wchar_t* out = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
  if (!out) {
    return NULL;
  }
  memcpy(out, value, (len + 1) * sizeof(wchar_t));
  return out;
}

static wchar_t* rex_audio_win_fullpath(const char* path) {
  wchar_t* wpath = rex_audio_widen_path(path);
  if (!wpath) {
    return NULL;
  }
  wchar_t fullbuf[MAX_PATH];
  DWORD full_len = GetFullPathNameW(wpath, MAX_PATH, fullbuf, NULL);
  if (full_len > 0 && full_len < MAX_PATH) {
    wchar_t* fullpath = rex_audio_wstrdup(fullbuf);
    free(wpath);
    return fullpath ? fullpath : wpath;
  }
  if (full_len >= MAX_PATH) {
    wchar_t* fullalloc = (wchar_t*)malloc((size_t)(full_len + 1) * sizeof(wchar_t));
    if (fullalloc && GetFullPathNameW(wpath, full_len + 1, fullalloc, NULL) > 0) {
      free(wpath);
      return fullalloc;
    }
    free(fullalloc);
  }
  return wpath;
}

static void rex_audio_win_init(void) {
  if (rex_audio_win_inited) {
    return;
  }
  for (int i = 0; i < REX_AUDIO_WIN_VOICES; i++) {
    swprintf(rex_audio_voices[i].alias, sizeof(rex_audio_voices[i].alias) / sizeof(wchar_t), L"rex_audio_%d", i);
    rex_audio_voices[i].opened = 0;
    rex_audio_voices[i].path = NULL;
  }
  swprintf(rex_audio_loop_voice.alias, sizeof(rex_audio_loop_voice.alias) / sizeof(wchar_t), L"rex_audio_loop");
  rex_audio_loop_voice.opened = 0;
  rex_audio_loop_voice.path = NULL;
  memset(rex_audio_pcm_voices, 0, sizeof(rex_audio_pcm_voices));
  rex_audio_win_inited = 1;
}

static void rex_audio_win_close_voice(RexAudioWinVoice* voice) {
  if (!voice) {
    return;
  }
  if (voice->opened) {
    wchar_t cmd[64];
    swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"stop %ls", voice->alias);
    mciSendStringW(cmd, NULL, 0, NULL);
    swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"close %ls", voice->alias);
    mciSendStringW(cmd, NULL, 0, NULL);
    voice->opened = 0;
  }
  if (voice->path) {
    free(voice->path);
    voice->path = NULL;
  }
}

static void rex_audio_win_stop_voice(RexAudioWinVoice* voice) {
  if (!voice || !voice->opened) {
    return;
  }
  wchar_t cmd[64];
  swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"stop %ls", voice->alias);
  mciSendStringW(cmd, NULL, 0, NULL);
}

static int rex_audio_win_voice_playing(const wchar_t* alias) {
  wchar_t cmd[64];
  wchar_t buf[64];
  swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"status %ls mode", alias);
  if (mciSendStringW(cmd, buf, sizeof(buf) / sizeof(buf[0]), NULL) != 0) {
    return 0;
  }
  return wcsstr(buf, L"playing") != NULL;
}

static void rex_audio_win_apply_volume_voice(RexAudioWinVoice* voice) {
  if (!voice || !voice->opened) {
    return;
  }
  int vol = (int)(rex_audio_volume * 1000.0 + 0.5);
  if (vol < 0) {
    vol = 0;
  } else if (vol > 1000) {
    vol = 1000;
  }
  wchar_t cmd[64];
  swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"setaudio %ls volume %d", voice->alias, vol);
  mciSendStringW(cmd, NULL, 0, NULL);
}

static void rex_audio_win_apply_volume(void) {
  for (int i = 0; i < REX_AUDIO_WIN_VOICES; i++) {
    rex_audio_win_apply_volume_voice(&rex_audio_voices[i]);
  }
  rex_audio_win_apply_volume_voice(&rex_audio_loop_voice);
  DWORD vol = (DWORD)(rex_audio_clamp_volume(rex_audio_volume) * 65535.0 + 0.5);
  DWORD stereo = vol | (vol << 16);
  for (int i = 0; i < REX_AUDIO_WIN_PCM_VOICES; i++) {
    RexAudioWinPcmVoice* voice = &rex_audio_pcm_voices[i];
    if (voice->active && voice->handle) {
      waveOutSetVolume(voice->handle, stereo);
    }
  }
}

static void rex_audio_win_close_pcm_voice(RexAudioWinPcmVoice* voice, int reset) {
  if (!voice) {
    return;
  }
  if (!voice->active && !voice->handle && !voice->data) {
    return;
  }
  if (voice->handle) {
    if (reset) {
      waveOutReset(voice->handle);
    }
    if (voice->header.dwFlags & WHDR_PREPARED) {
      waveOutUnprepareHeader(voice->handle, &voice->header, sizeof(voice->header));
    }
    waveOutClose(voice->handle);
  }
  free(voice->data);
  memset(voice, 0, sizeof(*voice));
}

static void rex_audio_win_gc_pcm_voices(void) {
  for (int i = 0; i < REX_AUDIO_WIN_PCM_VOICES; i++) {
    RexAudioWinPcmVoice* voice = &rex_audio_pcm_voices[i];
    if (!voice->active) {
      continue;
    }
    if (voice->header.dwFlags & WHDR_DONE) {
      rex_audio_win_close_pcm_voice(voice, 0);
    }
  }
}

static RexAudioWinPcmVoice* rex_audio_win_pick_pcm_voice(void) {
  rex_audio_win_gc_pcm_voices();
  for (int i = 0; i < REX_AUDIO_WIN_PCM_VOICES; i++) {
    if (!rex_audio_pcm_voices[i].active) {
      return &rex_audio_pcm_voices[i];
    }
  }
  rex_audio_win_close_pcm_voice(&rex_audio_pcm_voices[0], 1);
  return &rex_audio_pcm_voices[0];
}

static int rex_audio_win_play_mp3_pcm(const wchar_t* fullpath) {
  if (!fullpath || fullpath[0] == L'\0') {
    return 0;
  }

  RexAudioWinPcmVoice* voice = rex_audio_win_pick_pcm_voice();
  if (!voice) {
    return 0;
  }

  mp3dec_t dec;
  mp3dec_file_info_t info;
  memset(&info, 0, sizeof(info));
  mp3dec_init(&dec);
  if (mp3dec_load_w(&dec, fullpath, &info, NULL, NULL) != 0 || !info.buffer) {
    free(info.buffer);
    return 0;
  }
  if (info.samples == 0 || info.channels <= 0 || info.hz <= 0) {
    free(info.buffer);
    return 0;
  }

  size_t bytes = info.samples * sizeof(int16_t);
  if (bytes == 0 || bytes > (size_t)UINT32_MAX) {
    free(info.buffer);
    return 0;
  }

  WAVEFORMATEX wf;
  memset(&wf, 0, sizeof(wf));
  wf.wFormatTag = WAVE_FORMAT_PCM;
  wf.nChannels = (WORD)info.channels;
  wf.nSamplesPerSec = (DWORD)info.hz;
  wf.wBitsPerSample = 16;
  wf.nBlockAlign = (WORD)(wf.nChannels * (wf.wBitsPerSample / 8));
  wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

  if (waveOutOpen(&voice->handle, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
    free(info.buffer);
    return 0;
  }

  voice->data = (int16_t*)info.buffer;
  memset(&voice->header, 0, sizeof(voice->header));
  voice->header.lpData = (LPSTR)voice->data;
  voice->header.dwBufferLength = (DWORD)bytes;

  if (waveOutPrepareHeader(voice->handle, &voice->header, sizeof(voice->header)) != MMSYSERR_NOERROR) {
    rex_audio_win_close_pcm_voice(voice, 1);
    return 0;
  }

  DWORD vol = (DWORD)(rex_audio_clamp_volume(rex_audio_volume) * 65535.0 + 0.5);
  DWORD stereo = vol | (vol << 16);
  waveOutSetVolume(voice->handle, stereo);

  if (waveOutWrite(voice->handle, &voice->header, sizeof(voice->header)) != MMSYSERR_NOERROR) {
    rex_audio_win_close_pcm_voice(voice, 1);
    return 0;
  }

  voice->active = 1;
  return 1;
}

static int rex_audio_win_prepare_voice(RexAudioWinVoice* voice, wchar_t* fullpath, const wchar_t* type) {
  if (!voice || !fullpath) {
    free(fullpath);
    return 0;
  }
  if (voice->opened && voice->path && wcscmp(voice->path, fullpath) == 0) {
    free(fullpath);
    return 1;
  }
  rex_audio_win_close_voice(voice);
  voice->path = fullpath;
  size_t cmd_len = wcslen(voice->path) + 64u;
  wchar_t* cmd = (wchar_t*)malloc(cmd_len * sizeof(wchar_t));
  if (!cmd) {
    rex_audio_win_close_voice(voice);
    return 0;
  }
  if (type) {
    swprintf(cmd, cmd_len, L"open \"%ls\" type %ls alias %ls", voice->path, type, voice->alias);
  } else {
    swprintf(cmd, cmd_len, L"open \"%ls\" alias %ls", voice->path, voice->alias);
  }
  MCIERROR err = mciSendStringW(cmd, NULL, 0, NULL);
  free(cmd);

  // Some Windows installs reject explicit "type" even when the file can open.
  if (err != 0 && type) {
    size_t cmd2_len = wcslen(voice->path) + 48u;
    wchar_t* cmd2 = (wchar_t*)malloc(cmd2_len * sizeof(wchar_t));
    if (cmd2) {
      swprintf(cmd2, cmd2_len, L"open \"%ls\" alias %ls", voice->path, voice->alias);
      err = mciSendStringW(cmd2, NULL, 0, NULL);
      free(cmd2);
    }
  }

  if (err != 0) {
    rex_audio_win_close_voice(voice);
    return 0;
  }
  voice->opened = 1;
  rex_audio_win_apply_volume_voice(voice);
  return 1;
}

static int rex_audio_win_play_voice(RexAudioWinVoice* voice, int loop) {
  if (!voice || !voice->opened) {
    return 0;
  }
  wchar_t cmd[64];
  swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"seek %ls to start", voice->alias);
  mciSendStringW(cmd, NULL, 0, NULL);
  if (loop) {
    swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"play %ls repeat", voice->alias);
  } else {
    swprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"play %ls", voice->alias);
  }
  return mciSendStringW(cmd, NULL, 0, NULL) == 0;
}

static RexAudioWinVoice* rex_audio_win_pick_voice(const wchar_t* fullpath) {
  RexAudioWinVoice* free_voice = NULL;
  RexAudioWinVoice* idle_voice = NULL;
  for (int i = 0; i < REX_AUDIO_WIN_VOICES; i++) {
    RexAudioWinVoice* voice = &rex_audio_voices[i];
    if (!voice->opened) {
      if (!free_voice) {
        free_voice = voice;
      }
      continue;
    }
    if (!rex_audio_win_voice_playing(voice->alias)) {
      if (voice->path && wcscmp(voice->path, fullpath) == 0) {
        return voice;
      }
      if (!idle_voice) {
        idle_voice = voice;
      }
    }
  }
  if (free_voice) {
    return free_voice;
  }
  if (idle_voice) {
    return idle_voice;
  }
  return &rex_audio_voices[0];
}

int rex_audio_platform_play_ex(const char* path, int loop) {
  if (!path || path[0] == '\0') {
    return 0;
  }

  rex_audio_win_init();

  int is_mp3 = rex_audio_has_ext(path, ".mp3");
  const wchar_t* type = NULL;
  if (is_mp3) {
    type = L"mpegvideo";
  } else if (rex_audio_has_ext(path, ".wav")) {
    type = L"waveaudio";
  }

  wchar_t* fullpath = rex_audio_win_fullpath(path);
  if (!fullpath) {
    return 0;
  }

  int tried_pcm = 0;
  if (is_mp3 && !loop) {
    tried_pcm = 1;
    int ok_pcm = rex_audio_win_play_mp3_pcm(fullpath);
    if (ok_pcm) {
      free(fullpath);
      return 1;
    }
  }

  if (loop) {
    if (!rex_audio_win_prepare_voice(&rex_audio_loop_voice, fullpath, type)) {
      return 0;
    }
    return rex_audio_win_play_voice(&rex_audio_loop_voice, 1);
  }

  RexAudioWinVoice* voice = rex_audio_win_pick_voice(fullpath);
  if (!voice) {
    free(fullpath);
    return 0;
  }
  if (!rex_audio_win_prepare_voice(voice, fullpath, type)) {
    return 0;
  }
  if (rex_audio_win_play_voice(voice, 0)) {
    return 1;
  }
  int ok = 0;
  if (is_mp3 && !tried_pcm && voice->path) {
    ok = rex_audio_win_play_mp3_pcm(voice->path);
  }
  rex_audio_win_close_voice(voice);
  if (!ok && is_mp3) {
    fprintf(stderr, "Rex audio: mp3 playback failed: %s\n", path);
  }
  return ok;
}

int rex_audio_platform_play(const char* path) {
  return rex_audio_platform_play_ex(path, 0);
}

void rex_audio_platform_stop(void) {
  rex_audio_win_init();
  for (int i = 0; i < REX_AUDIO_WIN_VOICES; i++) {
    rex_audio_win_stop_voice(&rex_audio_voices[i]);
  }
  rex_audio_win_stop_voice(&rex_audio_loop_voice);
  for (int i = 0; i < REX_AUDIO_WIN_PCM_VOICES; i++) {
    rex_audio_win_close_pcm_voice(&rex_audio_pcm_voices[i], 1);
  }
}

void rex_audio_platform_set_volume(double volume) {
  rex_audio_volume = rex_audio_clamp_volume(volume);
  rex_audio_win_apply_volume();
}

double rex_audio_platform_get_volume(void) {
  return rex_audio_volume;
}

int rex_audio_platform_supports(const char* ext) {
  char buf[16];
  if (!rex_audio_extract_ext(ext, buf, sizeof(buf))) {
    return 0;
  }
  if (strcmp(buf, "wav") == 0) {
    return 1;
  }
  if (strcmp(buf, "mp3") == 0) {
    return 1;
  }
  return 0;
}

#elif defined(__linux__)

#ifndef REX_AUDIO_HAS_ALSA
#if defined(__has_include)
#if __has_include(<alsa/asoundlib.h>)
#define REX_AUDIO_HAS_ALSA 1
#else
#define REX_AUDIO_HAS_ALSA 0
#endif
#else
#define REX_AUDIO_HAS_ALSA 0
#endif
#endif

#if REX_AUDIO_HAS_ALSA

#include <alsa/asoundlib.h>
#include <pthread.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

typedef struct RexAudioBuffer {
  int channels;
  int sample_rate;
  int bits_per_sample;
  size_t frame_count;
  int16_t* data;
} RexAudioBuffer;

typedef struct RexAudioJob {
  char* path;
  int loop;
} RexAudioJob;

static pthread_mutex_t rex_audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t rex_audio_thread;
static int rex_audio_thread_active = 0;
static int rex_audio_stop_requested = 0;
static snd_pcm_t* rex_audio_pcm = NULL;
static double rex_audio_volume = 1.0;

static char* rex_audio_strdup(const char* s) {
  if (!s) {
    return NULL;
  }
  size_t len = strlen(s);
  char* out = (char*)malloc(len + 1u);
  if (!out) {
    return NULL;
  }
  memcpy(out, s, len + 1u);
  return out;
}

static int rex_audio_read_u16(FILE* f, uint16_t* out) {
  uint8_t b[2];
  if (fread(b, 1, 2, f) != 2) {
    return 0;
  }
  *out = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
  return 1;
}

static int rex_audio_read_u32(FILE* f, uint32_t* out) {
  uint8_t b[4];
  if (fread(b, 1, 4, f) != 4) {
    return 0;
  }
  *out = (uint32_t)b[0]
    | ((uint32_t)b[1] << 8)
    | ((uint32_t)b[2] << 16)
    | ((uint32_t)b[3] << 24);
  return 1;
}

static void rex_audio_free_buffer(RexAudioBuffer* buf) {
  if (!buf) {
    return;
  }
  free(buf->data);
  memset(buf, 0, sizeof(*buf));
}

static int rex_audio_load_wav(const char* path, RexAudioBuffer* out) {
  if (!path || !out) {
    return 0;
  }
  memset(out, 0, sizeof(*out));

  FILE* f = fopen(path, "rb");
  if (!f) {
    return 0;
  }

  char riff[4];
  char wave[4];
  uint32_t riff_size = 0;
  if (fread(riff, 1, 4, f) != 4 || memcmp(riff, "RIFF", 4) != 0) {
    fclose(f);
    return 0;
  }
  if (!rex_audio_read_u32(f, &riff_size)) {
    fclose(f);
    return 0;
  }
  if (fread(wave, 1, 4, f) != 4 || memcmp(wave, "WAVE", 4) != 0) {
    fclose(f);
    return 0;
  }

  uint16_t audio_format = 0;
  uint16_t channels = 0;
  uint32_t sample_rate = 0;
  uint16_t bits_per_sample = 0;
  int have_fmt = 0;
  int have_data = 0;
  uint8_t* raw = NULL;
  size_t raw_size = 0;

  while (!have_data) {
    char chunk_id[4];
    uint32_t chunk_size = 0;
    if (fread(chunk_id, 1, 4, f) != 4) {
      break;
    }
    if (!rex_audio_read_u32(f, &chunk_size)) {
      break;
    }
    if (memcmp(chunk_id, "fmt ", 4) == 0) {
      uint16_t block_align = 0;
      uint32_t byte_rate = 0;
      if (chunk_size < 16) {
        break;
      }
      if (!rex_audio_read_u16(f, &audio_format) ||
          !rex_audio_read_u16(f, &channels) ||
          !rex_audio_read_u32(f, &sample_rate) ||
          !rex_audio_read_u32(f, &byte_rate) ||
          !rex_audio_read_u16(f, &block_align) ||
          !rex_audio_read_u16(f, &bits_per_sample)) {
        break;
      }
      (void)byte_rate;
      (void)block_align;
      if (chunk_size > 16) {
        if (fseek(f, (long)(chunk_size - 16), SEEK_CUR) != 0) {
          break;
        }
      }
      have_fmt = 1;
    } else if (memcmp(chunk_id, "data", 4) == 0) {
      if (chunk_size > SIZE_MAX) {
        break;
      }
      raw = (uint8_t*)malloc((size_t)chunk_size);
      if (!raw) {
        break;
      }
      if (fread(raw, 1, (size_t)chunk_size, f) != (size_t)chunk_size) {
        break;
      }
      raw_size = (size_t)chunk_size;
      have_data = 1;
    } else {
      if (fseek(f, (long)chunk_size, SEEK_CUR) != 0) {
        break;
      }
    }
    if (chunk_size & 1u) {
      fseek(f, 1, SEEK_CUR);
    }
  }

  fclose(f);

  if (!have_fmt || !have_data || !raw) {
    free(raw);
    return 0;
  }
  if (channels == 0 || sample_rate == 0 || bits_per_sample == 0) {
    free(raw);
    return 0;
  }
  if (audio_format != 1 && audio_format != 3) {
    free(raw);
    return 0;
  }

  size_t bytes_per_sample = bits_per_sample / 8u;
  if (bytes_per_sample == 0) {
    free(raw);
    return 0;
  }
  size_t total_samples = raw_size / bytes_per_sample;
  size_t frame_count = total_samples / (size_t)channels;
  if (frame_count == 0) {
    free(raw);
    return 0;
  }
  size_t out_samples = frame_count * (size_t)channels;
  if (out_samples > (SIZE_MAX / sizeof(int16_t))) {
    free(raw);
    return 0;
  }
  int16_t* out_data = (int16_t*)malloc(out_samples * sizeof(int16_t));
  if (!out_data) {
    free(raw);
    return 0;
  }

  if (audio_format == 1) {
    if (bits_per_sample == 8) {
      for (size_t i = 0; i < out_samples; i++) {
        uint8_t v = raw[i];
        out_data[i] = (int16_t)(((int)v - 128) << 8);
      }
    } else if (bits_per_sample == 16) {
      for (size_t i = 0; i < out_samples; i++) {
        size_t base = i * 2u;
        int16_t v = (int16_t)((uint16_t)raw[base] | ((uint16_t)raw[base + 1] << 8));
        out_data[i] = v;
      }
    } else if (bits_per_sample == 24) {
      for (size_t i = 0; i < out_samples; i++) {
        size_t base = i * 3u;
        int32_t v = (int32_t)((uint32_t)raw[base]
          | ((uint32_t)raw[base + 1] << 8)
          | ((uint32_t)raw[base + 2] << 16));
        if (v & 0x800000) {
          v |= ~0xFFFFFF;
        }
        out_data[i] = (int16_t)(v >> 8);
      }
    } else if (bits_per_sample == 32) {
      for (size_t i = 0; i < out_samples; i++) {
        size_t base = i * 4u;
        int32_t v = (int32_t)((uint32_t)raw[base]
          | ((uint32_t)raw[base + 1] << 8)
          | ((uint32_t)raw[base + 2] << 16)
          | ((uint32_t)raw[base + 3] << 24));
        out_data[i] = (int16_t)(v >> 16);
      }
    } else {
      free(raw);
      free(out_data);
      return 0;
    }
  } else if (audio_format == 3 && bits_per_sample == 32) {
    for (size_t i = 0; i < out_samples; i++) {
      float v = 0.0f;
      size_t base = i * 4u;
      memcpy(&v, raw + base, sizeof(float));
      if (v > 1.0f) {
        v = 1.0f;
      } else if (v < -1.0f) {
        v = -1.0f;
      }
      out_data[i] = (int16_t)(v * 32767.0f);
    }
  } else {
    free(raw);
    free(out_data);
    return 0;
  }

  free(raw);
  out->channels = (int)channels;
  out->sample_rate = (int)sample_rate;
  out->bits_per_sample = 16;
  out->frame_count = frame_count;
  out->data = out_data;
  return 1;
}

static int rex_audio_load_mp3(const char* path, RexAudioBuffer* out) {
  if (!path || !out) {
    return 0;
  }
  memset(out, 0, sizeof(*out));

  mp3dec_t dec;
  mp3dec_file_info_t info;
  memset(&info, 0, sizeof(info));
  mp3dec_init(&dec);
  int ret = mp3dec_load(&dec, path, &info, NULL, NULL);
  if (ret != 0 || !info.buffer) {
    free(info.buffer);
    return 0;
  }
  if (info.samples == 0 || info.channels <= 0 || info.hz <= 0) {
    free(info.buffer);
    return 0;
  }
  size_t frame_count = info.samples / (size_t)info.channels;
  if (frame_count == 0) {
    free(info.buffer);
    return 0;
  }
  out->channels = info.channels;
  out->sample_rate = info.hz;
  out->bits_per_sample = 16;
  out->frame_count = frame_count;
  out->data = (int16_t*)info.buffer;
  return 1;
}

static void* rex_audio_thread_main(void* arg) {
  RexAudioJob* job = (RexAudioJob*)arg;
  char* path = job ? job->path : NULL;
  int loop = job ? job->loop : 0;
  RexAudioBuffer buffer;
  int ok = 0;
  if (path && rex_audio_has_ext(path, ".mp3")) {
    ok = rex_audio_load_mp3(path, &buffer);
  } else {
    ok = rex_audio_load_wav(path, &buffer);
  }
  if (job) {
    free(job->path);
    free(job);
  }
  if (!ok) {
    pthread_mutex_lock(&rex_audio_mutex);
    rex_audio_thread_active = 0;
    rex_audio_stop_requested = 0;
    pthread_mutex_unlock(&rex_audio_mutex);
    return NULL;
  }

  snd_pcm_t* handle = NULL;
  int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    rex_audio_free_buffer(&buffer);
    pthread_mutex_lock(&rex_audio_mutex);
    rex_audio_thread_active = 0;
    rex_audio_stop_requested = 0;
    pthread_mutex_unlock(&rex_audio_mutex);
    return NULL;
  }

  pthread_mutex_lock(&rex_audio_mutex);
  rex_audio_pcm = handle;
  pthread_mutex_unlock(&rex_audio_mutex);

  snd_pcm_hw_params_t* params = NULL;
  if (snd_pcm_hw_params_malloc(&params) < 0 || !params) {
    snd_pcm_close(handle);
    rex_audio_free_buffer(&buffer);
    pthread_mutex_lock(&rex_audio_mutex);
    rex_audio_pcm = NULL;
    rex_audio_thread_active = 0;
    rex_audio_stop_requested = 0;
    pthread_mutex_unlock(&rex_audio_mutex);
    return NULL;
  }

  if (snd_pcm_hw_params_any(handle, params) < 0 ||
      snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0 ||
      snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE) < 0 ||
      snd_pcm_hw_params_set_channels(handle, params, (unsigned int)buffer.channels) < 0) {
    snd_pcm_hw_params_free(params);
    snd_pcm_close(handle);
    rex_audio_free_buffer(&buffer);
    pthread_mutex_lock(&rex_audio_mutex);
    rex_audio_pcm = NULL;
    rex_audio_thread_active = 0;
    rex_audio_stop_requested = 0;
    pthread_mutex_unlock(&rex_audio_mutex);
    return NULL;
  }
  unsigned int rate = (unsigned int)buffer.sample_rate;
  if (snd_pcm_hw_params_set_rate_near(handle, params, &rate, NULL) < 0 ||
      snd_pcm_hw_params(handle, params) < 0) {
    snd_pcm_hw_params_free(params);
    snd_pcm_close(handle);
    rex_audio_free_buffer(&buffer);
    pthread_mutex_lock(&rex_audio_mutex);
    rex_audio_pcm = NULL;
    rex_audio_thread_active = 0;
    rex_audio_stop_requested = 0;
    pthread_mutex_unlock(&rex_audio_mutex);
    return NULL;
  }
  snd_pcm_hw_params_free(params);
  if (snd_pcm_prepare(handle) < 0) {
    snd_pcm_close(handle);
    rex_audio_free_buffer(&buffer);
    pthread_mutex_lock(&rex_audio_mutex);
    rex_audio_pcm = NULL;
    rex_audio_thread_active = 0;
    rex_audio_stop_requested = 0;
    pthread_mutex_unlock(&rex_audio_mutex);
    return NULL;
  }

  const size_t chunk = 1024u;
  int16_t* temp = NULL;
  size_t temp_cap = 0;
  int stop = 0;

  for (;;) {
    size_t frames_left = buffer.frame_count;
    const int16_t* cursor = buffer.data;
    while (frames_left > 0) {
      pthread_mutex_lock(&rex_audio_mutex);
      stop = rex_audio_stop_requested;
      double volume = rex_audio_volume;
      pthread_mutex_unlock(&rex_audio_mutex);
      if (stop) {
        break;
      }
      size_t frames_now = frames_left > chunk ? chunk : frames_left;
      size_t samples_now = frames_now * (size_t)buffer.channels;
      int use_scale = volume < 0.999 || volume > 1.001;
      const int16_t* write_ptr = cursor;
      if (use_scale) {
        size_t need = samples_now;
        if (need > temp_cap) {
          int16_t* next = (int16_t*)realloc(temp, need * sizeof(int16_t));
          if (!next) {
            stop = 1;
            break;
          }
          temp = next;
          temp_cap = need;
        }
        double v = rex_audio_clamp_volume(volume);
        for (size_t i = 0; i < samples_now; i++) {
          int sample = (int)((double)cursor[i] * v);
          if (sample < -32768) {
            sample = -32768;
          } else if (sample > 32767) {
            sample = 32767;
          }
          temp[i] = (int16_t)sample;
        }
        write_ptr = temp;
      }
      size_t offset = 0;
      while (offset < frames_now) {
        const int16_t* chunk_ptr = write_ptr + offset * (size_t)buffer.channels;
        snd_pcm_sframes_t written = snd_pcm_writei(handle, chunk_ptr, frames_now - offset);
        if (written < 0) {
          if (snd_pcm_recover(handle, written, 1) < 0) {
            stop = 1;
            break;
          }
          continue;
        }
        if (written == 0) {
          stop = 1;
          break;
        }
        offset += (size_t)written;
      }
      if (stop) {
        break;
      }
      cursor += samples_now;
      frames_left -= frames_now;
    }
    if (stop || !loop) {
      break;
    }
    snd_pcm_prepare(handle);
  }

  free(temp);
  if (stop) {
    snd_pcm_drop(handle);
  } else {
    snd_pcm_drain(handle);
  }

  snd_pcm_close(handle);
  rex_audio_free_buffer(&buffer);

  pthread_mutex_lock(&rex_audio_mutex);
  if (rex_audio_pcm == handle) {
    rex_audio_pcm = NULL;
  }
  rex_audio_thread_active = 0;
  rex_audio_stop_requested = 0;
  pthread_mutex_unlock(&rex_audio_mutex);

  return NULL;
}

int rex_audio_platform_play_ex(const char* path, int loop) {
  if (!path || path[0] == '\0') {
    return 0;
  }
  if (!rex_audio_has_ext(path, ".wav") && !rex_audio_has_ext(path, ".mp3")) {
    return 0;
  }

  rex_audio_platform_stop();

  RexAudioJob* job = (RexAudioJob*)malloc(sizeof(RexAudioJob));
  if (!job) {
    return 0;
  }
  job->path = rex_audio_strdup(path);
  job->loop = loop ? 1 : 0;
  if (!job->path) {
    free(job);
    return 0;
  }

  pthread_mutex_lock(&rex_audio_mutex);
  rex_audio_stop_requested = 0;
  rex_audio_thread_active = 1;
  pthread_mutex_unlock(&rex_audio_mutex);

  if (pthread_create(&rex_audio_thread, NULL, rex_audio_thread_main, job) != 0) {
    free(job->path);
    free(job);
    pthread_mutex_lock(&rex_audio_mutex);
    rex_audio_thread_active = 0;
    pthread_mutex_unlock(&rex_audio_mutex);
    return 0;
  }

  return 1;
}

int rex_audio_platform_play(const char* path) {
  return rex_audio_platform_play_ex(path, 0);
}

void rex_audio_platform_stop(void) {
  pthread_mutex_lock(&rex_audio_mutex);
  if (!rex_audio_thread_active) {
    pthread_mutex_unlock(&rex_audio_mutex);
    return;
  }
  rex_audio_stop_requested = 1;
  pthread_mutex_unlock(&rex_audio_mutex);

  pthread_join(rex_audio_thread, NULL);

  pthread_mutex_lock(&rex_audio_mutex);
  rex_audio_thread_active = 0;
  rex_audio_stop_requested = 0;
  pthread_mutex_unlock(&rex_audio_mutex);
}

void rex_audio_platform_set_volume(double volume) {
  pthread_mutex_lock(&rex_audio_mutex);
  rex_audio_volume = rex_audio_clamp_volume(volume);
  pthread_mutex_unlock(&rex_audio_mutex);
}

double rex_audio_platform_get_volume(void) {
  pthread_mutex_lock(&rex_audio_mutex);
  double v = rex_audio_volume;
  pthread_mutex_unlock(&rex_audio_mutex);
  return v;
}

int rex_audio_platform_supports(const char* ext) {
  char buf[16];
  if (!rex_audio_extract_ext(ext, buf, sizeof(buf))) {
    return 0;
  }
  if (strcmp(buf, "wav") == 0) {
    return 1;
  }
  if (strcmp(buf, "mp3") == 0) {
    return 1;
  }
  return 0;
}

#else

static int rex_audio_warned_missing_backend = 0;

int rex_audio_platform_play_ex(const char* path, int loop) {
  (void)path;
  (void)loop;
  if (!rex_audio_warned_missing_backend) {
    fprintf(stderr, "Rex audio disabled: built without ALSA support. Install ALSA dev package and rebuild.\n");
    rex_audio_warned_missing_backend = 1;
  }
  return 0;
}

int rex_audio_platform_play(const char* path) {
  return rex_audio_platform_play_ex(path, 0);
}

void rex_audio_platform_stop(void) {
}

void rex_audio_platform_set_volume(double volume) {
  (void)volume;
}

double rex_audio_platform_get_volume(void) {
  return 1.0;
}

int rex_audio_platform_supports(const char* ext) {
  (void)ext;
  return 0;
}

#endif

#else

int rex_audio_platform_play_ex(const char* path, int loop) {
  (void)path;
  (void)loop;
  return 0;
}

int rex_audio_platform_play(const char* path) {
  return rex_audio_platform_play_ex(path, 0);
}

void rex_audio_platform_stop(void) {
}

void rex_audio_platform_set_volume(double volume) {
  (void)volume;
}

double rex_audio_platform_get_volume(void) {
  return 1.0;
}

int rex_audio_platform_supports(const char* ext) {
  (void)ext;
  return 0;
}

#endif

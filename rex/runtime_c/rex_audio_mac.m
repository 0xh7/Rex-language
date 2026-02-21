#if defined(__APPLE__)

#import <Cocoa/Cocoa.h>

#include "rex_audio.h"

#include <ctype.h>
#include <string.h>

static NSSound* rex_audio_sound = nil;
static double rex_audio_volume = 1.0;

static double rex_audio_clamp_volume(double v) {
  if (v < 0.0) {
    return 0.0;
  }
  if (v > 1.0) {
    return 1.0;
  }
  return v;
}

static int rex_audio_extract_ext(const char* value, char* out, size_t out_len) {
  if (!value || !out || out_len == 0) {
    return 0;
  }
  const char* last_sep = NULL;
  const char* p = value;
  while (*p) {
    if (*p == '/' || *p == '\\') {
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

int rex_audio_platform_play_ex(const char* path, int loop) {
  if (!path || path[0] == '\0') {
    return 0;
  }
  @autoreleasepool {
    if (rex_audio_sound) {
      [rex_audio_sound stop];
      [rex_audio_sound release];
      rex_audio_sound = nil;
    }
    NSString* ns_path = [NSString stringWithUTF8String:path];
    if (!ns_path) {
      return 0;
    }
    rex_audio_sound = [[NSSound alloc] initWithContentsOfFile:ns_path byReference:YES];
    if (!rex_audio_sound) {
      return 0;
    }
    [rex_audio_sound setLoops:(loop != 0)];
    [rex_audio_sound setVolume:rex_audio_volume];
    [rex_audio_sound play];
    return 1;
  }
}

int rex_audio_platform_play(const char* path) {
  return rex_audio_platform_play_ex(path, 0);
}

void rex_audio_platform_stop(void) {
  @autoreleasepool {
    if (rex_audio_sound) {
      [rex_audio_sound stop];
      [rex_audio_sound release];
      rex_audio_sound = nil;
    }
  }
}

void rex_audio_platform_set_volume(double volume) {
  rex_audio_volume = rex_audio_clamp_volume(volume);
  @autoreleasepool {
    if (rex_audio_sound) {
      [rex_audio_sound setVolume:rex_audio_volume];
    }
  }
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

#endif

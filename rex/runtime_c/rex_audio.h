#ifndef REX_AUDIO_H
#define REX_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

int rex_audio_platform_play(const char* path);
int rex_audio_platform_play_ex(const char* path, int loop);
void rex_audio_platform_stop(void);
void rex_audio_platform_set_volume(double volume);
double rex_audio_platform_get_volume(void);
int rex_audio_platform_supports(const char* ext);

#ifdef __cplusplus
}
#endif

#endif

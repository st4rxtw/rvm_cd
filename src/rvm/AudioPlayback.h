#pragma once
#include <cstdint>
#include "MusicTrackInfo.h"

namespace rvm {

struct AudioPlayback {
    static constexpr int32_t MUSIC_STOPPED = 0;
    static constexpr int32_t MUSIC_PLAYING = 1;
    static constexpr int32_t MUSIC_PAUSED  = 2;
    static constexpr int32_t MUSIC_LOADING = 3;
    static constexpr int32_t MUSIC_READY   = 4;
    static constexpr int32_t NUM_CHANNELS  = 8;

    static int32_t  numGlobalSFX;
    static int32_t  numStageSFX;

    static void*    sfxSamples[256];
    static bool     sfxLoaded[256];
    static int32_t  channelSfxNum[8];
    static int32_t  nextChannelPos;
    static bool     musicEnabled;
    static int32_t  musicVolume;
    static float    musicVolumeSetting;
    static float    sfxVolumeSetting;
    static int32_t  musicStatus;
    static int32_t  currentMusicTrack;
    static MusicTrackInfo musicTracks[16];

    static void InitAudioPlayback();
    static void ReleaseAudioPlayback();
    static void ReleaseGlobalSFX();
    static void ReleaseStageSFX();
    static void SetGameVolumes(int32_t bgmVolume, int32_t sfxVolume);
    static void StopAllSFX();
    static void PauseSound();
    static void ResumeSound();
    static void SetMusicTrack(const char* fileName, int32_t trackNo,
                              uint8_t loopTrack, uint32_t loopPoint);
    static void SetMusicVolume(int32_t volume);
    static void PlayMusic(int32_t trackNo);
    static void StopMusic();
    static void LoadSfx(const char* fileName, int32_t sfxNum);
    static void PlaySfx(int32_t sfxNum, uint8_t sLoop);
    static void StopSfx(int32_t sfxNum);
    static void SetSfxAttributes(int32_t sfxNum, int32_t volume, int32_t pan);
};

}

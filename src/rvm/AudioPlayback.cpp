#include "AudioPlayback.h"
#include "FileIO.h"
#include "StageSystem.h"
#include "GlobalAppDefinitions.h"
#include <cstring>

namespace rvm {

int32_t       AudioPlayback::numGlobalSFX      = 0;
int32_t       AudioPlayback::numStageSFX       = 0;
void*         AudioPlayback::sfxSamples[256]   {};
bool          AudioPlayback::sfxLoaded[256]    {};
int32_t       AudioPlayback::channelSfxNum[8]  {};
int32_t       AudioPlayback::nextChannelPos    = 0;
bool          AudioPlayback::musicEnabled      = true;
int32_t       AudioPlayback::musicVolume       = 100;
float         AudioPlayback::musicVolumeSetting= 1.0f;
float         AudioPlayback::sfxVolumeSetting  = 1.0f;
int32_t       AudioPlayback::musicStatus       = 0;
int32_t       AudioPlayback::currentMusicTrack = 0;
MusicTrackInfo AudioPlayback::musicTracks[16]  {};

void AudioPlayback::InitAudioPlayback()
{
    FileData fData{};
    char fileName[32]{};
    for (int i = 0; i < 8; ++i)
        channelSfxNum[i] = -1;

    if (!FileIO::LoadFile("Data/Game/GameConfig.bin", fData))
        return;

    uint8_t num1 = FileIO::ReadByte();
    uint8_t num2;
    for (int i = 0; i < (int)num1; ++i)
        num2 = FileIO::ReadByte();

    uint8_t num3 = FileIO::ReadByte();
    for (int i = 0; i < (int)num3; ++i)
        num2 = FileIO::ReadByte();

    uint8_t num4 = FileIO::ReadByte();
    for (int i = 0; i < (int)num4; ++i)
        num2 = FileIO::ReadByte();

    uint8_t num5 = FileIO::ReadByte();
    for (int i = 0; i < (int)num5; ++i) {
        uint8_t n = FileIO::ReadByte();
        for (int j = 0; j < (int)n; ++j)
            num2 = FileIO::ReadByte();
    }
    for (int i = 0; i < (int)num5; ++i) {
        uint8_t n = FileIO::ReadByte();
        for (int j = 0; j < (int)n; ++j)
            num2 = FileIO::ReadByte();
    }

    uint8_t num8 = FileIO::ReadByte();
    for (int i = 0; i < (int)num8; ++i) {
        uint8_t n = FileIO::ReadByte();
        int idx;
        for (idx = 0; idx < (int)n; ++idx)
            fileName[idx] = (char)FileIO::ReadByte();
        fileName[idx] = '\0';
        num2 = FileIO::ReadByte();
        num2 = FileIO::ReadByte();
        num2 = FileIO::ReadByte();
        num2 = FileIO::ReadByte();
    }

    uint8_t num11 = FileIO::ReadByte();
    numGlobalSFX = (int)num11;
    for (int sfxNum = 0; sfxNum < (int)num11; ++sfxNum) {
        uint8_t n = FileIO::ReadByte();
        int idx;
        for (idx = 0; idx < (int)n; ++idx)
            fileName[idx] = (char)FileIO::ReadByte();
        fileName[idx] = '\0';
        FileIO::GetFileInfo(fData);
        FileIO::CloseFile();
        LoadSfx(fileName, sfxNum);
        FileIO::SetFileInfo(fData);
    }
}

void AudioPlayback::ReleaseAudioPlayback() {}
void AudioPlayback::ReleaseGlobalSFX()     {}
void AudioPlayback::ReleaseStageSFX()      {}

void AudioPlayback::SetGameVolumes(int32_t bgmVolume, int32_t sfxVolume)
{
    musicVolumeSetting = (float)bgmVolume * 0.01f;
    SetMusicVolume(musicVolume);
    sfxVolumeSetting = (float)sfxVolume * 0.01f;
}

void AudioPlayback::StopAllSFX() {}

void AudioPlayback::PauseSound()
{

    musicStatus = MUSIC_PAUSED;
}

void AudioPlayback::ResumeSound()
{

    musicStatus = MUSIC_PLAYING;
}

void AudioPlayback::SetMusicTrack(const char* fileName, int32_t trackNo,
                                   uint8_t loopTrack, uint32_t loopPoint)
{

    char tmp[64]{};
    int n = (int)FileIO::StringLength(fileName);
    for (int i = 0; i < n; ++i) {
        tmp[i] = fileName[i];
        if (tmp[i] == '/') tmp[i] = '-';
    }
    int idx = n - 4;
    if (idx < 0) idx = 0;
    if (n > 0) tmp[idx] = '\0';

    FileIO::StrClear(musicTracks[trackNo].trackName, 64);
    FileIO::StrCopy(musicTracks[trackNo].trackName, 64, "Music/", 64);
    FileIO::StrAdd(musicTracks[trackNo].trackName, 64, tmp, 64);
    musicTracks[trackNo].loop      = loopTrack == 1;
    musicTracks[trackNo].loopPoint = loopPoint;
}

void AudioPlayback::SetMusicVolume(int32_t volume)
{
    if (volume < 0)   volume = 0;
    if (volume > 100) volume = 100;
    musicVolume = volume;

}

void AudioPlayback::PlayMusic(int32_t trackNo)
{
    if (musicTracks[trackNo].trackName[0] == '\0') return;
    currentMusicTrack = trackNo;
    musicVolume       = 100;
    musicStatus       = MUSIC_PLAYING;

}

void AudioPlayback::StopMusic()
{
    musicStatus = MUSIC_STOPPED;

}

void AudioPlayback::LoadSfx(const char* fileName, int32_t sfxNum)
{
    if (sfxNum < 0 || sfxNum >= 256) return;
    FileData fData{};
    char strA[64]{};
    FileIO::StrCopy(strA, 64, "Data/SoundFX/", 64);
    FileIO::StrAdd(strA, 64, fileName, 64);
    if (!FileIO::LoadFile(strA, fData))
        return;

    for (int i = 0; i < 12; ++i) FileIO::ReadByte();

    uint32_t state = 1;
    for (uint32_t i = 0; state > 0 && i < 400; ++i) {
        switch (state) {
            case 1: state = (FileIO::ReadByte() == 100) ? 2 : 1; break;
            case 2: state = (FileIO::ReadByte() ==  97) ? 3 : 1; break;
            case 3: state = (FileIO::ReadByte() == 116) ? 4 : 1; break;
            case 4: state = (FileIO::ReadByte() ==  97) ? 0 : 1; break;
        }
    }

    sfxLoaded[sfxNum] = true;
    sfxSamples[sfxNum] = nullptr;
    FileIO::CloseFile();
}

void AudioPlayback::PlaySfx(int32_t sfxNum, uint8_t /*sLoop*/)
{

    for (int i = 0; i < 8; ++i) {
        if (channelSfxNum[i] == sfxNum) {
            nextChannelPos = i;
            i = 8;
        }
    }
    channelSfxNum[nextChannelPos] = sfxNum;
    if (++nextChannelPos == 8) nextChannelPos = 0;
}

void AudioPlayback::StopSfx(int32_t sfxNum)
{
    for (int i = 0; i < 8; ++i)
        if (channelSfxNum[i] == sfxNum)
            channelSfxNum[i] = -1;
}

void AudioPlayback::SetSfxAttributes(int32_t sfxNum, int32_t /*volume*/, int32_t /*pan*/)
{

    PlaySfx(sfxNum, 0);
}

}

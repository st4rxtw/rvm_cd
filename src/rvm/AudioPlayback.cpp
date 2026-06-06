#include "AudioPlayback.h"
#include "FileIO.h"
#include "Log.h"

#include <AL/al.h>
#include <AL/alc.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace rvm {

int32_t        AudioPlayback::numGlobalSFX       = 0;
int32_t        AudioPlayback::numStageSFX        = 0;
void*          AudioPlayback::sfxSamples[256]    {};
bool           AudioPlayback::sfxLoaded[256]     {};
int32_t        AudioPlayback::channelSfxNum[8]   {};
int32_t        AudioPlayback::nextChannelPos     = 0;
bool           AudioPlayback::musicEnabled       = true;
int32_t        AudioPlayback::musicVolume        = 100;
float          AudioPlayback::musicVolumeSetting = 1.0f;
float          AudioPlayback::sfxVolumeSetting   = 1.0f;
int32_t        AudioPlayback::musicStatus        = 0;
int32_t        AudioPlayback::currentMusicTrack  = 0;
MusicTrackInfo AudioPlayback::musicTracks[16]    {};

static ALCdevice*  s_alDevice  = nullptr;
static ALCcontext* s_alContext = nullptr;
static bool        s_alReady   = false;

static ALuint s_sfxBuf[256]       {};
static bool   s_sfxBufAlloced[256]{};
static ALuint s_sfxSrc[8]         {};

static ALuint      s_musSrc         = 0;
static const int   kMusBufs         = 4;
static const int   kMusBufFrames    = 4096;
static ALuint      s_musBuf[kMusBufs]{};

static std::thread              s_musThread;
static std::mutex               s_musMtx;
static std::condition_variable  s_musCv;

enum class MusCmd { None, Play, Stop, Pause, Resume, Quit };
static std::atomic<MusCmd> s_musCmd      { MusCmd::None };
static int                 s_musPending  = -1;

struct MusicDecoder {
    AVFormatContext* fmt       = nullptr;
    AVCodecContext*  codec     = nullptr;
    SwrContext*      swr       = nullptr;
    AVPacket*        pkt       = nullptr;
    AVFrame*         frm       = nullptr;
    int              streamIdx = -1;
#ifdef __SWITCH__
    FILE*            file      = nullptr;
    AVIOContext*     avio      = nullptr;

    static int readCb(void* opaque, uint8_t* buf, int sz) {
        int n = (int)fread(buf, 1, (size_t)sz, (FILE*)opaque);
        return n == 0 ? AVERROR_EOF : n;
    }
    static int64_t seekCb(void* opaque, int64_t off, int whence) {
        FILE* f = (FILE*)opaque;
        if (whence == AVSEEK_SIZE) {
            long cur = ftell(f); fseek(f, 0, SEEK_END);
            long sz = ftell(f); fseek(f, cur, SEEK_SET); return sz;
        }
        fseek(f, (long)off, whence); return ftell(f);
    }
#endif

    bool open(const char* path)
    {
#ifdef __SWITCH__
        file = fopen(path, "rb");
        if (!file) { Log::Error("FFmpeg: cannot open %s", path); return false; }
        uint8_t* iobuf = (uint8_t*)av_malloc(4096);
        avio = avio_alloc_context(iobuf, 4096, 0, file, readCb, nullptr, seekCb);
        fmt = avformat_alloc_context();
        fmt->pb = avio;
        fmt->flags |= AVFMT_FLAG_CUSTOM_IO;
        if (avformat_open_input(&fmt, nullptr, nullptr, nullptr) < 0) {
            Log::Error("FFmpeg: avformat_open_input failed for %s", path); close(); return false;
        }
#else
        if (avformat_open_input(&fmt, path, nullptr, nullptr) < 0) {
            Log::Error("FFmpeg: cannot open %s", path); return false;
        }
#endif
        if (avformat_find_stream_info(fmt, nullptr) < 0) {
            Log::Error("FFmpeg: no stream info"); close(); return false;
        }
        for (unsigned i = 0; i < fmt->nb_streams; ++i)
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                { streamIdx = (int)i; break; }
        if (streamIdx < 0) { Log::Error("FFmpeg: no audio stream"); close(); return false; }

        AVCodecParameters* par = fmt->streams[streamIdx]->codecpar;
        const AVCodec* dec = avcodec_find_decoder(par->codec_id);
        if (!dec) { Log::Error("FFmpeg: no decoder for codec %d", par->codec_id); close(); return false; }
        codec = avcodec_alloc_context3(dec);
        avcodec_parameters_to_context(codec, par);
        if (avcodec_open2(codec, dec, nullptr) < 0) { Log::Error("FFmpeg: avcodec_open2"); close(); return false; }

        swr = swr_alloc();
        AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
        av_opt_set_chlayout (swr, "in_chlayout",    &codec->ch_layout,  0);
        av_opt_set_int      (swr, "in_sample_rate",  codec->sample_rate, 0);
        av_opt_set_sample_fmt(swr, "in_sample_fmt",  codec->sample_fmt,  0);
        av_opt_set_chlayout (swr, "out_chlayout",   &stereo,             0);
        av_opt_set_int      (swr, "out_sample_rate", 44100,              0);
        av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
        if (swr_init(swr) < 0) { Log::Error("FFmpeg: swr_init"); close(); return false; }

        pkt = av_packet_alloc();
        frm = av_frame_alloc();
        return true;
    }

    void seekSamples(int64_t sample)
    {
        int64_t pts = av_rescale_q(sample, {1, 44100},
                                   fmt->streams[streamIdx]->time_base);
        avformat_seek_file(fmt, streamIdx, INT64_MIN, pts, pts, 0);
        avcodec_flush_buffers(codec);
    }

    int decode(int16_t* out, int maxFrames)
    {
        int written = 0;
        while (written < maxFrames) {
            int ret = avcodec_receive_frame(codec, frm);
            if (ret == AVERROR(EAGAIN)) {
                if (av_read_frame(fmt, pkt) < 0) return written;
                if (pkt->stream_index == streamIdx)
                    avcodec_send_packet(codec, pkt);
                av_packet_unref(pkt);
                continue;
            }
            if (ret < 0) return written;
            uint8_t* dst = (uint8_t*)(out + written * 2);
            int got = swr_convert(swr, &dst, maxFrames - written,
                                  (const uint8_t**)frm->data, frm->nb_samples);
            if (got > 0) written += got;
        }
        return written;
    }

    void close()
    {
        if (pkt)   { av_packet_free(&pkt);          pkt   = nullptr; }
        if (frm)   { av_frame_free(&frm);           frm   = nullptr; }
        if (swr)   { swr_free(&swr);                swr   = nullptr; }
        if (codec) { avcodec_free_context(&codec);  codec = nullptr; }
        if (fmt)   { avformat_close_input(&fmt);    fmt   = nullptr; }
#ifdef __SWITCH__
        if (avio)  { av_free(avio->buffer); avio->buffer = nullptr; avio_context_free(&avio); avio = nullptr; }
        if (file)  { fclose(file); file = nullptr; }
#endif
        streamIdx = -1;
    }
};

static void wakeMusThread(MusCmd cmd, int trackNo = -1)
{
    if (trackNo >= 0) s_musPending = trackNo;
    s_musCmd.store(cmd);
    s_musCv.notify_one();
}

static void musicThreadProc()
{
    MusicDecoder dec;
    bool isOpen = false;
    bool paused = false;
    int16_t buf[kMusBufFrames * 2];

    for (;;) {
        {
            std::unique_lock<std::mutex> lk(s_musMtx);
            s_musCv.wait_for(lk, std::chrono::milliseconds(10), []{
                return s_musCmd.load() != MusCmd::None;
            });
        }

        MusCmd cmd = s_musCmd.exchange(MusCmd::None);

        if (cmd == MusCmd::Quit) { if (isOpen) dec.close(); return; }

        if (cmd == MusCmd::Stop) {
            alSourceStop(s_musSrc);
            ALint q = 0; alGetSourcei(s_musSrc, AL_BUFFERS_QUEUED, &q);
            while (q-- > 0) { ALuint b; alSourceUnqueueBuffers(s_musSrc, 1, &b); }
            if (isOpen) { dec.close(); isOpen = false; }
            paused = false;
        }
        else if (cmd == MusCmd::Pause) { alSourcePause(s_musSrc); paused = true; }
        else if (cmd == MusCmd::Resume) { alSourcePlay(s_musSrc); paused = false; }
        else if (cmd == MusCmd::Play) {
            alSourceStop(s_musSrc);
            ALint q = 0; alGetSourcei(s_musSrc, AL_BUFFERS_QUEUED, &q);
            while (q-- > 0) { ALuint b; alSourceUnqueueBuffers(s_musSrc, 1, &b); }
            if (isOpen) { dec.close(); isOpen = false; }
            paused = false;

            int tn = s_musPending;
            if (tn < 0 || tn >= 16) continue;

            char path[512];
            std::snprintf(path, sizeof(path), "%s%s.wma",
                FileIO::basePath, AudioPlayback::musicTracks[tn].trackName);

            if (!dec.open(path)) continue;
            isOpen = true;

            for (int i = 0; i < kMusBufs; ++i) {
                int got = dec.decode(buf, kMusBufFrames);
                if (got > 0) {
                    alBufferData(s_musBuf[i], AL_FORMAT_STEREO16, buf,
                                 (ALsizei)(got * 4), 44100);
                    alSourceQueueBuffers(s_musSrc, 1, &s_musBuf[i]);
                }
            }
            alSourcef(s_musSrc, AL_GAIN,
                (float)AudioPlayback::musicVolume * 0.01f * AudioPlayback::musicVolumeSetting);
            alSourcePlay(s_musSrc);
        }

        if (isOpen && !paused) {
            ALint processed = 0;
            alGetSourcei(s_musSrc, AL_BUFFERS_PROCESSED, &processed);
            while (processed-- > 0) {
                ALuint b; alSourceUnqueueBuffers(s_musSrc, 1, &b);
                int got = dec.decode(buf, kMusBufFrames);
                if (got <= 0) {
                    MusicTrackInfo& t = AudioPlayback::musicTracks[AudioPlayback::currentMusicTrack];
                    if (t.loop) {
                        dec.seekSamples((int64_t)t.loopPoint);
                        got = dec.decode(buf, kMusBufFrames);
                    }
                }
                if (got > 0) {
                    alBufferData(b, AL_FORMAT_STEREO16, buf, (ALsizei)(got * 4), 44100);
                    alSourceQueueBuffers(s_musSrc, 1, &b);
                } else {
                    alSourceStop(s_musSrc);
                    dec.close(); isOpen = false;
                    AudioPlayback::musicStatus = AudioPlayback::MUSIC_STOPPED;
                }
            }
            ALint state = 0; alGetSourcei(s_musSrc, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED && isOpen) alSourcePlay(s_musSrc);
        }
    }
}

void AudioPlayback::InitAudioPlayback()
{
    for (int i = 0; i < 8; ++i) channelSfxNum[i] = -1;

    s_alDevice = alcOpenDevice(nullptr);
    if (!s_alDevice) { Log::Error("AL: no device"); return; }
    s_alContext = alcCreateContext(s_alDevice, nullptr);
    if (!alcMakeContextCurrent(s_alContext)) { Log::Error("AL: context failed"); return; }
    s_alReady = true;

    alGenSources(8, s_sfxSrc);
    alGenSources(1, &s_musSrc);
    alGenBuffers(kMusBufs, s_musBuf);
    for (int i = 0; i < 8; ++i) {
        alSourcef(s_sfxSrc[i], AL_GAIN, sfxVolumeSetting);
        alSourcei(s_sfxSrc[i], AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(s_sfxSrc[i], AL_POSITION, 0, 0, 0);
    }
    alSourcei(s_musSrc, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(s_musSrc, AL_POSITION, 0, 0, 0);

    {
        FileData fData{};
        char fileName[64]{};
        uint8_t tmp;
        if (FileIO::LoadFile("Data/Game/GameConfig.bin", fData)) {
            uint8_t n;
            n = FileIO::ReadByte(); for(int i=0;i<n;++i) tmp=FileIO::ReadByte();
            n = FileIO::ReadByte(); for(int i=0;i<n;++i) tmp=FileIO::ReadByte();
            n = FileIO::ReadByte(); for(int i=0;i<n;++i) tmp=FileIO::ReadByte();

            uint8_t objCount = FileIO::ReadByte();
            for(int i=0;i<objCount;++i){ uint8_t l=FileIO::ReadByte(); for(int j=0;j<l;++j) tmp=FileIO::ReadByte(); }
            for(int i=0;i<objCount;++i){ uint8_t l=FileIO::ReadByte(); for(int j=0;j<l;++j) tmp=FileIO::ReadByte(); }

            uint8_t varCount = FileIO::ReadByte();
            for(int i=0;i<varCount;++i){
                uint8_t l=FileIO::ReadByte();
                int j; for(j=0;j<l&&j<63;++j) fileName[j]=(char)FileIO::ReadByte();
                for(int k=j;k<l;++k) tmp=FileIO::ReadByte();
                fileName[j<63?j:63]='\0';
                tmp=FileIO::ReadByte(); tmp=FileIO::ReadByte();
                tmp=FileIO::ReadByte(); tmp=FileIO::ReadByte();
            }

            uint8_t sfxCount = FileIO::ReadByte();
            numGlobalSFX = sfxCount;
            for(int s=0;s<sfxCount;++s){
                uint8_t l=FileIO::ReadByte();
                int j; for(j=0;j<l&&j<63;++j) fileName[j]=(char)FileIO::ReadByte();
                for(int k=j;k<l;++k) tmp=FileIO::ReadByte();
                fileName[j<63?j:63]='\0';
                FileIO::GetFileInfo(fData);
                FileIO::CloseFile();
                LoadSfx(fileName, s);
                FileIO::SetFileInfo(fData);
            }
        }
    }

    s_musThread = std::thread(musicThreadProc);
    Log::Info("AudioPlayback ready");
}

void AudioPlayback::ReleaseAudioPlayback()
{
    wakeMusThread(MusCmd::Quit);
    if (s_musThread.joinable()) s_musThread.join();
    if (!s_alReady) return;
    alDeleteSources(8, s_sfxSrc);
    alDeleteSources(1, &s_musSrc);
    alDeleteBuffers(kMusBufs, s_musBuf);
    for (int i = 0; i < 256; ++i) {
        if (s_sfxBufAlloced[i]) { alDeleteBuffers(1, &s_sfxBuf[i]); s_sfxBufAlloced[i]=false; }
        delete[] static_cast<uint8_t*>(sfxSamples[i]); sfxSamples[i]=nullptr;
    }
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(s_alContext);
    alcCloseDevice(s_alDevice);
    s_alReady = false;
}

void AudioPlayback::ReleaseGlobalSFX()
{
    for (int i = 0; i < numGlobalSFX && i < 256; ++i) {
        if (s_sfxBufAlloced[i]) { alDeleteBuffers(1, &s_sfxBuf[i]); s_sfxBufAlloced[i]=false; }
        delete[] static_cast<uint8_t*>(sfxSamples[i]); sfxSamples[i]=nullptr; sfxLoaded[i]=false;
    }
}

void AudioPlayback::ReleaseStageSFX()
{
    for (int i = numGlobalSFX; i < numGlobalSFX + numStageSFX && i < 256; ++i) {
        if (s_sfxBufAlloced[i]) { alDeleteBuffers(1, &s_sfxBuf[i]); s_sfxBufAlloced[i]=false; }
        delete[] static_cast<uint8_t*>(sfxSamples[i]); sfxSamples[i]=nullptr; sfxLoaded[i]=false;
    }
    numStageSFX = 0;
}

void AudioPlayback::SetGameVolumes(int32_t bgmVolume, int32_t sfxVolume)
{
    musicVolumeSetting = (float)bgmVolume * 0.01f;
    sfxVolumeSetting   = (float)sfxVolume * 0.01f;
    SetMusicVolume(musicVolume);
    if (s_alReady)
        for (int i = 0; i < 8; ++i)
            alSourcef(s_sfxSrc[i], AL_GAIN, sfxVolumeSetting);
}

void AudioPlayback::StopAllSFX()
{
    if (!s_alReady) return;
    for (int i = 0; i < 8; ++i) { alSourceStop(s_sfxSrc[i]); channelSfxNum[i]=-1; }
}

void AudioPlayback::PauseSound()
{
    musicStatus = MUSIC_PAUSED;
    wakeMusThread(MusCmd::Pause);
    if (s_alReady)
        for (int i = 0; i < 8; ++i) alSourcePause(s_sfxSrc[i]);
}

void AudioPlayback::ResumeSound()
{
    if (musicStatus == MUSIC_PAUSED) {
        musicStatus = MUSIC_PLAYING;
        wakeMusThread(MusCmd::Resume);
    }
    if (s_alReady)
        for (int i = 0; i < 8; ++i) {
            ALint st; alGetSourcei(s_sfxSrc[i], AL_SOURCE_STATE, &st);
            if (st == AL_PAUSED) alSourcePlay(s_sfxSrc[i]);
        }
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
    if (s_alReady)
        alSourcef(s_musSrc, AL_GAIN,
            (float)musicVolume * 0.01f * musicVolumeSetting);
}

void AudioPlayback::PlayMusic(int32_t trackNo)
{
    if (trackNo < 0 || trackNo >= 16) return;
    if (musicTracks[trackNo].trackName[0] == '\0') return;
    if (!s_alReady) return;
    currentMusicTrack = trackNo;
    musicVolume       = 100;
    musicStatus       = MUSIC_PLAYING;
    wakeMusThread(MusCmd::Play, trackNo);
}

void AudioPlayback::StopMusic()
{
    musicStatus = MUSIC_STOPPED;
    wakeMusThread(MusCmd::Stop);
}

void AudioPlayback::LoadSfx(const char* fileName, int32_t sfxNum)
{
    if (sfxNum < 0 || sfxNum >= 256) return;

    FileData fData{};
    char path[64]{};
    FileIO::StrCopy(path, 64, "Data/SoundFX/", 64);
    FileIO::StrAdd(path, 64, fileName, 64);
    if (!FileIO::LoadFile(path, fData)) return;

    for (int i = 0; i < 12; ++i) FileIO::ReadByte();

    uint16_t channels = 1, bitsPerSample = 16;
    uint32_t sampleRate = 22050, dataSize = 0;
    bool     gotFmt = false;

    for (int pass = 0; pass < 32; ++pass) {
        char id[4];
        for (int i = 0; i < 4; ++i) id[i] = (char)FileIO::ReadByte();
        uint32_t sz = (uint32_t)FileIO::ReadByte()        |
                      ((uint32_t)FileIO::ReadByte() <<  8) |
                      ((uint32_t)FileIO::ReadByte() << 16) |
                      ((uint32_t)FileIO::ReadByte() << 24);

        if (id[0]=='f' && id[1]=='m' && id[2]=='t') {
            FileIO::ReadByte(); FileIO::ReadByte();
            channels      = (uint16_t)FileIO::ReadByte() | ((uint16_t)FileIO::ReadByte() << 8);
            sampleRate    = (uint32_t)FileIO::ReadByte() | ((uint32_t)FileIO::ReadByte() << 8)
                          | ((uint32_t)FileIO::ReadByte() << 16) | ((uint32_t)FileIO::ReadByte() << 24);
            for (int i = 0; i < 6; ++i) FileIO::ReadByte();
            bitsPerSample = (uint16_t)FileIO::ReadByte() | ((uint16_t)FileIO::ReadByte() << 8);
            for (uint32_t i = 16; i < sz; ++i) FileIO::ReadByte();
            gotFmt = true;
        } else if (id[0]=='d' && id[1]=='a' && id[2]=='t' && id[3]=='a') {
            dataSize = sz;
            break;
        } else {
            for (uint32_t i = 0; i < sz; ++i) FileIO::ReadByte();
        }
    }

    if (!dataSize || !gotFmt) { FileIO::CloseFile(); return; }

    auto* pcm = new uint8_t[dataSize];
    for (uint32_t i = 0; i < dataSize; ++i) pcm[i] = FileIO::ReadByte();
    FileIO::CloseFile();

    if (sfxSamples[sfxNum]) delete[] static_cast<uint8_t*>(sfxSamples[sfxNum]);
    sfxSamples[sfxNum] = pcm;
    sfxLoaded[sfxNum]  = true;

    if (s_alReady) {
        if (!s_sfxBufAlloced[sfxNum]) {
            alGenBuffers(1, &s_sfxBuf[sfxNum]);
            s_sfxBufAlloced[sfxNum] = true;
        }
        ALenum fmt = (channels == 2)
            ? (bitsPerSample == 16 ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO8)
            : (bitsPerSample == 16 ? AL_FORMAT_MONO16   : AL_FORMAT_MONO8);
        alBufferData(s_sfxBuf[sfxNum], fmt, pcm, (ALsizei)dataSize, (ALsizei)sampleRate);
    }
}

void AudioPlayback::PlaySfx(int32_t sfxNum, uint8_t sLoop)
{
    if (!s_alReady || sfxNum < 0 || sfxNum >= 256 || !sfxLoaded[sfxNum]) return;

    int ch = nextChannelPos;
    for (int i = 0; i < 8; ++i)
        if (channelSfxNum[i] == sfxNum) { ch = i; break; }

    alSourceStop(s_sfxSrc[ch]);
    alSourcei(s_sfxSrc[ch], AL_BUFFER,  (ALint)s_sfxBuf[sfxNum]);
    alSourcei(s_sfxSrc[ch], AL_LOOPING, sLoop ? AL_TRUE : AL_FALSE);
    alSourcef(s_sfxSrc[ch], AL_GAIN,    sfxVolumeSetting);
    alSourcePlay(s_sfxSrc[ch]);
    channelSfxNum[ch] = sfxNum;
    if (ch == nextChannelPos && ++nextChannelPos >= 8) nextChannelPos = 0;
}

void AudioPlayback::StopSfx(int32_t sfxNum)
{
    if (!s_alReady) return;
    for (int i = 0; i < 8; ++i)
        if (channelSfxNum[i] == sfxNum) { alSourceStop(s_sfxSrc[i]); channelSfxNum[i]=-1; }
}

void AudioPlayback::SetSfxAttributes(int32_t sfxNum, int32_t volume, int32_t /*pan*/)
{
    if (!s_alReady) return;
    for (int i = 0; i < 8; ++i)
        if (channelSfxNum[i] == sfxNum)
            alSourcef(s_sfxSrc[i], AL_GAIN, (float)volume / 100.0f * sfxVolumeSetting);
}

}

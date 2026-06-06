#include "VideoPlayer.h"
#include "Log.h"

#include <AL/al.h>
#include <AL/alc.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <chrono>

namespace rvm {

int      VideoPlayer::width       = 0;
int      VideoPlayer::height      = 0;
uint8_t* VideoPlayer::frameRGB    = nullptr;
bool     VideoPlayer::hasNewFrame = false;

static AVFormatContext* s_fmt    = nullptr;
static AVPacket*        s_pkt    = nullptr;
static AVFrame*         s_vfrm   = nullptr;
static AVFrame*         s_afrm   = nullptr;
static AVCodecContext*  s_vcodec = nullptr;
static AVCodecContext*  s_acodec = nullptr;
static SwsContext*      s_sws    = nullptr;
static SwrContext*      s_swr    = nullptr;
static int              s_vidIdx = -1;
static int              s_audIdx = -1;
static bool             s_eof      = false;
static bool             s_finished = false;

#ifdef __SWITCH__
static FILE*        s_file = nullptr;
static AVIOContext* s_avio = nullptr;

static int vp_readCb(void* opaque, uint8_t* buf, int sz) {
    int n = (int)fread(buf, 1, (size_t)sz, (FILE*)opaque);
    return n == 0 ? AVERROR_EOF : n;
}
static int64_t vp_seekCb(void* opaque, int64_t off, int whence) {
    FILE* f = (FILE*)opaque;
    if (whence == AVSEEK_SIZE) {
        long cur = ftell(f); fseek(f, 0, SEEK_END);
        long sz = ftell(f); fseek(f, cur, SEEK_SET); return sz;
    }
    fseek(f, (long)off, whence); return ftell(f);
}
#endif

static double          s_fps          = 30.0;
static int64_t         s_nextFrameNum = 0;
static std::chrono::steady_clock::time_point s_startTime;

static const int kAudBufs = 32;
static ALuint    s_audSrc  = 0;
static ALuint    s_audBufs[kAudBufs]{};
static bool      s_audBufsAlloc = false;
static int       s_audHead = 0;
static int       s_audTail = 0;

static int  audOnSource() { return s_audHead - s_audTail; }
static bool audCanQueue()  { return audOnSource() < kAudBufs; }

static void audReclaim()
{
    if (s_audSrc == 0) return;
    ALint processed = 0;
    alGetSourcei(s_audSrc, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0) {
        ALuint b; alSourceUnqueueBuffers(s_audSrc, 1, &b);
        ++s_audTail;
    }
}

static void audQueuePCM(const int16_t* pcm, int frames)
{
    if (s_audSrc == 0 || frames <= 0) return;
    audReclaim();
    if (!audCanQueue()) return;

    ALuint buf = s_audBufs[s_audHead % kAudBufs];
    alBufferData(buf, AL_FORMAT_STEREO16, pcm, (ALsizei)(frames * 4), 44100);
    alSourceQueueBuffers(s_audSrc, 1, &buf);
    ++s_audHead;

    ALint state = 0;
    alGetSourcei(s_audSrc, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) alSourcePlay(s_audSrc);
}

static void drainAudio()
{
    if (!s_acodec || !s_swr) return;
    for (;;) {
        int ret = avcodec_receive_frame(s_acodec, s_afrm);
        if (ret != 0) break;

        int outMax = (int)av_rescale_rnd(
            swr_get_delay(s_swr, s_acodec->sample_rate) + s_afrm->nb_samples,
            44100, s_acodec->sample_rate, AV_ROUND_UP);

        auto* pcm = new int16_t[(size_t)outMax * 2];
        uint8_t* dst = (uint8_t*)pcm;
        int got = swr_convert(s_swr, &dst, outMax,
                              (const uint8_t**)s_afrm->data, s_afrm->nb_samples);
        audQueuePCM(pcm, got);
        delete[] pcm;
    }
}

static bool readOnePacket()
{
    if (s_eof) return false;
    if (av_read_frame(s_fmt, s_pkt) < 0) {
        s_eof = true;
        avcodec_send_packet(s_vcodec, nullptr);
        return false;
    }
    if (s_pkt->stream_index == s_vidIdx)
        avcodec_send_packet(s_vcodec, s_pkt);
    else if (s_pkt->stream_index == s_audIdx && s_acodec) {
        avcodec_send_packet(s_acodec, s_pkt);
        drainAudio();
    }
    av_packet_unref(s_pkt);
    return true;
}

bool VideoPlayer::Open(const char* path)
{
    Close();

#ifdef __SWITCH__
    s_file = fopen(path, "rb");
    if (!s_file) { Log::Error("Video: cannot open %s", path); return false; }
    uint8_t* iobuf = (uint8_t*)av_malloc(4096);
    s_avio = avio_alloc_context(iobuf, 4096, 0, s_file, vp_readCb, nullptr, vp_seekCb);
    s_fmt = avformat_alloc_context();
    s_fmt->pb = s_avio;
    s_fmt->flags |= AVFMT_FLAG_CUSTOM_IO;
    if (avformat_open_input(&s_fmt, nullptr, nullptr, nullptr) < 0) {
        Log::Error("Video: avformat_open_input failed for %s", path); Close(); return false;
    }
#else
    if (avformat_open_input(&s_fmt, path, nullptr, nullptr) < 0) {
        Log::Error("Video: cannot open %s", path); return false;
    }
#endif
    if (avformat_find_stream_info(s_fmt, nullptr) < 0) {
        Log::Error("Video: no stream info"); Close(); return false;
    }
    for (unsigned i = 0; i < s_fmt->nb_streams; ++i) {
        auto t = s_fmt->streams[i]->codecpar->codec_type;
        if (t == AVMEDIA_TYPE_VIDEO && s_vidIdx < 0) s_vidIdx = (int)i;
        if (t == AVMEDIA_TYPE_AUDIO && s_audIdx < 0) s_audIdx = (int)i;
    }
    if (s_vidIdx < 0) { Log::Error("Video: no video stream"); Close(); return false; }

    {
        auto* par = s_fmt->streams[s_vidIdx]->codecpar;
        auto* dec = avcodec_find_decoder(par->codec_id);
        if (!dec) { Log::Error("Video: no video decoder"); Close(); return false; }
        s_vcodec = avcodec_alloc_context3(dec);
        avcodec_parameters_to_context(s_vcodec, par);
        if (avcodec_open2(s_vcodec, dec, nullptr) < 0) { Close(); return false; }
        width  = s_vcodec->width;
        height = s_vcodec->height;
        AVRational fr = s_fmt->streams[s_vidIdx]->avg_frame_rate;
        s_fps = (fr.den > 0) ? (double)fr.num / fr.den : 30.0;
        s_sws = sws_getContext(width, height, s_vcodec->pix_fmt,
                               width, height, AV_PIX_FMT_RGB24,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
        frameRGB = new uint8_t[(size_t)width * height * 3];
        s_vfrm   = av_frame_alloc();
    }

    if (s_audIdx >= 0) {
        auto* par = s_fmt->streams[s_audIdx]->codecpar;
        auto* dec = avcodec_find_decoder(par->codec_id);
        if (dec) {
            s_acodec = avcodec_alloc_context3(dec);
            avcodec_parameters_to_context(s_acodec, par);
            if (avcodec_open2(s_acodec, dec, nullptr) == 0) {
                s_swr = swr_alloc();
                AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
                av_opt_set_chlayout (s_swr, "in_chlayout",    &s_acodec->ch_layout,  0);
                av_opt_set_int      (s_swr, "in_sample_rate",  s_acodec->sample_rate, 0);
                av_opt_set_sample_fmt(s_swr, "in_sample_fmt",  s_acodec->sample_fmt,  0);
                av_opt_set_chlayout (s_swr, "out_chlayout",   &stereo,                0);
                av_opt_set_int      (s_swr, "out_sample_rate", 44100,                 0);
                av_opt_set_sample_fmt(s_swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,     0);
                if (swr_init(s_swr) < 0) {
                    swr_free(&s_swr); avcodec_free_context(&s_acodec); s_audIdx = -1;
                } else {
                    s_afrm = av_frame_alloc();
                    if (!s_audBufsAlloc) { alGenBuffers(kAudBufs, s_audBufs); s_audBufsAlloc = true; }
                    alGenSources(1, &s_audSrc);
                    alSourcei(s_audSrc, AL_SOURCE_RELATIVE, AL_TRUE);
                    alSource3f(s_audSrc, AL_POSITION, 0, 0, 0);
                    s_audHead = s_audTail = 0;
                }
            } else { avcodec_free_context(&s_acodec); s_audIdx = -1; }
        } else { s_audIdx = -1; }
    }

    s_pkt = av_packet_alloc();

    s_finished     = false;
    s_nextFrameNum = 0;
    s_startTime    = std::chrono::steady_clock::now();
    hasNewFrame    = false;

    Log::Info("Video: %s  %dx%d  %.2f fps  audio=%s",
              path, width, height, s_fps, s_audIdx >= 0 ? "yes" : "no");
    return true;
}

void VideoPlayer::DecodeFrame()
{
    if (s_finished || !s_fmt) return;

    audReclaim();

    auto   now     = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - s_startTime).count();
    if (elapsed < (double)s_nextFrameNum / s_fps) {
        hasNewFrame = false;
        return;
    }

    for (int limit = 512; limit-- > 0; ) {
        int ret = avcodec_receive_frame(s_vcodec, s_vfrm);
        if (ret == 0) {
            uint8_t* dst[1] = { frameRGB };
            int      lsz[1] = { width * 3 };
            sws_scale(s_sws, (const uint8_t* const*)s_vfrm->data, s_vfrm->linesize,
                      0, height, dst, lsz);
            ++s_nextFrameNum;
            hasNewFrame = true;
            return;
        }
        if (ret != AVERROR(EAGAIN)) { s_finished = true; return; }
        if (!readOnePacket() && s_eof) continue;
    }
    hasNewFrame = false;
}

bool VideoPlayer::IsFinished() { return s_finished; }

void VideoPlayer::Close()
{
    if (s_audSrc) {
        alSourceStop(s_audSrc);
        ALint q = 0; alGetSourcei(s_audSrc, AL_BUFFERS_QUEUED, &q);
        while (q-- > 0) { ALuint b; alSourceUnqueueBuffers(s_audSrc, 1, &b); }
        alDeleteSources(1, &s_audSrc); s_audSrc = 0;
    }
    if (s_afrm)   { av_frame_free(&s_afrm);          s_afrm   = nullptr; }
    if (s_swr)    { swr_free(&s_swr);                s_swr    = nullptr; }
    if (s_acodec) { avcodec_free_context(&s_acodec); s_acodec = nullptr; }
    if (s_vfrm)   { av_frame_free(&s_vfrm);          s_vfrm   = nullptr; }
    if (s_sws)    { sws_freeContext(s_sws);           s_sws    = nullptr; }
    if (s_vcodec) { avcodec_free_context(&s_vcodec); s_vcodec = nullptr; }
    if (s_pkt)    { av_packet_free(&s_pkt);           s_pkt    = nullptr; }
    if (s_fmt)    { avformat_close_input(&s_fmt);     s_fmt    = nullptr; }
#ifdef __SWITCH__
    if (s_avio) { av_free(s_avio->buffer); s_avio->buffer = nullptr; avio_context_free(&s_avio); s_avio = nullptr; }
    if (s_file) { fclose(s_file); s_file = nullptr; }
#endif
    delete[] frameRGB; frameRGB = nullptr;
    width = height = 0;
    s_vidIdx = s_audIdx = -1;
    s_eof = s_finished = false;
    hasNewFrame = false;
}

}

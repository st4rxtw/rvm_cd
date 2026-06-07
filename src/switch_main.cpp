#include <cstdio>
#include <cstring>

#include <switch.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <glad/glad.h>

#include <cd/Game.h>
#include <rvm/FileIO.h>
#include <rvm/Log.h>
#include <rvm/InputSystem.h>
#include <rvm/GlobalAppDefinitions.h>
#include <rvm/StageSystem.h>
#include <rvm/ObjectSystem.h>
#include <rvm/AudioPlayback.h>

static EGLDisplay s_display = EGL_NO_DISPLAY;
static EGLContext s_context = EGL_NO_CONTEXT;
static EGLSurface s_surface = EGL_NO_SURFACE;

static bool egl_init(NWindow* nwin)
{
    s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (s_display == EGL_NO_DISPLAY) return false;
    if (!eglInitialize(s_display, nullptr, nullptr)) return false;

    eglBindAPI(EGL_OPENGL_API);

    static const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE,   8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    EGLConfig cfg; EGLint n;
    if (!eglChooseConfig(s_display, attribs, &cfg, 1, &n) || n == 0) return false;

    s_surface = eglCreateWindowSurface(s_display, cfg, (EGLNativeWindowType)nwin, nullptr);
    if (s_surface == EGL_NO_SURFACE) return false;

    static const EGLint ctxAttribs[] = {
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_NONE
    };
    s_context = eglCreateContext(s_display, cfg, EGL_NO_CONTEXT, ctxAttribs);
    if (s_context == EGL_NO_CONTEXT) return false;

    eglMakeCurrent(s_display, s_surface, s_surface, s_context);
    eglSwapInterval(s_display, 1);
    return true;
}

static void egl_done()
{
    if (s_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (s_context != EGL_NO_CONTEXT) eglDestroyContext(s_display, s_context);
        if (s_surface != EGL_NO_SURFACE) eglDestroySurface(s_display, s_surface);
        eglTerminate(s_display);
    }
}

static void parse_args(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        const char* p;
        if ((p = strstr(argv[i], "dataFile=")))
            snprintf(rvm::FileIO::dataFile, sizeof(rvm::FileIO::dataFile), "%s", p + 9);
        if (strstr(argv[i], "usingCWD=true"))
            rvm::FileIO::usingCWD = true;
    }
}

static void nx_base_path(char* out, size_t cap, const char* nroPath)
{
    strncpy(out, nroPath ? nroPath : "", cap - 1);
    out[cap - 1] = '\0';
    char* slash = strrchr(out, '/');
    if (slash) slash[1] = '\0';
    else out[0] = '\0';
}

int main(int argc, char* argv[])
{
    parse_args(argc, argv);

    char base[512] = {};
    nx_base_path(base, sizeof(base), argc > 0 ? argv[0] : nullptr);
    rvm::FileIO::SetBasePath(base);

    char logPath[512];
    snprintf(logPath, sizeof(logPath), "%srvm_cd.log", base);
    rvm::Log::OpenFile(logPath);

    NWindow* nwin = nwindowGetDefault();
    nwindowSetDimensions(nwin, 1280, 720);

    if (!egl_init(nwin)) { rvm::Log::Error("EGL init failed"); return 1; }

    if (!gladLoadGLLoader((GLADloadproc)eglGetProcAddress)) {
        rvm::Log::Error("gladLoadGL failed"); egl_done(); return 1;
    }
    rvm::Log::Info("GL %s", glGetString(GL_VERSION));

    glViewport(0, 0, 1280, 720);

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    rvm::Log::Info("Init...");
    cd::Game::Init(nullptr, 1280, 720);
    rvm::Log::Info("Running.");

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 held    = padGetButtons(&pad);
        u64 pressed = padGetButtonsDown(&pad);

        auto& inp = rvm::InputSystem::touchData;

        inp.up      = (held & HidNpadButton_Up)    ? 1 : 0;
        inp.down    = (held & HidNpadButton_Down)   ? 1 : 0;
        inp.left    = (held & HidNpadButton_Left)   ? 1 : 0;
        inp.right   = (held & HidNpadButton_Right)  ? 1 : 0;
        inp.buttonA = (held & HidNpadButton_A)      ? 1 : 0;
        inp.buttonB = (held & HidNpadButton_B)      ? 1 : 0;
        inp.buttonC = (held & HidNpadButton_X)      ? 1 : 0;
        inp.start   = (held & HidNpadButton_Plus)   ? 1 : 0;

        static constexpr int STICK_DEAD = 10000;
        HidAnalogStickState lstick = padGetStickPos(&pad, 0);
        if (lstick.x < -STICK_DEAD) inp.left  = 1;
        if (lstick.x >  STICK_DEAD) inp.right = 1;
        if (lstick.y >  STICK_DEAD) inp.up    = 1;
        if (lstick.y < -STICK_DEAD) inp.down  = 1;

        if (pressed & HidNpadButton_Plus) {
            using namespace rvm;
            if (FileIO::activeStageList == 0) {
                if (StageSystem::stageListPosition == 4 ||
                    StageSystem::stageListPosition == 5)
                    inp.start   = 1;
                else
                    inp.buttonB = 1;
            } else if (StageSystem::stageMode == 2) {
                ObjectEntity& pauseObj = ObjectSystem::objectEntityList[9];
                if (pauseObj.state == 3 && GlobalAppDefinitions::gameMode == 1) {
                    pauseObj.state    = 4;
                    pauseObj.value[0] = 0;
                    pauseObj.value[1] = 0;
                    pauseObj.alpha    = 248;
                    AudioPlayback::PlaySfx(27, 0);
                }
            } else {
                GlobalAppDefinitions::gameMessage = 2;
            }
        }

        cd::Game::Update(nullptr);
        cd::Game::Draw();
        eglSwapBuffers(s_display, s_surface);
    }

    rvm::Log::Info("Shutdown.");
    egl_done();
    return 0;
}

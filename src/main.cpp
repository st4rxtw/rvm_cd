#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <glad/glad.h>
#include <cd/Game.h>
#include <rvm/FileIO.h>
#include <rvm/Log.h>

// pc
#ifndef __SWITCH__

#include <climits>
#include <unistd.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static void compute_base_path(char* out, size_t cap, const char* argv0)
{
    char buf[PATH_MAX] = {};
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (n > 0) {
        buf[n] = '\0';
        char* slash = strrchr(buf, '/');
        if (slash) { slash[1] = '\0'; snprintf(out, cap, "%s", buf); return; }
    }
    snprintf(out, cap, "%s", argv0 ? argv0 : "");
    char* slash = strrchr(out, '/');
    if (slash) slash[1] = '\0';
    else out[0] = '\0';
}

static void on_focus(GLFWwindow*, int focused)
{
    if (focused) cd::Game::OnFocusGained();
    else         cd::Game::OnFocusLost();
}

int main(int argc, char* argv[])
{
    char base[PATH_MAX] = {};
    compute_base_path(base, sizeof(base), argc > 0 ? argv[0] : nullptr);
    rvm::FileIO::SetBasePath(base);

    if (!glfwInit()) { rvm::Log::Error("glfwInit failed"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(800, 480, "Sonic CD", nullptr, nullptr);
    if (!win) { rvm::Log::Error("glfwCreateWindow failed"); glfwTerminate(); return 1; }

    {
        char iconPath[PATH_MAX];
        snprintf(iconPath, sizeof(iconPath), "%sicon.png", base);
        int w, h, ch;
        unsigned char* px = stbi_load(iconPath, &w, &h, &ch, 4);
        if (px) {
            GLFWimage img{ w, h, px };
            glfwSetWindowIcon(win, 1, &img);
            stbi_image_free(px);
        }
    }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        rvm::Log::Error("gladLoadGL failed"); glfwDestroyWindow(win); glfwTerminate(); return 1;
    }
    rvm::Log::Info("GL %s", glGetString(GL_VERSION));

    glfwSetWindowFocusCallback(win, on_focus);
    { int w,h; glfwGetFramebufferSize(win,&w,&h); glViewport(0,0,w,h); }

    rvm::Log::Info("Init...");
    cd::Game::Init(win, 800, 480);
    rvm::Log::Info("Running.");

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        cd::Game::Update(win);
        cd::Game::Draw();
        glfwSwapBuffers(win);
    }

    rvm::Log::Info("Shutdown.");
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}

// nx
#else

#include <switch.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <rvm/InputSystem.h>

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

static void nx_base_path(char* out, size_t cap, const char* nroPath)
{
    strncpy(out, nroPath ? nroPath : "", cap - 1);
    out[cap - 1] = '\0';
    char* slash = strrchr(out, '/');
    if (slash) slash[1] = '\0';
    else { out[0] = '\0'; }
}

int main(int argc, char* argv[])
{
    char base[512] = {};
    nx_base_path(base, sizeof(base), argc > 0 ? argv[0] : nullptr);
    rvm::FileIO::SetBasePath(base);

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
        u64 down = padGetButtons(&pad);

        auto& inp = rvm::InputSystem::touchData;
        inp.up      = (down & HidNpadButton_Up)    ? 1 : 0;
        inp.down    = (down & HidNpadButton_Down)   ? 1 : 0;
        inp.left    = (down & HidNpadButton_Left)   ? 1 : 0;
        inp.right   = (down & HidNpadButton_Right)  ? 1 : 0;
        inp.buttonA = (down & HidNpadButton_A)      ? 1 : 0;
        inp.buttonB = (down & HidNpadButton_B)      ? 1 : 0;
        inp.buttonC = (down & HidNpadButton_X)      ? 1 : 0;
        inp.start   = (down & HidNpadButton_Plus)   ? 1 : 0;

        cd::Game::Update(nullptr);
        cd::Game::Draw();
        eglSwapBuffers(s_display, s_surface);
    }

    rvm::Log::Info("Shutdown.");
    egl_done();
    return 0;
}

#endif

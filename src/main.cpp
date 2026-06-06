#include <climits>
#include <unistd.h>
#include <cstdio>
#include <cstring>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cd/Game.h>
#include <rvm/FileIO.h>
#include <rvm/Log.h>

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

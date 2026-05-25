#include "RenderDevice.h"
#include "GraphicsSystem.h"
#include "InputSystem.h"
#include "GlobalAppDefinitions.h"
#include "Log.h"

#include <glad/glad.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstddef>

namespace rvm {

void*    RenderDevice::gfxTexture[6] {};
int32_t  RenderDevice::orthWidth     = 0;
int32_t  RenderDevice::viewWidth     = 0;
int32_t  RenderDevice::viewHeight    = 0;
float    RenderDevice::viewAspect    = 0.75f;
int32_t  RenderDevice::bufferWidth   = 0;
int32_t  RenderDevice::bufferHeight  = 0;
int32_t  RenderDevice::highResMode   = 0;
bool     RenderDevice::useFBTexture  = true;

static GLuint s_atlasTexture  = 0;
static GLuint s_vbo2D         = 0;
static GLuint s_vao2D         = 0;
static GLuint s_ibo           = 0;
static GLuint s_vbo3D         = 0;
static GLuint s_vao3D         = 0;
static GLuint s_prog2D        = 0;
static GLuint s_prog3D        = 0;
static bool   s_glReady       = false;

static const char* kVS2D = R"GLSL(
#version 330 core
layout(location=0) in vec2  aPos;
layout(location=1) in vec2  aUV;
layout(location=2) in vec4  aColor;
out vec2 vUV;
out vec4 vColor;
uniform vec2 uInvHalfScreen;
void main(){
    vec2 ndc = aPos * uInvHalfScreen - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
}
)GLSL";

static const char* kFS2D = R"GLSL(
#version 330 core
in vec2 vUV;
in vec4 vColor;
out vec4 fragColor;
uniform sampler2D uTex;
void main(){
    vec4 t = texture(uTex, vUV);
    if(t.a < 0.01) discard;
    fragColor = t * vColor;
}
)GLSL";

static const char* kVS3D = R"GLSL(
#version 330 core
layout(location=0) in vec3  aPos;
layout(location=1) in vec2  aUV;
layout(location=2) in vec4  aColor;
out vec2 vUV;
out vec4 vColor;
uniform mat4 uMVP;
void main(){
    gl_Position = uMVP * vec4(aPos, 1.0);
    vUV = aUV;
    vColor = aColor;
}
)GLSL";

static const char* kFS3D = R"GLSL(
#version 330 core
in vec2 vUV;
in vec4 vColor;
out vec4 fragColor;
uniform sampler2D uTex;
void main(){
    vec4 t = texture(uTex, vUV);
    if(t.a < 0.01) discard;
    fragColor = t * vColor;
}
)GLSL";

static GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ char buf[512]; glGetShaderInfoLog(s,512,nullptr,buf); Log::Error("Shader: %s",buf); }
    return s;
}

static GLuint linkProg(const char* vs, const char* fs)
{
    GLuint v=compileShader(GL_VERTEX_SHADER,vs);
    GLuint f=compileShader(GL_FRAGMENT_SHADER,fs);
    GLuint p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f);
    glLinkProgram(p);
    GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ char buf[512]; glGetProgramInfoLog(p,512,nullptr,buf); Log::Error("Link: %s",buf); }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

void RenderDevice::InitRenderDevice()
{
    GraphicsSystem::SetupPolygonLists();

    glGenTextures(1, &s_atlasTexture);
    glBindTexture(GL_TEXTURE_2D, s_atlasTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0,
                 GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, nullptr);
    glGenerateMipmap(GL_TEXTURE_2D);

    s_prog2D = linkProg(kVS2D, kFS2D);
    s_prog3D = linkProg(kVS3D, kFS3D);

    glGenVertexArrays(1, &s_vao2D);
    glGenBuffers(1, &s_vbo2D);
    glGenBuffers(1, &s_ibo);

    glBindVertexArray(s_vao2D);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo2D);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GraphicsSystem::gfxPolyList), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(DrawVertex),(void*)offsetof(DrawVertex,x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(DrawVertex),(void*)offsetof(DrawVertex,u));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(DrawVertex),(void*)offsetof(DrawVertex,color));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GraphicsSystem::gfxPolyListIndex),
                 GraphicsSystem::gfxPolyListIndex, GL_STATIC_DRAW);
    glBindVertexArray(0);

    glGenVertexArrays(1, &s_vao3D);
    glGenBuffers(1, &s_vbo3D);
    glBindVertexArray(s_vao3D);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo3D);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GraphicsSystem::polyList3D), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(DrawVertex3D),(void*)offsetof(DrawVertex3D,x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(DrawVertex3D),(void*)offsetof(DrawVertex3D,u));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(DrawVertex3D),(void*)offsetof(DrawVertex3D,color));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);
    glBindVertexArray(0);

    s_glReady = true;
    Log::Info("RenderDevice GL ready");
}

void RenderDevice::UpdateHardwareTextures()
{
    GraphicsSystem::SetActivePalette(0, 0, 240);
    GraphicsSystem::UpdateTextureBufferWithTiles();
    GraphicsSystem::UpdateTextureBufferWithSortedSprites();

    if(s_glReady){
        glBindTexture(GL_TEXTURE_2D, s_atlasTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 1024,
                        GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,
                        GraphicsSystem::texBuffer);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    for (uint8_t p = 1; p < 6; ++p) {
        GraphicsSystem::SetActivePalette(p, 0, 240);
        GraphicsSystem::UpdateTextureBufferWithTiles();
        GraphicsSystem::UpdateTextureBufferWithSprites();
    }
    GraphicsSystem::SetActivePalette(0, 0, 240);
}

void RenderDevice::SetScreenDimensions(int32_t width, int32_t height)
{
    InputSystem::touchWidth  = width;
    InputSystem::touchHeight = height;
    viewWidth  = width;
    viewHeight = height;
    bufferWidth = (int32_t)((float)viewWidth / (float)viewHeight * 240.0f);
    bufferWidth += 8;
    bufferWidth = (bufferWidth >> 4) << 4;
    if (bufferWidth > 400) bufferWidth = 400;
    viewAspect = 0.75f;
    GlobalAppDefinitions::HQ3DFloorEnabled = (viewHeight >= 480);
    if (viewHeight >= 480) {
        GraphicsSystem::SetScreenRenderSize(bufferWidth, bufferWidth);
        bufferWidth  *= 2;
        bufferHeight  = 480;
    } else {
        bufferHeight = 240;
        GraphicsSystem::SetScreenRenderSize(bufferWidth, bufferWidth);
    }
    orthWidth = GlobalAppDefinitions::SCREEN_XSIZE * 16;
}

static int s_flipCount = 0;
static void doFlip()
{
    if(!s_glReady) return;

    if(++s_flipCount <= 5 || s_flipCount % 60 == 0)
        Log::Info("flip#%d verts=%u idx=%u opaqueIdx=%u 3D:vs=%u is=%u pos=(%.2f,%.2f,%.2f) ang=%.1f",
            s_flipCount,
            (unsigned)GraphicsSystem::gfxVertexSize,
            (unsigned)GraphicsSystem::gfxIndexSize,
            (unsigned)GraphicsSystem::gfxIndexSizeOpaque,
            (unsigned)GraphicsSystem::vertexSize3D,
            (unsigned)GraphicsSystem::indexSize3D,
            GraphicsSystem::floor3DPosX,
            GraphicsSystem::floor3DPosY,
            GraphicsSystem::floor3DPosZ,
            GraphicsSystem::floor3DAngle);

    int32_t sw = GlobalAppDefinitions::SCREEN_XSIZE * 16;
    int32_t sh = 240 * 16;

    glViewport(0, 0, RenderDevice::viewWidth, RenderDevice::viewHeight);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(s_prog2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_atlasTexture);
    glUniform1i(glGetUniformLocation(s_prog2D,"uTex"),0);
    glUniform2f(glGetUniformLocation(s_prog2D,"uInvHalfScreen"),
                2.0f/(float)sw, 2.0f/(float)sh);

    glBindVertexArray(s_vao2D);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo2D);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        (int32_t)GraphicsSystem::gfxVertexSize*sizeof(DrawVertex),
        GraphicsSystem::gfxPolyList);

    int32_t opaqueIdx = (int32_t)GraphicsSystem::gfxIndexSizeOpaque * 3;
    int32_t totalIdx  = (int32_t)GraphicsSystem::gfxIndexSize * 3;

    glDisable(GL_BLEND);
    if(opaqueIdx > 0)
        glDrawElements(GL_TRIANGLES, opaqueIdx, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    if(GraphicsSystem::render3DEnabled){
        glUseProgram(s_prog3D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_atlasTexture);
        glUniform1i(glGetUniformLocation(s_prog3D,"uTex"),0);

        float ang3 = (GraphicsSystem::floor3DAngle + 180.0f) * (3.14159265f/180.0f);
        float c3=cosf(ang3), s3=sinf(ang3);
        float tx=GraphicsSystem::floor3DPosX;
        float ty=GraphicsSystem::floor3DPosY;
        float tz=GraphicsSystem::floor3DPosZ;

        float model[16]={
            1.35f*c3,  0, -s3, 0,
            0,       -0.9f,  0, 0,
            1.35f*s3,  0,  c3, 0,
            1.35f*(c3*tx+s3*tz), -0.9f*ty, -s3*tx+c3*tz, 1
        };

        float fov3=1.8326f, ar3=0.75f, near3=0.1f, far3=2000.0f;
        float w3 = 1.0f/tanf(fov3*0.5f);
        float h3 = 1.0f/(w3*ar3);
        float qz  = -(far3+near3)/(far3-near3);
        float qw  = -(2.0f*far3*near3)/(far3-near3);
        float proj[16]={
            w3,       0,   0,  0,
            0,  h3*0.5f,   0,  0,
            0,        0,  qz, -1,
            0,        0,  qw,  0
        };

        float mvp[16];
        for(int r=0;r<4;r++) for(int cc=0;cc<4;cc++){
            mvp[r+cc*4]=0;
            for(int k=0;k<4;k++) mvp[r+cc*4]+=proj[r+k*4]*model[k+cc*4];
        }
        glUniformMatrix4fv(glGetUniformLocation(s_prog3D,"uMVP"),1,GL_FALSE,mvp);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

        glBindVertexArray(s_vao3D);
        glBindBuffer(GL_ARRAY_BUFFER, s_vbo3D);
        glBufferSubData(GL_ARRAY_BUFFER,0,
            (int32_t)GraphicsSystem::vertexSize3D*sizeof(DrawVertex3D),
            GraphicsSystem::polyList3D);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawElements(GL_TRIANGLES,
            (int32_t)GraphicsSystem::indexSize3D*3,
            GL_UNSIGNED_SHORT, 0);
        glDisable(GL_BLEND);
        glBindVertexArray(0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    glUseProgram(s_prog2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_atlasTexture);
    glUniform1i(glGetUniformLocation(s_prog2D,"uTex"),0);
    glUniform2f(glGetUniformLocation(s_prog2D,"uInvHalfScreen"),
                2.0f/(float)sw, 2.0f/(float)sh);

    glBindVertexArray(s_vao2D);
    if(totalIdx > opaqueIdx){
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawElements(GL_TRIANGLES, totalIdx - opaqueIdx,
                       GL_UNSIGNED_SHORT,
                       (void*)(uintptr_t)(opaqueIdx * sizeof(int16_t)));
        glDisable(GL_BLEND);
    }
    glBindVertexArray(0);
}

void RenderDevice::FlipScreen()     { doFlip(); }
void RenderDevice::FlipScreenHRes() { doFlip(); }

}

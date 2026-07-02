#include "../NativeScript.h"

namespace cd {

void Clouds_Startup()
{
    NS_LoadSpriteSheet("Title/Clouds.gif");

    Scene3D::numVertices = 0;
    Scene3D::numFaces    = 0;

    Scene3D::MatrixTranslateXYZ(Scene3D::matWorld, 0, 0, 0);
    Scene3D::MatrixRotateXYZ(Scene3D::matWorld, 0, 0, 0);
    Scene3D::MatrixRotateXYZ(Scene3D::matView,  8, 0, 0);

    int32_t ap0  = 0;
    int32_t ap1  = 0;
    int32_t tv2  = 0;
    int32_t tv3  = 0;
    int32_t tv4  = 0;
    int32_t tv5  = 0;

    while (tv3 < 10) {
        int32_t tv0  = 0;
        int32_t tv1  = -0x2000;
        tv4 = 0;

        while (tv0 < 16) {

            Scene3D::vertexBuffer[ap0].x = tv1;
            Scene3D::vertexBuffer[ap0].y = 0;
            Scene3D::vertexBuffer[ap0].z = tv2;
            Scene3D::vertexBuffer[ap0].u = tv4;
            Scene3D::vertexBuffer[ap0].v = tv5;
            ap0++;

            tv2 += 512;
            Scene3D::vertexBuffer[ap0].x = tv1;
            Scene3D::vertexBuffer[ap0].y = 0;
            Scene3D::vertexBuffer[ap0].z = tv2;
            Scene3D::vertexBuffer[ap0].u = tv4;
            Scene3D::vertexBuffer[ap0].v = tv5 + 63;
            ap0++;

            tv2 -= 512;
            tv1 += 1024;
            Scene3D::vertexBuffer[ap0].x = tv1;
            Scene3D::vertexBuffer[ap0].y = 0;
            Scene3D::vertexBuffer[ap0].z = tv2;
            Scene3D::vertexBuffer[ap0].u = tv4 + 63;
            Scene3D::vertexBuffer[ap0].v = tv5;
            ap0++;

            tv2 += 512;
            Scene3D::vertexBuffer[ap0].x = tv1;
            Scene3D::vertexBuffer[ap0].y = 0;
            Scene3D::vertexBuffer[ap0].z = tv2;
            Scene3D::vertexBuffer[ap0].u = tv4 + 63;
            Scene3D::vertexBuffer[ap0].v = tv5 + 63;

            tv4  = (tv4 + 64) & 255;
            tv2 -= 512;

            ap0 -= 3;

            Scene3D::indexBuffer[ap1].a = ap0;
            Scene3D::indexBuffer[ap1].b = ap0 + 2;
            Scene3D::indexBuffer[ap1].c = ap0 + 1;
            Scene3D::indexBuffer[ap1].d = ap0 + 3;
            Scene3D::indexBuffer[ap1].flag = FACE_FLAG_TEXTURED_3D;

            ap0 += 4;
            ap1++;
            tv0++;

            Scene3D::numVertices += 4;
            Scene3D::numFaces++;
        }

        tv5  = (tv5 + 64) & 255;
        tv2 += 512;
        tv3++;
    }
}

void Clouds_Draw(int32_t entityNo)
{
    ObjectEntity& e = ObjectSystem::objectEntityList[entityNo];

    e.value[0]  = (e.value[0] + 8) & 0x7FF;
    int32_t scroll = -e.value[0];

    NS_MatrixTranslateXYZ(MAT_WORLD, -512, 720, scroll);
    NS_Draw3DScene(entityNo);
}

}

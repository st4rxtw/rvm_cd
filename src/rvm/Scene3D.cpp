#include "Scene3D.h"
#include "GlobalAppDefinitions.h"
#include "GraphicsSystem.h"

namespace rvm {

Vertex3D Scene3D::vertexBuffer[4096]{};
Vertex3D Scene3D::vertexBufferT[4096]{};
Face3D   Scene3D::indexBuffer[1024]{};
SortList Scene3D::drawList[1024]{};
int32_t  Scene3D::numVertices  = 0;
int32_t  Scene3D::numFaces     = 0;
int32_t  Scene3D::projectionX  = 136;
int32_t  Scene3D::projectionY  = 160;
int32_t  Scene3D::matWorld[16]{};
int32_t  Scene3D::matView[16]{};
int32_t  Scene3D::matFinal[16]{};
int32_t  Scene3D::matTemp[16]{};

void Scene3D::SetIdentityMatrix(int32_t* m)
{
    m[0]=256; m[1]=0;   m[2]=0;   m[3]=0;
    m[4]=0;   m[5]=256; m[6]=0;   m[7]=0;
    m[8]=0;   m[9]=0;   m[10]=256;m[11]=0;
    m[12]=0;  m[13]=0;  m[14]=0;  m[15]=256;
}

void Scene3D::MatrixMultiply(int32_t* a, const int32_t* b)
{
    int32_t tmp[16];
    for (uint32_t i = 0; i < 16; ++i) {
        uint32_t col = i & 3;
        uint32_t row = i & 12;
        tmp[i] = (b[col]   * a[row]   >> 8)
               + (b[col+4] * a[row+1] >> 8)
               + (b[col+8] * a[row+2] >> 8)
               + (b[col+12]* a[row+3] >> 8);
    }
    for (int i = 0; i < 16; ++i) a[i] = tmp[i];
}

void Scene3D::MatrixTranslateXYZ(int32_t* m, int32_t xPos, int32_t yPos, int32_t zPos)
{
    SetIdentityMatrix(m);
    m[12] = xPos; m[13] = yPos; m[14] = zPos;
}

void Scene3D::MatrixScaleXYZ(int32_t* m, int32_t xScale, int32_t yScale, int32_t zScale)
{
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[0] = xScale; m[5] = yScale; m[10] = zScale; m[15] = 256;
}

void Scene3D::MatrixRotateX(int32_t* m, int32_t angle)
{
    if (angle < 0) angle = 512 - angle;
    angle &= 511;
    int32_t s = GlobalAppDefinitions::SinValue512[angle] >> 1;
    int32_t c = GlobalAppDefinitions::CosValue512[angle] >> 1;
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[0]=256; m[5]=c; m[6]=s; m[9]=-s; m[10]=c; m[15]=256;
}

void Scene3D::MatrixRotateY(int32_t* m, int32_t angle)
{
    if (angle < 0) angle = 512 - angle;
    angle &= 511;
    int32_t s = GlobalAppDefinitions::SinValue512[angle] >> 1;
    int32_t c = GlobalAppDefinitions::CosValue512[angle] >> 1;
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[0]=c; m[2]=s; m[5]=256; m[8]=-s; m[10]=c; m[15]=256;
}

void Scene3D::MatrixRotateZ(int32_t* m, int32_t angle)
{
    if (angle < 0) angle = 512 - angle;
    angle &= 511;
    int32_t s = GlobalAppDefinitions::SinValue512[angle] >> 1;
    int32_t c = GlobalAppDefinitions::CosValue512[angle] >> 1;
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[0]=c; m[2]=s; m[5]=256; m[8]=-s; m[10]=c; m[15]=256;
}

void Scene3D::MatrixRotateXYZ(int32_t* m, int32_t angleX, int32_t angleY, int32_t angleZ)
{
    if (angleX < 0) angleX = 512 - angleX; angleX &= 511;
    if (angleY < 0) angleY = 512 - angleY; angleY &= 511;
    if (angleZ < 0) angleZ = 512 - angleZ; angleZ &= 511;
    int32_t sx = GlobalAppDefinitions::SinValue512[angleX] >> 1;
    int32_t cx = GlobalAppDefinitions::CosValue512[angleX] >> 1;
    int32_t sy = GlobalAppDefinitions::SinValue512[angleY] >> 1;
    int32_t cy = GlobalAppDefinitions::CosValue512[angleY] >> 1;
    int32_t sz = GlobalAppDefinitions::SinValue512[angleZ] >> 1;
    int32_t cz = GlobalAppDefinitions::CosValue512[angleZ] >> 1;
    m[0]  = (cy*cz>>8) + ((sx*sy>>8)*sz>>8);
    m[1]  = (cy*sz>>8) - ((sx*sy>>8)*cz>>8);
    m[2]  = cx*sy>>8;
    m[3]  = 0;
    m[4]  = -cx*sz>>8;
    m[5]  = cx*cz>>8;
    m[6]  = sx;
    m[7]  = 0;
    m[8]  = ((sx*cy>>8)*sz>>8) - (sy*cz>>8);
    m[9]  = (-sy*sz>>8) - ((sx*cy>>8)*cz>>8);
    m[10] = cx*cy>>8;
    m[11] = 0;
    m[12] = m[13] = m[14] = 0;
    m[15] = 256;
}

void Scene3D::TransformVertexBuffer()
{
    for (int i = 0; i < 16; ++i) matFinal[i] = matWorld[i];
    MatrixMultiply(matFinal, matView);
    for (int i = 0; i < numVertices; ++i) {
        const Vertex3D& v = vertexBuffer[i];
        Vertex3D& t = vertexBufferT[i];
        t.x = (matFinal[0]*v.x>>8)+(matFinal[4]*v.y>>8)+(matFinal[8]*v.z>>8) +matFinal[12];
        t.y = (matFinal[1]*v.x>>8)+(matFinal[5]*v.y>>8)+(matFinal[9]*v.z>>8) +matFinal[13];
        t.z = (matFinal[2]*v.x>>8)+(matFinal[6]*v.y>>8)+(matFinal[10]*v.z>>8)+matFinal[14];
        if (t.z < 1 && t.z > 0) t.z = 1;
    }
}

void Scene3D::TransformVertices(const int32_t* m, int32_t vStart, int32_t vEnd)
{
    ++vEnd;
    for (int i = vStart; i < vEnd; ++i) {
        Vertex3D& v = vertexBuffer[i];
        Vertex3D tmp;
        tmp.x = (m[0]*v.x>>8)+(m[4]*v.y>>8)+(m[8]*v.z>>8) +m[12];
        tmp.y = (m[1]*v.x>>8)+(m[5]*v.y>>8)+(m[9]*v.z>>8) +m[13];
        tmp.z = (m[2]*v.x>>8)+(m[6]*v.y>>8)+(m[10]*v.z>>8)+m[14];
        v.x = tmp.x; v.y = tmp.y; v.z = tmp.z;
    }
}

void Scene3D::Sort3DDrawList()
{
    for (int i = 0; i < numFaces; ++i) {
        drawList[i].z =
            (vertexBufferT[indexBuffer[i].a].z +
             vertexBufferT[indexBuffer[i].b].z +
             vertexBufferT[indexBuffer[i].c].z +
             vertexBufferT[indexBuffer[i].d].z) >> 2;
        drawList[i].index = i;
    }
    for (int i = 0; i < numFaces; ++i) {
        for (int j = numFaces - 1; j > i; --j) {
            if (drawList[j].z > drawList[j-1].z) {
                SortList tmp  = drawList[j];
                drawList[j]   = drawList[j-1];
                drawList[j-1] = tmp;
            }
        }
    }
}

void Scene3D::Draw3DScene(int32_t surfaceNum)
{
    Quad2D face{};
    for (int i = 0; i < numFaces; ++i) {
        const Face3D& f = indexBuffer[drawList[i].index];
        switch (f.flag) {
            case 0:
                if (vertexBufferT[f.a].z>256 && vertexBufferT[f.b].z>256 &&
                    vertexBufferT[f.c].z>256 && vertexBufferT[f.d].z>256) {
                    int sc = GlobalAppDefinitions::SCREEN_CENTER;
                    face.vertex[0].x = sc + vertexBufferT[f.a].x*projectionX/vertexBufferT[f.a].z;
                    face.vertex[0].y = 120 - vertexBufferT[f.a].y*projectionY/vertexBufferT[f.a].z;
                    face.vertex[1].x = sc + vertexBufferT[f.b].x*projectionX/vertexBufferT[f.b].z;
                    face.vertex[1].y = 120 - vertexBufferT[f.b].y*projectionY/vertexBufferT[f.b].z;
                    face.vertex[2].x = sc + vertexBufferT[f.c].x*projectionX/vertexBufferT[f.c].z;
                    face.vertex[2].y = 120 - vertexBufferT[f.c].y*projectionY/vertexBufferT[f.c].z;
                    face.vertex[3].x = sc + vertexBufferT[f.d].x*projectionX/vertexBufferT[f.d].z;
                    face.vertex[3].y = 120 - vertexBufferT[f.d].y*projectionY/vertexBufferT[f.d].z;
                    face.vertex[0].u = vertexBuffer[f.a].u; face.vertex[0].v = vertexBuffer[f.a].v;
                    face.vertex[1].u = vertexBuffer[f.b].u; face.vertex[1].v = vertexBuffer[f.b].v;
                    face.vertex[2].u = vertexBuffer[f.c].u; face.vertex[2].v = vertexBuffer[f.c].v;
                    face.vertex[3].u = vertexBuffer[f.d].u; face.vertex[3].v = vertexBuffer[f.d].v;
                    GraphicsSystem::DrawTexturedQuad(face, surfaceNum);
                }
                break;
            case 1:
                face.vertex[0].x=vertexBuffer[f.a].x; face.vertex[0].y=vertexBuffer[f.a].y;
                face.vertex[1].x=vertexBuffer[f.b].x; face.vertex[1].y=vertexBuffer[f.b].y;
                face.vertex[2].x=vertexBuffer[f.c].x; face.vertex[2].y=vertexBuffer[f.c].y;
                face.vertex[3].x=vertexBuffer[f.d].x; face.vertex[3].y=vertexBuffer[f.d].y;
                face.vertex[0].u=vertexBuffer[f.a].u; face.vertex[0].v=vertexBuffer[f.a].v;
                face.vertex[1].u=vertexBuffer[f.b].u; face.vertex[1].v=vertexBuffer[f.b].v;
                face.vertex[2].u=vertexBuffer[f.c].u; face.vertex[2].v=vertexBuffer[f.c].v;
                face.vertex[3].u=vertexBuffer[f.d].u; face.vertex[3].v=vertexBuffer[f.d].v;
                GraphicsSystem::DrawTexturedQuad(face, surfaceNum);
                break;
            case 2:
                if (vertexBufferT[f.a].z>256 && vertexBufferT[f.b].z>256 &&
                    vertexBufferT[f.c].z>256 && vertexBufferT[f.d].z>256) {
                    int sc = GlobalAppDefinitions::SCREEN_CENTER;
                    face.vertex[0].x = sc + vertexBufferT[f.a].x*projectionX/vertexBufferT[f.a].z;
                    face.vertex[0].y = 120 - vertexBufferT[f.a].y*projectionY/vertexBufferT[f.a].z;
                    face.vertex[1].x = sc + vertexBufferT[f.b].x*projectionX/vertexBufferT[f.b].z;
                    face.vertex[1].y = 120 - vertexBufferT[f.b].y*projectionY/vertexBufferT[f.b].z;
                    face.vertex[2].x = sc + vertexBufferT[f.c].x*projectionX/vertexBufferT[f.c].z;
                    face.vertex[2].y = 120 - vertexBufferT[f.c].y*projectionY/vertexBufferT[f.c].z;
                    face.vertex[3].x = sc + vertexBufferT[f.d].x*projectionX/vertexBufferT[f.d].z;
                    face.vertex[3].y = 120 - vertexBufferT[f.d].y*projectionY/vertexBufferT[f.d].z;
                    GraphicsSystem::DrawQuad(face, f.color);
                }
                break;
            case 3:
                face.vertex[0].x=vertexBuffer[f.a].x; face.vertex[0].y=vertexBuffer[f.a].y;
                face.vertex[1].x=vertexBuffer[f.b].x; face.vertex[1].y=vertexBuffer[f.b].y;
                face.vertex[2].x=vertexBuffer[f.c].x; face.vertex[2].y=vertexBuffer[f.c].y;
                face.vertex[3].x=vertexBuffer[f.d].x; face.vertex[3].y=vertexBuffer[f.d].y;
                GraphicsSystem::DrawQuad(face, f.color);
                break;
        }
    }
}

}

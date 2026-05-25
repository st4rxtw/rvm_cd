#pragma once
#include <cstdint>
#include "Vertex3D.h"
#include "Face3D.h"
#include "SortList.h"

namespace rvm {

struct Scene3D {
    static Vertex3D vertexBuffer[4096];
    static Vertex3D vertexBufferT[4096];
    static Face3D   indexBuffer[1024];
    static SortList drawList[1024];
    static int32_t  numVertices;
    static int32_t  numFaces;
    static int32_t  projectionX;
    static int32_t  projectionY;
    static int32_t  matWorld[16];
    static int32_t  matView[16];
    static int32_t  matFinal[16];
    static int32_t  matTemp[16];

    static void SetIdentityMatrix(int32_t* m);
    static void MatrixMultiply(int32_t* a, const int32_t* b);
    static void MatrixTranslateXYZ(int32_t* m, int32_t xPos, int32_t yPos, int32_t zPos);
    static void MatrixScaleXYZ(int32_t* m, int32_t xScale, int32_t yScale, int32_t zScale);
    static void MatrixRotateX(int32_t* m, int32_t angle);
    static void MatrixRotateY(int32_t* m, int32_t angle);
    static void MatrixRotateZ(int32_t* m, int32_t angle);
    static void MatrixRotateXYZ(int32_t* m, int32_t angleX, int32_t angleY, int32_t angleZ);
    static void TransformVertexBuffer();
    static void TransformVertices(const int32_t* m, int32_t vStart, int32_t vEnd);
    static void Sort3DDrawList();
    static void Draw3DScene(int32_t surfaceNum);
};

}

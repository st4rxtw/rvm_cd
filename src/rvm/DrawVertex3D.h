#pragma once
#include <cstdint>
#include "DrawVertex.h"

namespace rvm {

struct DrawVertex3D {
    float       x{}, y{}, z{};
    float       u{}, v{};
    VertexColor color{};

    DrawVertex3D() = default;
    DrawVertex3D(float x, float y, float z, float u, float v, VertexColor c)
        : x(x), y(y), z(z), u(u), v(v), color(c) {}
};

}

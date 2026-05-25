#pragma once
#include <cstdint>

namespace rvm {

struct VertexColor {
    uint8_t r{}, g{}, b{}, a{};
    constexpr VertexColor() = default;
    constexpr VertexColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : r(r), g(g), b(b), a(a) {}
};

struct DrawVertex {
    float       x{}, y{};
    float       u{}, v{};
    VertexColor color{};

    DrawVertex() = default;
    DrawVertex(float x, float y, float u, float v, VertexColor c)
        : x(x), y(y), u(u), v(v), color(c) {}
};

}

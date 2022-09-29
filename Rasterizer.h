//
// Created by admin on 2022/9/23.
//

#ifndef CG_BASIC_RASTERIZER_H
#define CG_BASIC_RASTERIZER_H

#include <array>
#include "Primitive.h"
#include "Shader.h"

class ScreenBuffer;

struct RasterizerPayload {
    std::array<Primitive::GPUVertex *, 3> &triangleVertexes;
    std::deque<Primitive::Light> &lightList;
};

class Rasterizer {
public:
    Rasterizer(ScreenBuffer &screenBuffer, Primitive::Material &material,
               std::function<void(const Shader::FragmentShaderPayload &)> &fragmentShader);

    ScreenBuffer &screenBuffer;
    Primitive::Material &material;
    std::function<void(const Shader::FragmentShaderPayload &)> &fragmentShader;

    virtual void rasterizeTriangle(const RasterizerPayload &payload);

    static bool checkInsideTriangle(float posX, float posY, std::array<Primitive::GPUVertex *, 3> &triangleVertexes);

    static std::array<float, 3>
    getBarycentricCoordinates(float x, float y, std::array<Primitive::GPUVertex *, 3> &triangleVertexes);

    static std::array<float, 3>
    convertBarycentricCoordinates(float screenSpaceAlpha, float screenSpaceBeta, float screenSpaceGamma,
                                  std::array<Primitive::GPUVertex *, 3> &triangleVertexes);

    void rasterizeTriangleLine(const RasterizerPayload &payload);

    void drawScreenSpacePoint(Eigen::Vector3f &pointScreenSpacePos, const RasterizerPayload &payload);
};


#endif //CG_BASIC_RASTERIZER_H

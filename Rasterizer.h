//
// Created by admin on 2022/9/23.
//

#ifndef CG_BASIC_RASTERIZER_H
#define CG_BASIC_RASTERIZER_H


#include <array>
#include "Primitive.h"
#include "Shader.h"
#include "ScreenBuffer.h"

class Renderer;

struct RasterizerPayload {
    std::array<Primitive::Vertex *, 3> &triangleVertexes;
    std::array<Eigen::Vector3f, 3> &vertexesViewSpacePos;
    std::deque<Primitive::Light> &lightList;
};

class Rasterizer {
public:
    Rasterizer(ScreenBuffer &screenBuffer, Primitive::Material &material,
               std::function<void(Shader::FragmentShaderPayload &)> &fragmentShader);

    ScreenBuffer &screenBuffer;
    Primitive::Material &material;
    std::function<void(Shader::FragmentShaderPayload &)> &fragmentShader;

    virtual void rasterizeTriangle(const RasterizerPayload &payload);

    static bool checkInsideTriangle(float posX, float posY, std::array<Primitive::Vertex *, 3> &triangleVertexes);

    static std::array<float, 3>
    getBarycentricCoordinates(float x, float y, std::array<Primitive::Vertex *, 3> &triangleVertexes);

    static std::array<float, 3>
    convertBarycentricCoordinates(float screenSpaceAlpha, float screenSpaceBeta, float screenSpaceGamma,
                                  std::array<Eigen::Vector3f, 3> &vertexesViewSpacePos);

    void rasterizeTriangleLine(const RasterizerPayload &payload);
};


#endif //CG_BASIC_RASTERIZER_H

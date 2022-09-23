//
// Created by admin on 2022/9/19.
//

#ifndef CG_BASIC_RENDERER_H
#define CG_BASIC_RENDERER_H

#include <vector>
#include <eigen3/Eigen/Eigen>
#include <opencv2/opencv.hpp>
#include "Primitive.h"
#include "Shader.h"
#include "Rasterizer.h"
#include "ScreenBuffer.h"

struct RendererPayload {
    Primitive::Geometry &geometry;
    std::function<void(Shader::VertexShaderPayload &)> &vertexShader;
    std::function<void(Shader::FragmentShaderPayload &)> &fragmentShader;
    std::deque<Primitive::Light> &lightList;
};

class Renderer {
public:
    ScreenBuffer &screenBuffer;
    std::deque<Primitive::Vertex> vertexes;
    std::deque<uint> indexes;
    Eigen::Matrix4f modelMatrix;
    Eigen::Matrix4f viewMatrix;
    Eigen::Matrix4f projectionMatrix;
    std::deque<Primitive::Light> lightList;

    Renderer(ScreenBuffer &screenBuffer);

    void renderGeometry(const RendererPayload &payload);

    void transformLights();
};

#endif //CG_BASIC_RENDERER_H

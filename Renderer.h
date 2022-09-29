//
// Created by admin on 2022/9/19.
//

#ifndef CG_BASIC_RENDERER_H
#define CG_BASIC_RENDERER_H


#include <vector>
#include <eigen3/Eigen/Eigen>
#include "Primitive.h"
#include "Shader.h"
#include "Statics.h"

class ScreenBuffer;

class CameraObject;

struct RendererPayload {
    Primitive::Geometry &geometry;
    std::function<void(const Shader::VertexShaderPayload &)> &vertexShader;
    std::function<void(const Shader::FragmentShaderPayload &)> &fragmentShader;
    std::deque<Primitive::Light> &lightList;
};

class Renderer {
public:
    ScreenBuffer &screenBuffer;
    CameraObject &cameraObject;
    std::deque<Primitive::GPUVertex> vertexes;
    std::deque<uint> indexes;
    Eigen::Matrix4f modelMatrix;
    Eigen::Matrix4f viewMatrix;
    Eigen::Matrix4f projectionMatrix;
    std::deque<Primitive::Light> lightList;
    RenderMode renderMode = DEFAULT;

    Renderer(ScreenBuffer &screenBuffer, CameraObject &cameraObject);

    void renderGeometry(const RendererPayload &payload);

    bool clipTriangle(const Eigen::Vector3f& paneNormal, const Eigen::Vector3f& panePoint, int indexesI);

    void transformLights();
};

#endif //CG_BASIC_RENDERER_H

//
// Created by admin on 2022/9/19.
//

#ifndef CG_BASIC_RENDERER_H
#define CG_BASIC_RENDERER_H


#include <vector>
#include <eigen3/Eigen/Eigen>
#include "Primitive.h"
#include "Shader.h"

class ScreenBuffer;

class CameraObject;

struct RenderOption {
    bool zWrite = true;
    bool zTest = true;
    enum Culling {
        CULL_FRONT, CULL_BACK, CULL_NONE
    } culling = CULL_BACK;
    enum RenderMode {
        MODE_DEFAULT, MODE_LINE_ONLY
    } renderMode = MODE_DEFAULT;
};

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
    Eigen::Matrix3f normalMatrix;
    std::deque<Primitive::Light> lightList;
    RenderOption renderOption;

    Renderer(ScreenBuffer &screenBuffer, CameraObject &cameraObject);

    void renderGeometry(const RendererPayload &payload);

    bool clipTriangle(int indexesI);

    bool cullTriangle(int indexesI);

    void transformLights();

    template<typename T>
    T lineLerp(T &a1, T &a2, float weight);

    Primitive::GPUVertex lineLerp(Primitive::GPUVertex &a1, Primitive::GPUVertex &a2, float weight);
};

#endif //CG_BASIC_RENDERER_H

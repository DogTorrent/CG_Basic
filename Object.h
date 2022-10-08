//
// Created by admin on 2022/9/28.
//

#ifndef CG_BASIC_OBJECT_H
#define CG_BASIC_OBJECT_H

#include <deque>
#include <eigen3/Eigen/Core>
#include "Primitive.h"
#include "Shader.h"
#include "Renderer.h"

class SceneObject {
public:
    std::deque<Primitive::Geometry> geometryList;
    Eigen::Vector3f scalingRatio;
    Eigen::Vector4f rotationAxis;
    float rotationDegree = 0;
    Eigen::Vector4f modelPos;
    std::function<void(const Shader::VertexShaderPayload &)> vertexShader;
    std::function<void(const Shader::FragmentShaderPayload &)> fragmentShader;
    RenderOption renderOption;
};

class CameraObject {
public:
    Eigen::Vector4f pos;
    Eigen::Vector4f toward;
    Eigen::Vector4f top;
    float FoV = 45;
    float aspectRatio = 1;
    float nearPaneZ = 1;
    float farPaneZ = 50;
    void rotate(const Eigen::Vector4f &axis, float angleDegree);
    void moveRight(float delta);
    void moveUp(float delta);
    void moveForward(float delta);
};

#endif //CG_BASIC_OBJECT_H

//
// Created by admin on 2022/9/20.
//

#ifndef CG_BASIC_SHADER_H
#define CG_BASIC_SHADER_H


#include <functional>
#include <eigen3/Eigen/Core>
#include "Primitive.h"

namespace Shader {
    struct FragmentShaderPayload {
        Eigen::Vector3f &viewSpacePos;
        Eigen::Vector3f &color;
        Eigen::Vector3f &normal;
        Eigen::Vector2f &uv;
        std::deque<Primitive::Light> &lights;
        Primitive::Material &material;
    };

    struct VertexShaderPayload {
        Primitive::Vertex &vertex;
        Eigen::Vector3f &viewSpacePos;
        Eigen::Matrix4f &modelMatrix;
        Eigen::Matrix4f &viewMatrix;
        Eigen::Matrix4f &projectionMatrix;
    };

    void basicVertexShader(VertexShaderPayload &payload);

    void emptyVertexShader(VertexShaderPayload &payload);

    void basicFragmentShader(FragmentShaderPayload &payload);

    void emptyFragmentShader(FragmentShaderPayload &payload);

    void textureFragmentShader(FragmentShaderPayload &payload);

    void blinnPhongFragmentShader(FragmentShaderPayload &payload);
};


#endif //CG_BASIC_SHADER_H

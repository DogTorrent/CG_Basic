//
// Created by admin on 2022/9/20.
//

#include "Shader.h"

namespace Shader {
    void basicVertexShader(const VertexShaderPayload &payload) {
        Eigen::Matrix4f mv = payload.viewMatrix * payload.modelMatrix;
        // model and view transform
        // model_space -> world_space -> view_space
        payload.vertex.viewSpacePos = mv * payload.vertex.pos;

        // projection transform
        // view_space -> clip_space
        payload.vertex.pos = payload.projectionMatrix * payload.vertex.viewSpacePos;
    };

    void emptyVertexShader(const VertexShaderPayload &payload) {
    };

    void basicFragmentShader(const FragmentShaderPayload &payload) {
    };

    void emptyFragmentShader(const FragmentShaderPayload &payload) {
    };

    void textureFragmentShader(const FragmentShaderPayload &payload) {
        payload.color = payload.material.diffuseTexture.getValue(payload.uv.x(), payload.uv.y());
    };

    void blinnPhongFragmentShader(const FragmentShaderPayload &payload) {
        if (payload.lights.empty()) return;
        Eigen::Vector3f ka = payload.material.ka; // Ambient factor
        Eigen::Vector3f ks = payload.material.ks; // Specular factor, or called Shininess
        Eigen::Vector3f kd = payload.material.kd;
        if (!payload.material.diffuseTexture.isEmpty())
            kd = payload.material.diffuseTexture.getValue(payload.uv.x(), payload.uv.y()) / 255.f; // Diffuse factor
        float ns = payload.material.ns; // Specular range exponent

        Eigen::Vector3f ambientIntensity(0.005, 0.005, 0.005);

        Eigen::Vector3f La = Eigen::Vector3f::Zero(), Ld = Eigen::Vector3f::Zero(), Ls = Eigen::Vector3f::Zero();
        for (auto &light: payload.lights) {
            // For each light source in the code, calculate the ambient, diffuse and specular and accumulate them
            Eigen::Vector3f l = light.pos - payload.viewSpacePos;
            Eigen::Vector3f v = -payload.viewSpacePos;
            Eigen::Vector3f h = l.normalized() + v.normalized();
            float r2 = l.squaredNorm();
            float cos_nl = payload.normal.dot(l) / (payload.normal.norm() * l.norm());
            float cos_nh = payload.normal.dot(h) / (payload.normal.norm() * h.norm());

            La += ka.cwiseProduct(ambientIntensity);
            Ld += kd.cwiseProduct(light.intensity / r2) * MAX(0, cos_nl);
            Ls += ks.cwiseProduct(light.intensity / r2) * pow(MAX(0, cos_nh), ns);
        }
        payload.color = (La + Ld + Ls) * 255.f;
    }
}
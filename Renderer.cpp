//
// Created by admin on 2022/9/19.
//

#include "Renderer.h"
#include "Rasterizer.h"
#include "ScreenBuffer.h"

Renderer::Renderer(ScreenBuffer &screenBuffer) : screenBuffer(screenBuffer) {};

// transform the light from world space to view space
void Renderer::transformLights() {
    for (auto &light: lightList) {
        Eigen::Vector4f pos4(light.pos.x(), light.pos.y(), light.pos.z(), 1);
        pos4 = viewMatrix * pos4;
        light.pos.x() = pos4.x();
        light.pos.y() = pos4.y();
        light.pos.z() = pos4.z();
    }
}

void Renderer::renderGeometry(const RendererPayload &payload) {
    Primitive::Geometry &geometry = payload.geometry;
    // clear
    vertexes.clear();
    indexes.clear();
    lightList.clear();

    // copy data
    Primitive::Material material = geometry.material;
    geometry.material.diffuseTexture.copyTo(material.diffuseTexture);
    for (const auto &light: payload.lightList) {
        lightList.push_back(light);
    }
    for (auto &vertex: geometry.mesh.vertexes) {
        vertexes.push_back(vertex);
    }
    for (auto index: geometry.mesh.indexes) {
        indexes.push_back(index);
    }

    transformLights();
    Rasterizer rasterizer(screenBuffer, material, payload.fragmentShader);
    for (int indexesI = 0; indexesI < indexes.size(); indexesI += 3) {
        std::vector<Shader::VertexShaderPayload> vertexShaderPayload;
        vertexShaderPayload.reserve(3);
        std::array<Primitive::Vertex *, 3> triangleVertexes{&vertexes[indexes[indexesI + 0]],
                                                            &vertexes[indexes[indexesI + 1]],
                                                            &vertexes[indexes[indexesI + 2]]};
        std::array<Eigen::Vector3f, 3> triangleViewSpacePos;
        for (int i = 0; i < 3; ++i) {
            // create shader payload
            vertexShaderPayload.push_back(
                    Shader::VertexShaderPayload{*triangleVertexes[i], triangleViewSpacePos[i], modelMatrix, viewMatrix,
                                                projectionMatrix});
        }
        for (int i = 0; i < 3; ++i) {
            // apply mvp transformation
            Shader::basicVertexShader(vertexShaderPayload[i]);
        }

        // todo
        // clip

        for (int i = 0; i < 3; ++i) {
            Primitive::Vertex &vertex = *triangleVertexes[i];

            // apply vertex shader
            payload.vertexShader(vertexShaderPayload[i]);

            // Homogeneous division
            // clip_space -> ndc_space, but do not divide w by w
            vertex.pos /= vertex.pos.w();

            // Viewport transformation
            // ndc_space -> screen_space
            // [-1, 1] => [0, width], [-1, 1] => [0, height], [-1, 1] => [0, MAX_DEPTH]
            vertex.pos.x() = 0.5f * (float) screenBuffer.width * (vertex.pos.x() + 1.0f);
            vertex.pos.y() = 0.5f * (float) screenBuffer.height * (vertex.pos.y() + 1.0f);
            vertex.pos.z() = 0.5f * (float) MAX_DEPTH * (vertex.pos.z() + 1.0f);
        }
        RasterizerPayload rasterizerPayload{triangleVertexes, triangleViewSpacePos, lightList};
        rasterizer.rasterizeTriangle(rasterizerPayload);
    }
}

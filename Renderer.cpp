//
// Created by admin on 2022/9/19.
//

#include <iostream>
#include "Renderer.h"
#include "Rasterizer.h"
#include "ScreenBuffer.h"
#include "Object.h"

Renderer::Renderer(ScreenBuffer &screenBuffer, CameraObject &cameraObject) : screenBuffer(screenBuffer),
                                                                             cameraObject(cameraObject) {};

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
        vertexes.emplace_back(vertex);
    }
    for (auto index: geometry.mesh.indexes) {
        indexes.push_back(index);
    }

    transformLights();

    for (auto &vertex: vertexes) {
        // apply mvp transformation
        Shader::basicVertexShader(Shader::VertexShaderPayload{vertex, modelMatrix, viewMatrix, projectionMatrix});
    }

    auto cosHalfVFoV = (float) cos(cameraObject.FoV / 2 / 180 * EIGEN_PI);
    auto sinHalfVFoV = (float) sin(cameraObject.FoV / 2 / 180 * EIGEN_PI);
    auto halfHFoV = atan(cameraObject.aspectRatio * (float) tan(cameraObject.FoV / 2 / 180 * EIGEN_PI));
    auto cosHalfHFoV = (float) cos(halfHFoV);
    auto sinHalfHFoV = (float) sin(halfHFoV);
    Rasterizer rasterizer(screenBuffer, material, payload.fragmentShader);
    for (int indexesI = 0; indexesI < indexes.size(); indexesI += 3) {
        std::array<Primitive::GPUVertex *, 3> triangleVertexes{&vertexes[indexes[indexesI + 0]],
                                                               &vertexes[indexes[indexesI + 1]],
                                                               &vertexes[indexes[indexesI + 2]]};
        if ((triangleVertexes[0]->clipSpacePos - triangleVertexes[1]->clipSpacePos).isZero(1e-5)
            || (triangleVertexes[1]->clipSpacePos - triangleVertexes[2]->clipSpacePos).isZero(1e-5)
            || (triangleVertexes[2]->clipSpacePos - triangleVertexes[0]->clipSpacePos).isZero(1e-5))
            continue;
        // clip
        Eigen::Vector3f paneNormal;
        Eigen::Vector3f panePoint(0, 0, 0);
        //top
        paneNormal = {0, cosHalfVFoV, -sinHalfVFoV};
        if (!clipTriangle(paneNormal, panePoint, indexesI)) continue;
        //bottom
        paneNormal = {0, -cosHalfVFoV, -sinHalfVFoV};
        if (!clipTriangle(paneNormal, panePoint, indexesI)) continue;
        //left
        paneNormal = {cosHalfHFoV, 0, -sinHalfHFoV};
        if (!clipTriangle(paneNormal, panePoint, indexesI)) continue;
        //right
        paneNormal = {-cosHalfHFoV, 0, -sinHalfHFoV};
        if (!clipTriangle(paneNormal, panePoint, indexesI)) continue;
        //near
        paneNormal = {0, 0, -1};
        panePoint = {0, 0, -cameraObject.nearPaneZ};
        if (!clipTriangle(paneNormal, panePoint, indexesI)) continue;
        //far
        paneNormal = {0, 0, 1};
        panePoint = {0, 0, cameraObject.farPaneZ};
        if (!clipTriangle(paneNormal, panePoint, indexesI)) continue;

        for (int i = 0; i < 3; ++i) {
            Primitive::GPUVertex &vertex = *triangleVertexes[i];

            // apply vertex shader
            payload.vertexShader(Shader::VertexShaderPayload{vertex, modelMatrix, viewMatrix, projectionMatrix});

            // Homogeneous division
            // clip_space -> ndc_space
            vertex.ndcSpacePos = vertex.clipSpacePos / vertex.clipSpacePos.w();

            // Viewport transformation
            // ndc_space -> screen_space
            // [-1, 1] => [0, width], [-1, 1] => [0, height], [-1, 1] => [0, MAX_DEPTH]
            vertex.screenSpacePos.x() = 0.5f * (float) screenBuffer.width * (vertex.ndcSpacePos.x() + 1.0f);
            vertex.screenSpacePos.y() = 0.5f * (float) screenBuffer.height * (vertex.ndcSpacePos.y() + 1.0f);
            vertex.screenSpacePos.z() = 0.5f * (float) MAX_DEPTH * (vertex.ndcSpacePos.z() + 1.0f);
        }

        // ignore line/dot
        if ((triangleVertexes[0]->screenSpacePos.head(2) - triangleVertexes[1]->screenSpacePos.head(2)).isZero(1e-5)
            || (triangleVertexes[1]->screenSpacePos.head(2) - triangleVertexes[2]->screenSpacePos.head(2)).isZero(1e-5)
            || (triangleVertexes[2]->screenSpacePos.head(2) - triangleVertexes[0]->screenSpacePos.head(2)).isZero(1e-5))
            continue;

        RasterizerPayload rasterizerPayload{triangleVertexes, lightList};
        if (renderMode == DEFAULT) rasterizer.rasterizeTriangle(rasterizerPayload);
        else if (renderMode == LINE_ONLY) rasterizer.rasterizeTriangleLine(rasterizerPayload);
    }
}

bool Renderer::clipTriangle(const Eigen::Vector3f &paneNormal, const Eigen::Vector3f &panePoint, int indexesI) {
    std::array<Primitive::GPUVertex *, 3> triangleVertexes{&vertexes[indexes[indexesI + 0]],
                                                           &vertexes[indexes[indexesI + 1]],
                                                           &vertexes[indexes[indexesI + 2]]};
    std::array<float, 3> dotProduct{(triangleVertexes[0]->clipSpacePos.head(3) - panePoint).dot(paneNormal),
                                    (triangleVertexes[1]->clipSpacePos.head(3) - panePoint).dot(paneNormal),
                                    (triangleVertexes[2]->clipSpacePos.head(3) - panePoint).dot(paneNormal)};
    int outCount = 0;
    for (auto item: dotProduct) {
        if (item > 1e-5) ++outCount;
    }
    if (outCount == 0) return true;
    // 1 out but 2 in
    if (outCount == 1) {
        int outI = 0;
        while (dotProduct[outI] <= 1e-5) ++outI;
        float alpha, beta;

        Primitive::GPUVertex newV1;
        alpha = dotProduct[outI] / (dotProduct[outI] - dotProduct[(outI + 2) % 3]);
        beta = 1 - alpha;
        newV1.pos = alpha * triangleVertexes[(outI + 2) % 3]->pos + beta * triangleVertexes[outI]->pos;
        newV1.clipSpacePos =
                alpha * triangleVertexes[(outI + 2) % 3]->clipSpacePos + beta * triangleVertexes[outI]->clipSpacePos;
        newV1.viewSpacePos =
                alpha * triangleVertexes[(outI + 2) % 3]->viewSpacePos + beta * triangleVertexes[outI]->viewSpacePos;
        newV1.uv = alpha * triangleVertexes[(outI + 2) % 3]->uv + beta * triangleVertexes[outI]->uv;
        newV1.normal = alpha * triangleVertexes[(outI + 2) % 3]->normal + beta * triangleVertexes[outI]->normal;
        newV1.color = alpha * triangleVertexes[(outI + 2) % 3]->color + beta * triangleVertexes[outI]->color;

        Primitive::GPUVertex newV2;
        alpha = dotProduct[outI] / (dotProduct[outI] - dotProduct[(outI + 1) % 3]);
        beta = 1 - alpha;
        newV2.pos = alpha * triangleVertexes[(outI + 1) % 3]->pos + beta * triangleVertexes[outI]->pos;
        newV2.clipSpacePos =
                alpha * triangleVertexes[(outI + 1) % 3]->clipSpacePos + beta * triangleVertexes[outI]->clipSpacePos;
        newV2.viewSpacePos =
                alpha * triangleVertexes[(outI + 1) % 3]->viewSpacePos + beta * triangleVertexes[outI]->viewSpacePos;
        newV2.uv = alpha * triangleVertexes[(outI + 1) % 3]->uv + beta * triangleVertexes[outI]->uv;
        newV2.normal = alpha * triangleVertexes[(outI + 1) % 3]->normal + beta * triangleVertexes[outI]->normal;
        newV2.color = alpha * triangleVertexes[(outI + 1) % 3]->color + beta * triangleVertexes[outI]->color;

        vertexes.emplace_back(std::move(newV1));
        int indexNew1 = (int) vertexes.size() - 1;
        vertexes.emplace_back(std::move(newV2));
        int indexNew2 = (int) vertexes.size() - 1;

        indexes.emplace_back(indexes[indexesI] + (outI + 2) % 3);
        indexes.emplace_back(indexNew1);
        indexes.emplace_back(indexes[indexesI] + (outI + 1) % 3);

        indexes.emplace_back(indexes[indexesI] + (outI + 1) % 3);
        indexes.emplace_back(indexNew1);
        indexes.emplace_back(indexNew2);
    } else if (outCount == 2) {
        int inI = 0;
        while (dotProduct[inI] > 1e-5) ++inI;
        float alpha, beta;

        Primitive::GPUVertex newV1;
        alpha = -dotProduct[inI] / (-dotProduct[inI] + dotProduct[(inI + 2) % 3]);
        beta = 1 - alpha;
        newV1.pos = alpha * triangleVertexes[(inI + 2) % 3]->pos + beta * triangleVertexes[inI]->pos;
        newV1.clipSpacePos =
                alpha * triangleVertexes[(inI + 2) % 3]->clipSpacePos + beta * triangleVertexes[inI]->clipSpacePos;
        newV1.viewSpacePos =
                alpha * triangleVertexes[(inI + 2) % 3]->viewSpacePos + beta * triangleVertexes[inI]->viewSpacePos;
        newV1.uv = alpha * triangleVertexes[(inI + 2) % 3]->uv + beta * triangleVertexes[inI]->uv;
        newV1.normal = alpha * triangleVertexes[(inI + 2) % 3]->normal + beta * triangleVertexes[inI]->normal;
        newV1.color = alpha * triangleVertexes[(inI + 2) % 3]->color + beta * triangleVertexes[inI]->color;

        Primitive::GPUVertex newV2;
        alpha = -dotProduct[inI] / (-dotProduct[inI] + dotProduct[(inI + 1) % 3]);
        beta = 1 - alpha;
        newV2.pos = alpha * triangleVertexes[(inI + 1) % 3]->pos + beta * triangleVertexes[inI]->pos;
        newV2.clipSpacePos =
                alpha * triangleVertexes[(inI + 1) % 3]->clipSpacePos + beta * triangleVertexes[inI]->clipSpacePos;
        newV2.viewSpacePos =
                alpha * triangleVertexes[(inI + 1) % 3]->viewSpacePos + beta * triangleVertexes[inI]->viewSpacePos;
        newV2.ndcSpacePos =
                alpha * triangleVertexes[(inI + 1) % 3]->ndcSpacePos + beta * triangleVertexes[inI]->ndcSpacePos;
        newV2.uv = alpha * triangleVertexes[(inI + 1) % 3]->uv + beta * triangleVertexes[inI]->uv;
        newV2.normal = alpha * triangleVertexes[(inI + 1) % 3]->normal + beta * triangleVertexes[inI]->normal;
        newV2.color = alpha * triangleVertexes[(inI + 1) % 3]->color + beta * triangleVertexes[inI]->color;

        vertexes.emplace_back(std::move(newV1));
        int indexNew1 = (int) vertexes.size() - 1;
        vertexes.emplace_back(std::move(newV2));
        int indexNew2 = (int) vertexes.size() - 1;

        indexes.emplace_back(indexes[indexesI] + inI);
        indexes.emplace_back(indexNew1);
        indexes.emplace_back(indexNew2);
    }
    return false;
}

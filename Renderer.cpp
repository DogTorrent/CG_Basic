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

    Rasterizer rasterizer(screenBuffer, material, payload.fragmentShader);
    for (int indexesI = 0; indexesI < indexes.size(); indexesI += 3) {
        if (!clipTriangle(indexesI)) continue;

        std::array<Primitive::GPUVertex *, 3> triangleVertexes{&vertexes[indexes[indexesI + 0]],
                                                               &vertexes[indexes[indexesI + 1]],
                                                               &vertexes[indexes[indexesI + 2]]};
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
        if (abs((triangleVertexes[0]->screenSpacePos.head(2) - triangleVertexes[1]->screenSpacePos.head(2)).norm()) <
            1e-5
            || abs((triangleVertexes[1]->screenSpacePos.head(2) - triangleVertexes[2]->screenSpacePos.head(2)).norm()) <
               1e-5
            || abs((triangleVertexes[2]->screenSpacePos.head(2) - triangleVertexes[0]->screenSpacePos.head(2)).norm()) <
               1e-5)
            continue;

        RasterizerPayload rasterizerPayload{triangleVertexes, lightList};
        if (renderMode == DEFAULT) rasterizer.rasterizeTriangle(rasterizerPayload);
        else if (renderMode == LINE_ONLY) rasterizer.rasterizeTriangleLine(rasterizerPayload);
    }
}

bool Renderer::clipTriangle(int indexesI) {
    std::deque<Primitive::GPUVertex> verts;
    verts.push_back(vertexes[indexes[indexesI + 0]]);
    verts.push_back(vertexes[indexes[indexesI + 1]]);
    verts.push_back(vertexes[indexes[indexesI + 2]]);
    std::deque<Eigen::Vector4f> paneCoeffs = {
            //near
            {0,  0,  -1, 1},
            //far
            {0,  0,  1,  1},
            //left
            {-1, 0,  0,  1},
            //right
            {1,  0,  0,  1},
            //bottom
            {0,  -1, 0,  1},
            //top
            {0,  1,  0,  1},
    };
    bool allInside = true;
    float offsetRange = 1e-5;
    for (auto &paneCoeff: paneCoeffs) {
        for (auto &currV: verts) {
            if (currV.clipSpacePos.dot(paneCoeff) < -offsetRange) {
                allInside = false;
                break;
            }
        }
        if (!allInside) break;
    }
    if (allInside) return true;

    for (auto &paneCoeff: paneCoeffs) {
        std::deque<Primitive::GPUVertex> newVerts;
        Primitive::GPUVertex *preV = nullptr;
        Primitive::GPUVertex *currV = &(verts.back());
        float preD;
        float currD = currV->clipSpacePos.dot(paneCoeff);
        for (auto &vert: verts) {
            preV = currV;
            preD = currD;
            currV = &vert;
            currD = currV->clipSpacePos.dot(paneCoeff);
            if ((preD >= offsetRange && currD < -offsetRange) || (preD < -offsetRange && currD >= offsetRange)) {
                newVerts.emplace_back(lineLerp(*preV, *currV, preD / (preD - currD)));
            }
            if (currD >= offsetRange) {
                newVerts.push_back(*currV);
            }
        }
        if (newVerts.empty() || newVerts.size() < 3) return false;
        verts = newVerts;
    }

    vertexes.push_back(verts[0]);
    int index0 = (int) vertexes.size() - 1;
    vertexes.push_back(verts[1]);
    int index1 = (int) vertexes.size() - 1;
    vertexes.push_back(verts[2]);
    int index2 = (int) vertexes.size() - 1;
    indexes[indexesI] = index0;
    indexes[indexesI + 1] = index1;
    indexes[indexesI + 2] = index2;
    for (int i = 3; i < verts.size(); ++i) {
        index1 = index2;
        vertexes.push_back(verts[i]);
        index2 = (int) vertexes.size() - 1;
        indexes.push_back(index0);
        indexes.push_back(index1);
        indexes.push_back(index2);
    }
    return true;
}

template<typename T>
T Renderer::lineLerp(T &a1, T &a2, float weight) {
    return weight * a2 + (1 - weight) * a1;
}

Primitive::GPUVertex Renderer::lineLerp(Primitive::GPUVertex &a1, Primitive::GPUVertex &a2, float weight) {
    Primitive::GPUVertex newV;
    newV.pos = lineLerp(a1.pos, a2.pos, weight);
    newV.clipSpacePos = lineLerp(a1.clipSpacePos, a2.clipSpacePos, weight);
    newV.viewSpacePos = lineLerp(a1.viewSpacePos, a2.viewSpacePos, weight);
    newV.uv = lineLerp(a1.uv, a2.uv, weight);
    newV.color = lineLerp(a1.color, a2.color, weight);
    newV.normal = lineLerp(a1.normal, a2.normal, weight);
    return newV;
}

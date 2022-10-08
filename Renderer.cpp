//
// Created by admin on 2022/9/19.
//

#include <iostream>
#include <queue>
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

    // clip
    std::queue<int> disabledTriangleIndexI;
    for (int indexesI = 0; indexesI + 2 < indexes.size(); indexesI += 3) {
        if (!clipTriangle(indexesI)) {
            disabledTriangleIndexI.push(indexesI);
            continue;
        }
    }

    for (auto &vertex: vertexes) {
        if (!vertex.enabled) continue;

        // apply vertex shader
        payload.vertexShader(Shader::VertexShaderPayload{vertex, modelMatrix, viewMatrix, projectionMatrix});

        // Homogeneous division
        // clip_space -> ndc_space
        vertex.pos /= vertex.pos.w();

        // Viewport transformation
        // ndc_space -> screen_space
        // [-1, 1] => [0, width], [-1, 1] => [0, height], [-1, 1] => [0, MAX_DEPTH]
        vertex.pos.x() = 0.5f * (float) screenBuffer.width * (vertex.pos.x() + 1.0f);
        vertex.pos.y() = 0.5f * (float) screenBuffer.height * (vertex.pos.y() + 1.0f);
        vertex.pos.z() = (vertex.pos.z() - cameraObject.nearPaneZ) / (cameraObject.farPaneZ - cameraObject.nearPaneZ);
    }

    Rasterizer rasterizer(screenBuffer, material, payload.fragmentShader);

    for (int indexesI = 0; indexesI + 2 < indexes.size(); indexesI += 3) {
        if (!disabledTriangleIndexI.empty() && indexesI == disabledTriangleIndexI.front()) {
            disabledTriangleIndexI.pop();
            continue;
        }

        std::array<Primitive::GPUVertex *, 3> triangleVertexes{&vertexes[indexes[indexesI]],
                                                               &vertexes[indexes[indexesI + 1]],
                                                               &vertexes[indexes[indexesI + 2]]};
        RasterizerPayload rasterizerPayload{triangleVertexes, lightList};

        if (renderMode == DEFAULT) rasterizer.rasterizeTriangle(rasterizerPayload);
        else if (renderMode == LINE_ONLY) rasterizer.rasterizeTriangleLine(rasterizerPayload);
    }
}

/**
 * clip triangle
 * @param indexesI the index of the first vert of the triangle in the `indexes` array
 * @return should render origin triangle or not
 */
bool Renderer::clipTriangle(int indexesI) {
    std::deque<Eigen::Vector4f> paneCoeffs = {
            //near
            // w_pane = -z, w - w_pane = - w_pane + w = z + w >= 0 -> inside
            {0,  0,  1,  1},
            //far
            // w_pane = z, w - w_pane = - w_pane + w = -z + w >= 0 -> inside
            {0,  0,  -1, 1},
            //left
            // w_pane = -x, w - w_pane = - w_pane + w = x + w >= 0 -> inside
            {1,  0,  0,  1},
            //right
            // w_pane = x, w - w_pane = - w_pane + w = -x + w >= 0 -> inside
            {-1, 0,  0,  1},
            //bottom
            // w_pane = -y, w - w_pane = - w_pane + w = y + w >= 0 -> inside
            {0,  1,  0,  1},
            //top
            // w_pane = y, w - w_pane = - w_pane + w = -y + w >= 0 -> inside
            {0,  -1, 0,  1},
    };

    bool allInside = true;
    for (auto &paneCoeff: paneCoeffs) {
        for (int i = 0; i < 3; ++i) {
            auto &currV = vertexes[indexes[indexesI + i]];
            if (currV.pos.dot(paneCoeff) < 0) {
                allInside = false;
                currV.enabled = false;
            }
        }
    }
    if (allInside) return true;

    std::deque<Primitive::GPUVertex> verts;
    verts.push_back(vertexes[indexes[indexesI + 0]]);
    verts.push_back(vertexes[indexes[indexesI + 1]]);
    verts.push_back(vertexes[indexes[indexesI + 2]]);

    // clip for w_pane = near/far/left/right/bottom/top
    for (auto &paneCoeff: paneCoeffs) {
        std::deque<Primitive::GPUVertex> newVerts;
        Primitive::GPUVertex *preV = nullptr;
        Primitive::GPUVertex *currV = &(verts.back());
        float preD;
        float currD = currV->pos.dot(paneCoeff);
        for (auto &vert: verts) {
            preV = currV;
            preD = currD;
            currV = &vert;
            currD = currV->pos.dot(paneCoeff);
            if ((preD >= 0 && currD < 0) || (preD < 0 && currD >= 0)) {
                newVerts.emplace_back(lineLerp(*preV, *currV, abs(preD) / (abs(preD) + abs(currD))));
                // Manually set the w value to avoid interpolated wi != -w, which may cause infinite loops
                newVerts.back().pos.w() = -newVerts.back().pos.head(3).dot(paneCoeff.head(3));
            }
            if (currD >= 0) {
                newVerts.push_back(*currV);
            }
        }
        if (newVerts.size() < 3) return false;
        verts = newVerts;
    }

    // push new vertexes to the back of the `vertexes` array and push new indexes to the back of the `indexes` array
    vertexes.push_back(verts[0]);
    int index0 = (int) vertexes.size() - 1;
    vertexes.push_back(verts[1]);
    int index1 = (int) vertexes.size() - 1;
    vertexes.push_back(verts[2]);
    int index2 = (int) vertexes.size() - 1;
    indexes.push_back(index0);
    indexes.push_back(index1);
    indexes.push_back(index2);
    for (int i = 3; i < verts.size(); ++i) {
        index1 = index2;
        vertexes.push_back(verts[i]);
        index2 = (int) vertexes.size() - 1;
        indexes.push_back(index0);
        indexes.push_back(index1);
        indexes.push_back(index2);
    }
    return false;
}

template<typename T>
T Renderer::lineLerp(T &a1, T &a2, float weight) {
    return (1 - weight) * a1 + weight * a2;
}

Primitive::GPUVertex Renderer::lineLerp(Primitive::GPUVertex &a1, Primitive::GPUVertex &a2, float weight) {
    Primitive::GPUVertex newV;
    newV.pos = lineLerp(a1.pos, a2.pos, weight);
    newV.viewSpacePos = lineLerp(a1.viewSpacePos, a2.viewSpacePos, weight);
    newV.uv = lineLerp(a1.uv, a2.uv, weight);
    newV.color = lineLerp(a1.color, a2.color, weight);
    newV.normal = lineLerp(a1.normal, a2.normal, weight).normalized();
    return newV;
}

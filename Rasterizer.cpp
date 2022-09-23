//
// Created by admin on 2022/9/23.
//

#include "Rasterizer.h"
#include "Renderer.h"

Rasterizer::Rasterizer(ScreenBuffer &screenBuffer, Primitive::Material &material,
                       std::function<void(Shader::FragmentShaderPayload &)> &fragmentShader) :
        screenBuffer(screenBuffer), material(material), fragmentShader(fragmentShader) {}

void Rasterizer::rasterizeTriangle(const RasterizerPayload &payload) {

    // Find out the AABB bounding box of current triangle but ignore pixels out of screen range
    int left = screenBuffer.width - 1, right = 0;
    int bottom = screenBuffer.height - 1, top = 0;
    for (int i = 0; i < 3; ++i) {
        auto &vPos = payload.triangleVertexes[i]->pos;
        if ((int) floor(vPos.x()) < left) {
            left = floor(vPos.x());
        }
        if ((int) floor(vPos.x()) > right) {
            right = floor(vPos.x());
        }
        if ((int) floor(vPos.y()) < bottom) {
            bottom = floor(vPos.y());
        }
        if ((int) floor(vPos.y()) > top) {
            top = floor(vPos.y());
        }
    }
    if (left < 0) left = 0;
    if (right > screenBuffer.width - 1) right = screenBuffer.width - 1;
    if (bottom < 0) bottom = 0;
    if (top > screenBuffer.height - 1) top = screenBuffer.height - 1;

    for (int x = left; x <= right; ++x) {
        for (int y = bottom; y <= top; ++y) {
            Eigen::Vector3f pointScreenSpacePos((float) x + 0.5f, (float) y + 0.5f, 0);
            if (!checkInsideTriangle(pointScreenSpacePos.x(), pointScreenSpacePos.y(),
                                     payload.triangleVertexes))
                continue;

            // calc barycentric coordinates in screen space
            auto [screenSpaceAlpha, screenSpaceBeta, screenSpaceGamma]
                    = getBarycentricCoordinates(pointScreenSpacePos.x(), pointScreenSpacePos.y(),
                                                payload.triangleVertexes);

            // convert barycentric coordinates from screen space to view space
            auto [viewSpaceAlpha, viewSpaceBeta, viewSpaceGamma]
                    = convertBarycentricCoordinates(screenSpaceAlpha, screenSpaceBeta, screenSpaceGamma,
                                                    payload.vertexesViewSpacePos);

            // interpolate z
            pointScreenSpacePos.z() = viewSpaceAlpha * payload.triangleVertexes[0]->pos.z()
                                      + viewSpaceBeta * payload.triangleVertexes[1]->pos.z()
                                      + viewSpaceGamma * payload.triangleVertexes[2]->pos.z();

            // z test
            if (pointScreenSpacePos.z() >= screenBuffer.valueInDepthBuffer(x, y) ||
                pointScreenSpacePos.z() <= 0)
                continue;

            // z write
            screenBuffer.valueInDepthBuffer(x, y) = pointScreenSpacePos.z();

            // interpolate other
            Eigen::Vector3f color = viewSpaceAlpha * payload.triangleVertexes[0]->color
                                    + viewSpaceBeta * payload.triangleVertexes[1]->color
                                    + viewSpaceGamma * payload.triangleVertexes[2]->color;
            Eigen::Vector3f normal = viewSpaceAlpha * payload.triangleVertexes[0]->normal
                                     + viewSpaceBeta * payload.triangleVertexes[1]->normal
                                     + viewSpaceGamma * payload.triangleVertexes[2]->normal;
            Eigen::Vector2f uv = viewSpaceAlpha * payload.triangleVertexes[0]->uv
                                 + viewSpaceBeta * payload.triangleVertexes[1]->uv
                                 + viewSpaceGamma * payload.triangleVertexes[2]->uv;
            Eigen::Vector3f viewSpacePos = viewSpaceAlpha * payload.vertexesViewSpacePos[0]
                                           + viewSpaceBeta * payload.vertexesViewSpacePos[1]
                                           + viewSpaceGamma * payload.vertexesViewSpacePos[2];

            // apply fragment shader
            Shader::FragmentShaderPayload fragmentShaderPayload{viewSpacePos, color, normal, uv,
                                                                payload.lightList,
                                                                material};
            Shader::basicFragmentShader(fragmentShaderPayload);
            fragmentShader(fragmentShaderPayload);

            screenBuffer.valueInFrameBuffer(x, y) = fragmentShaderPayload.color;
        }
    }
}

void Rasterizer::rasterizeTriangleLine(const RasterizerPayload &payload) {
    for (int i = 0; i < 3; ++i) {
        Eigen::Vector2f p1(payload.triangleVertexes[i]->pos.x(), payload.triangleVertexes[i]->pos.y());
        Eigen::Vector2f p2(payload.triangleVertexes[(i + 1) % 3]->pos.x(),
                           payload.triangleVertexes[(i + 1) % 3]->pos.y());
        float deltaY = p2.y() - p1.y();
        float deltaX = p2.x() - p1.x();
        bool isKLessThanOne = abs(deltaY) < abs(deltaX);

        float ddx;
        float ddy;
        int steps;
        Eigen::Vector2f p;

        if (isKLessThanOne) {
            if (p1.x() > p2.x()) {
                std::swap(p1, p2);
                deltaX = -deltaX;
                deltaY = -deltaY;
            }
            ddx = 1;
            ddy = deltaY / deltaX;
            p.x() = round(p1.x()) + 0.5f;
            p.y() = p1.y() + (p.x() - p1.x()) * ddy;
            steps = (int) round(p2.x()) - (int) round(p1.x()) + 1;
        } else {
            if (p1.y() > p2.y()) {
                std::swap(p1, p2);
                deltaX = -deltaX;
                deltaY = -deltaY;
            }
            ddy = 1;
            ddx = deltaX / deltaY;
            p.y() = round(p1.y()) + 0.5f;
            p.x() = p1.x() + (p.y() - p1.y()) * ddx;
            steps = (int) round(p2.y()) - (int) round(p1.y()) + 1;
        }

        for (int step = 0; step < steps; ++step) {
            int pixelX = floor(p.x());
            int pixelY = floor(p.y());
            Eigen::Vector3f pointScreenSpacePos(p.x(), p.y(), 0);

            // calc barycentric coordinates in screen space
            auto [screenSpaceAlpha, screenSpaceBeta, screenSpaceGamma]
                    = getBarycentricCoordinates(pointScreenSpacePos.x(), pointScreenSpacePos.y(),
                                                payload.triangleVertexes);

            // convert barycentric coordinates from screen space to view space
            auto [viewSpaceAlpha, viewSpaceBeta, viewSpaceGamma]
                    = convertBarycentricCoordinates(screenSpaceAlpha, screenSpaceBeta, screenSpaceGamma,
                                                    payload.vertexesViewSpacePos);

            // interpolate z
            pointScreenSpacePos.z() = viewSpaceAlpha * payload.triangleVertexes[0]->pos.z()
                                      + viewSpaceBeta * payload.triangleVertexes[1]->pos.z()
                                      + viewSpaceGamma * payload.triangleVertexes[2]->pos.z();

            // z test
            if (pointScreenSpacePos.z() >= screenBuffer.valueInDepthBuffer(pixelX, pixelY) ||
                pointScreenSpacePos.z() < 0)
                continue;

            // z write
            screenBuffer.valueInDepthBuffer(pixelX, pixelY) = pointScreenSpacePos.z();

            // interpolate other
            Eigen::Vector3f color = viewSpaceAlpha * payload.triangleVertexes[0]->color
                                    + viewSpaceBeta * payload.triangleVertexes[1]->color
                                    + viewSpaceGamma * payload.triangleVertexes[2]->color;
            Eigen::Vector3f normal = viewSpaceAlpha * payload.triangleVertexes[0]->normal
                                     + viewSpaceBeta * payload.triangleVertexes[1]->normal
                                     + viewSpaceGamma * payload.triangleVertexes[2]->normal;
            Eigen::Vector2f uv = viewSpaceAlpha * payload.triangleVertexes[0]->uv
                                 + viewSpaceBeta * payload.triangleVertexes[1]->uv
                                 + viewSpaceGamma * payload.triangleVertexes[2]->uv;
            Eigen::Vector3f viewSpacePos = viewSpaceAlpha * payload.vertexesViewSpacePos[0]
                                           + viewSpaceBeta * payload.vertexesViewSpacePos[1]
                                           + viewSpaceGamma * payload.vertexesViewSpacePos[2];

            // apply fragment shader
            Shader::FragmentShaderPayload fragmentShaderPayload{viewSpacePos, color, normal, uv,
                                                                payload.lightList,
                                                                material};
            Shader::basicFragmentShader(fragmentShaderPayload);
            fragmentShader(fragmentShaderPayload);

            screenBuffer.valueInFrameBuffer(pixelX, pixelY) = fragmentShaderPayload.color;

            p.x() += ddx;
            p.y() += ddy;
        }
    }
}

bool Rasterizer::checkInsideTriangle(float posX, float posY, std::array<Primitive::Vertex *, 3> &triangleVertexes) {
    std::array<Eigen::Vector3f, 3> vec;
    std::array<Eigen::Vector3f, 3> vec_p;
    std::array<float, 3> crossZ{};
    for (int i = 0; i < 3; ++i) {
        vec[i] = {triangleVertexes[i]->pos.x() - posX, triangleVertexes[i]->pos.y() - posY, 0};
        vec_p[i] = {triangleVertexes[i]->pos.x() - triangleVertexes[(i + 1) % 3]->pos.x(),
                    triangleVertexes[i]->pos.y() - triangleVertexes[(i + 1) % 3]->pos.y(), 0};
        crossZ[i] = vec[i].cross(vec_p[i]).z();
    }
    if ((crossZ[0] >= 0 && crossZ[1] >= 0 && crossZ[2] >= 0) ||
        (crossZ[0] <= 0 && crossZ[1] <= 0 && crossZ[2] <= 0))
        return true;
    return false;
}

std::array<float, 3>
Rasterizer::getBarycentricCoordinates(float x, float y, std::array<Primitive::Vertex *, 3> &triangleVertexes) {
    Eigen::Vector2f AminusB = {triangleVertexes[0]->pos.x() - triangleVertexes[1]->pos.x(),
                               triangleVertexes[0]->pos.y() - triangleVertexes[1]->pos.y()};
    Eigen::Vector2f BminusC = {triangleVertexes[1]->pos.x() - triangleVertexes[2]->pos.x(),
                               triangleVertexes[1]->pos.y() - triangleVertexes[2]->pos.y()};
    Eigen::Vector2f CminusA = {triangleVertexes[2]->pos.x() - triangleVertexes[0]->pos.x(),
                               triangleVertexes[2]->pos.y() - triangleVertexes[0]->pos.y()};
    float alpha = ((x - triangleVertexes[1]->pos.x()) * BminusC.y() - (y - triangleVertexes[1]->pos.y()) * BminusC.x())
                  / (AminusB.x() * BminusC.y() - AminusB.y() * BminusC.x());
    float beta = ((x - triangleVertexes[2]->pos.x()) * CminusA.y() - (y - triangleVertexes[2]->pos.y()) * CminusA.x())
                 / (BminusC.x() * CminusA.y() - BminusC.y() * CminusA.x());
    float gamma = 1 - alpha - beta;
    return {alpha, beta, gamma};
}

std::array<float, 3>
Rasterizer::convertBarycentricCoordinates(float screenSpaceAlpha, float screenSpaceBeta, float screenSpaceGamma,
                                          std::array<Eigen::Vector3f, 3> &vertexesViewSpacePos) {

    // correct z
    float viewSpaceZ = 1 / (screenSpaceAlpha / vertexesViewSpacePos[0].z()
                            + screenSpaceBeta / vertexesViewSpacePos[1].z()
                            + screenSpaceGamma / vertexesViewSpacePos[2].z());
    // convert barycentric coordinates to view space
    float viewSpaceAlpha = screenSpaceAlpha * viewSpaceZ / vertexesViewSpacePos[0].z();
    float viewSpaceBeta = screenSpaceBeta * viewSpaceZ / vertexesViewSpacePos[1].z();
    float viewSpaceGamma = screenSpaceGamma * viewSpaceZ / vertexesViewSpacePos[2].z();
    return {viewSpaceAlpha, viewSpaceBeta, viewSpaceGamma};
}

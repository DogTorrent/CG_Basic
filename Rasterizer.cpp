//
// Created by admin on 2022/9/23.
//

#include "Rasterizer.h"
#include "ScreenBuffer.h"
#include "Renderer.h"

Rasterizer::Rasterizer(ScreenBuffer &screenBuffer, Primitive::Material &material,
                       std::function<void(const Shader::FragmentShaderPayload &)> &fragmentShader) :
        screenBuffer(screenBuffer), material(material), fragmentShader(fragmentShader) {}

void Rasterizer::rasterizeTriangle(const RasterizerPayload &payload) {
    std::vector<Eigen::Vector2f> scanTrianglePos;
    scanTrianglePos.reserve(3);
    for (auto item: payload.triangleVertexes) {
        scanTrianglePos.emplace_back(item->screenSpacePos.x(), item->screenSpacePos.y());
    }
    std::sort(scanTrianglePos.begin(), scanTrianglePos.end(),
              [](Eigen::Vector2f &a, Eigen::Vector2f &b) -> bool {
                  if (a.y() == b.y()) return a.x() < b.x();
                  return a.y() < b.y();
              });
    if (scanTrianglePos[0].y() == scanTrianglePos[1].y()) {
        // flat bottom
    } else if (scanTrianglePos[1].y() == scanTrianglePos[2].y()) {
        // flat top
        std::swap(scanTrianglePos[0], scanTrianglePos[1]);
        std::swap(scanTrianglePos[1], scanTrianglePos[2]);
    } else {
        float x;
        if (scanTrianglePos[2].y() == scanTrianglePos[0].y()) {
            x = scanTrianglePos[0].x();
        } else {
            x = (scanTrianglePos[1].y() - scanTrianglePos[0].y())
                / (scanTrianglePos[2].y() - scanTrianglePos[0].y())
                * (scanTrianglePos[2].x() - scanTrianglePos[0].x()) +
                scanTrianglePos[0].x();
        }
        scanTrianglePos = {
                // flat bottom
                {scanTrianglePos[1].x(), scanTrianglePos[1].y()},
                {x,                      scanTrianglePos[1].y()},
                {scanTrianglePos[2].x(), scanTrianglePos[2].y()},
                // flat top
                {scanTrianglePos[1].x(), scanTrianglePos[1].y()},
                {x,                      scanTrianglePos[1].y()},
                {scanTrianglePos[0].x(), scanTrianglePos[0].y()},
        };

        if (scanTrianglePos[0].x() > scanTrianglePos[1].x()) std::swap(scanTrianglePos[0], scanTrianglePos[1]);
        if (scanTrianglePos[3].x() > scanTrianglePos[4].x()) std::swap(scanTrianglePos[3], scanTrianglePos[4]);
    }
    //  2     0 1
    // 0 1 or  2
    for (int i = 0; i < scanTrianglePos.size(); i += 3) {
        int flatSideY = floor(scanTrianglePos[i].y());
        int vertexSideY = floor(scanTrianglePos[i + 2].y());
        int deltaY = (flatSideY <= vertexSideY) ? 1 : -1;
        int left = floor(MIN(scanTrianglePos[i].x(), scanTrianglePos[i + 2].x()));
        int right = floor(MAX(scanTrianglePos[i + 1].x(), scanTrianglePos[i + 2].x()));

        float dStartXdy, dEndXdy;
        if (flatSideY != vertexSideY) {
            dStartXdy = (scanTrianglePos[i + 2].x() - scanTrianglePos[i].x()) /
                        (scanTrianglePos[i + 2].y() - scanTrianglePos[i].y());
            dEndXdy = (scanTrianglePos[i + 2].x() - scanTrianglePos[i + 1].x()) /
                      (scanTrianglePos[i + 2].y() - scanTrianglePos[i + 1].y());
        }

        for (int y = flatSideY; y != vertexSideY + deltaY; y += deltaY) {
            int startX = left, endX = right;
            if (flatSideY != vertexSideY) {
                startX = floor(((float) y + 0.5f - scanTrianglePos[i].y()) * dStartXdy + scanTrianglePos[i].x());
                endX = floor(((float) y + 0.5f - scanTrianglePos[i + 1].y()) * dEndXdy + scanTrianglePos[i + 1].x());
            }
            if (startX < left) startX = left;
            if (endX > right) endX = right;
            while (startX <= right &&
                   !checkInsideTriangle((float) startX + 0.5f, (float) y + 0.5f, payload.triangleVertexes))
                ++startX;
            while (endX >= left &&
                   !checkInsideTriangle((float) endX + 0.5f, (float) y + 0.5f, payload.triangleVertexes))
                --endX;
            for (int x = startX; x <= endX; ++x) {
                Eigen::Vector3f pointScreenSpacePos((float) x + 0.5f, (float) y + 0.5f, 0);
                drawScreenSpacePoint(pointScreenSpacePos, payload);
            }
        }
    }
}

void Rasterizer::rasterizeTriangleLine(const RasterizerPayload &payload) {
    for (int i = 0; i < 3; ++i) {
        Eigen::Vector2f p1(payload.triangleVertexes[i]->screenSpacePos.x(),
                           payload.triangleVertexes[i]->screenSpacePos.y());
        Eigen::Vector2f p2(payload.triangleVertexes[(i + 1) % 3]->screenSpacePos.x(),
                           payload.triangleVertexes[(i + 1) % 3]->screenSpacePos.y());
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
            Eigen::Vector3f pointScreenSpacePos(p.x(), p.y(), 0);
            drawScreenSpacePoint(pointScreenSpacePos, payload);

            p.x() += ddx;
            p.y() += ddy;
        }
    }
}

bool Rasterizer::checkInsideTriangle(float posX, float posY, std::array<Primitive::GPUVertex *, 3> &triangleVertexes) {
    std::array<Eigen::Vector3f, 3> vec;
    std::array<Eigen::Vector3f, 3> vec_p;
    std::array<float, 3> crossZ{};
    for (int i = 0; i < 3; ++i) {
        vec[i] = {triangleVertexes[i]->screenSpacePos.x() - posX, triangleVertexes[i]->screenSpacePos.y() - posY, 0};
        vec_p[i] = {triangleVertexes[i]->screenSpacePos.x() - triangleVertexes[(i + 1) % 3]->screenSpacePos.x(),
                    triangleVertexes[i]->screenSpacePos.y() - triangleVertexes[(i + 1) % 3]->screenSpacePos.y(), 0};
        crossZ[i] = vec[i].cross(vec_p[i]).z();
    }
    if ((crossZ[0] >= 0 && crossZ[1] >= 0 && crossZ[2] >= 0) ||
        (crossZ[0] <= 0 && crossZ[1] <= 0 && crossZ[2] <= 0))
        return true;
    return false;
}

std::array<float, 3>
Rasterizer::getBarycentricCoordinates(float x, float y, std::array<Primitive::GPUVertex *, 3> &triangleVertexes) {
    Eigen::Vector2f AminusB = (triangleVertexes[0]->screenSpacePos - triangleVertexes[1]->screenSpacePos).head(2);
    Eigen::Vector2f BminusC = (triangleVertexes[1]->screenSpacePos - triangleVertexes[2]->screenSpacePos).head(2);
    Eigen::Vector2f CminusA = (triangleVertexes[2]->screenSpacePos - triangleVertexes[0]->screenSpacePos).head(2);
    Eigen::Vector2f PminusA = Eigen::Vector2f(x, y) - triangleVertexes[0]->screenSpacePos.head(2);
    Eigen::Vector2f PminusB = Eigen::Vector2f(x, y) - triangleVertexes[1]->screenSpacePos.head(2);
    Eigen::Vector2f PminusC = Eigen::Vector2f(x, y) - triangleVertexes[2]->screenSpacePos.head(2);
    float alpha, beta, gamma;
    alpha = (PminusB.x() * BminusC.y() - PminusB.y() * BminusC.x())
            / (AminusB.x() * BminusC.y() - AminusB.y() * BminusC.x());
    beta = (PminusC.x() * CminusA.y() - PminusC.y() * CminusA.x())
           / (BminusC.x() * CminusA.y() - BminusC.y() * CminusA.x());
    gamma = 1 - alpha - beta;
    return {alpha, beta, gamma};
}

std::array<float, 3>
Rasterizer::convertBarycentricCoordinates(float screenSpaceAlpha, float screenSpaceBeta, float screenSpaceGamma,
                                          std::array<Primitive::GPUVertex *, 3> &triangleVertexes) {

    // correct z
    float viewSpaceZ = 1 / (screenSpaceAlpha / triangleVertexes[0]->viewSpacePos.z()
                            + screenSpaceBeta / triangleVertexes[1]->viewSpacePos.z()
                            + screenSpaceGamma / triangleVertexes[2]->viewSpacePos.z());
    // convert barycentric coordinates to view space
    float viewSpaceAlpha = screenSpaceAlpha * viewSpaceZ / triangleVertexes[0]->viewSpacePos.z();
    float viewSpaceBeta = screenSpaceBeta * viewSpaceZ / triangleVertexes[1]->viewSpacePos.z();
    float viewSpaceGamma = screenSpaceGamma * viewSpaceZ / triangleVertexes[2]->viewSpacePos.z();
    return {viewSpaceAlpha, viewSpaceBeta, viewSpaceGamma};
}

void Rasterizer::drawScreenSpacePoint(Eigen::Vector3f &pointScreenSpacePos, const RasterizerPayload &payload) {
    int pixelX = floor(pointScreenSpacePos.x());
    int pixelY = floor(pointScreenSpacePos.y());

    // clip out of range
    if (pixelX < 0 || pixelX >= screenBuffer.width || pixelY < 0 || pixelY >= screenBuffer.height) return;

    // calc barycentric coordinates in screen space
    auto [screenSpaceAlpha, screenSpaceBeta, screenSpaceGamma]
            = getBarycentricCoordinates(pointScreenSpacePos.x(), pointScreenSpacePos.y(),
                                        payload.triangleVertexes);
    if (_isnanf(screenSpaceAlpha) || _isnanf(screenSpaceBeta) || _isnanf(screenSpaceGamma)) return;
    if (isinf(screenSpaceAlpha) || isinf(screenSpaceBeta) || isinf(screenSpaceGamma)) return;

    // convert barycentric coordinates from screen space to view space
    auto [viewSpaceAlpha, viewSpaceBeta, viewSpaceGamma]
            = convertBarycentricCoordinates(screenSpaceAlpha, screenSpaceBeta, screenSpaceGamma,
                                            payload.triangleVertexes);

    // interpolate z
    pointScreenSpacePos.z() = viewSpaceAlpha * payload.triangleVertexes[0]->screenSpacePos.z()
                              + viewSpaceBeta * payload.triangleVertexes[1]->screenSpacePos.z()
                              + viewSpaceGamma * payload.triangleVertexes[2]->screenSpacePos.z();

    // clip out of range
    if (pointScreenSpacePos.z() < 0 || pointScreenSpacePos.z() > MAX_DEPTH) return;

    // z test
    if (pointScreenSpacePos.z() >= screenBuffer.valueInDepthBuffer(pixelX, pixelY))
        return;

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
    Eigen::Vector3f viewSpacePos = viewSpaceAlpha * payload.triangleVertexes[0]->viewSpacePos.head(3)
                                   + viewSpaceBeta * payload.triangleVertexes[1]->viewSpacePos.head(3)
                                   + viewSpaceGamma * payload.triangleVertexes[2]->viewSpacePos.head(3);

    // apply fragment shader
    Shader::FragmentShaderPayload fragmentShaderPayload{viewSpacePos, color, normal, uv,
                                                        payload.lightList,
                                                        material};
    Shader::basicFragmentShader(fragmentShaderPayload);
    fragmentShader(fragmentShaderPayload);

    screenBuffer.valueInFrameBuffer(pixelX, pixelY) = fragmentShaderPayload.color;
}

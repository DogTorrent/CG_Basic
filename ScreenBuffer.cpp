//
// Created by admin on 2022/9/23.
//

#include "ScreenBuffer.h"

ScreenBuffer::ScreenBuffer(int width, int height) {
    this->width = width;
    this->height = height;
    frameBuffer.resize(width * height);
    depthBuffer.resize(width * height, MAX_DEPTH);
}

int ScreenBuffer::getIndex(int x, int y) const {
    // opencv use top left corner as (0,0), while unity(which this pipeline is simulating) using bottom left corner as (0,0)
    return (height - 1 - y) * width + x;
}

void ScreenBuffer::clearBuffer() {
    for (auto &pixel: frameBuffer) pixel.setZero();
    std::fill(depthBuffer.begin(), depthBuffer.end(), MAX_DEPTH);
}

Eigen::Vector3f &ScreenBuffer::valueInFrameBuffer(int x, int y) {
    return frameBuffer[getIndex(x, y)];
}

float &ScreenBuffer::valueInDepthBuffer(int x, int y) {
    return depthBuffer[getIndex(x, y)];
}

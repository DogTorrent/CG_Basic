//
// Created by admin on 2022/9/20.
//

#include "Primitive.h"

Primitive::Vertex::Vertex(Eigen::Vector4f pos, Eigen::Vector3f color, Eigen::Vector2f uv,
                          Eigen::Vector3f normal) : pos(std::move(pos)), color(std::move(color)), uv(std::move(uv)),
                                                    normal(std::move(normal)) {
}

Primitive::Texture::Texture(const std::string &name) {
    image_data = cv::imread(name);
    cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
}

Eigen::Vector3f Primitive::Texture::getValue(float u, float v) {
    if (image_data.empty()) return {1, 1, 1};
    if (u < 0) u = 0;
    else if (u > 1) u = 1;
    if (v < 0) v = 0;
    else if (v > 1) v = 1;
    auto imgU = u * (float) (image_data.cols - 1);
    auto imgV = (1.f - v) * (float) (image_data.rows - 1);
    auto color = image_data.at<cv::Vec3b>((int) imgV, (int) imgU);
    Eigen::Vector3f res(color[0], color[1], color[2]);
    return res;
}

void Primitive::Texture::cleanData() {
    image_data.release();
}

void Primitive::Texture::copyTo(Primitive::Texture &dest) {
    image_data.copyTo(dest.image_data);
}

bool Primitive::Texture::isEmpty() {
    return image_data.empty();
}

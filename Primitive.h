//
// Created by admin on 2022/9/20.
//

#ifndef CG_BASIC_PRIMITIVE_H
#define CG_BASIC_PRIMITIVE_H

#include <array>
#include <eigen3/Eigen/Core>
#include <deque>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>

namespace Primitive {

    class Vertex {
    public:
        Vertex(Eigen::Vector4f pos, Eigen::Vector3f color, Eigen::Vector2f uv,
               Eigen::Vector3f normal);

        Vertex() = default;

    public:
        Eigen::Vector4f pos;
        Eigen::Vector3f color;
        Eigen::Vector2f uv;
        Eigen::Vector3f normal;
    };

    class GPUVertex : public Vertex {
    public:
        Eigen::Vector4f viewSpacePos;
        Eigen::Vector4f clipSpacePos;
        Eigen::Vector4f ndcSpacePos;
        Eigen::Vector3f screenSpacePos;

        explicit GPUVertex(const Vertex &vertex);

        GPUVertex() = default;
    };

    class Light {
    public:
        Eigen::Vector3f pos;
        Eigen::Vector3f intensity;
    };

    class Texture {
    private:
        cv::Mat image_data;
    public:

        explicit Texture(const std::string &name);

        Texture() = default;

        void copyTo(Texture &dest);

        Eigen::Vector3f getValue(float u, float v);

        void cleanData();

        bool isEmpty();
    };

    class Material {
    public:
        Eigen::Vector3f ka; // Ambient Color
        Eigen::Vector3f kd; // Diffuse Color
        Eigen::Vector3f ks; // Specular Color
        float ns; // Specular Exponent
        Texture diffuseTexture;
    };

    class Mesh {
    public:
        std::deque<Vertex> vertexes;
        std::deque<uint> indexes;
    };

    class Geometry {
    public:
        Mesh mesh;
        Material material;
    };
};


#endif //CG_BASIC_PRIMITIVE_H

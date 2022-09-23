//
// Created by admin on 2022/9/19.
//

#ifndef CG_BASIC_TRANSFORMMATRIX_H
#define CG_BASIC_TRANSFORMMATRIX_H

#include <eigen3/Eigen/Eigen>

class TransformMatrix {
public:
    static Eigen::Matrix4f
    getModelMatrix(const Eigen::Matrix4f &scalingMatrix, const Eigen::Matrix4f &rotationMatrix, const Eigen::Matrix4f &movingMatrix);

    static Eigen::Matrix4f
    getViewMatrix(const Eigen::Vector4f &cameraPos, const Eigen::Vector4f &cameraToward, const Eigen::Vector4f &cameraTop);

    static Eigen::Matrix4f getProjectionMatrix(float FoV, float aspectRatio, float zNear, float zFar);

    static Eigen::Matrix4f getScalingMatrix(const Eigen::Vector3f &scalingRatio);
    static Eigen::Matrix4f getRotationMatrix(const Eigen::Vector4f& axis, float angleDegree);
    static Eigen::Matrix4f getMovingMatrix(const Eigen::Vector4f &pos);
};


#endif //CG_BASIC_TRANSFORMMATRIX_H

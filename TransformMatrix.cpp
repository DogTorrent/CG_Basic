//
// Created by admin on 2022/9/19.
//

#include "TransformMatrix.h"

Eigen::Matrix4f
TransformMatrix::getModelMatrix(const Eigen::Matrix4f &scalingMatrix, const Eigen::Matrix4f &rotationMatrix,
                                const Eigen::Matrix4f &movingMatrix) {
    return movingMatrix * scalingMatrix * rotationMatrix;
}

Eigen::Matrix4f
TransformMatrix::getViewMatrix(const Eigen::Vector4f &cameraPos, const Eigen::Vector4f &cameraToward,
                               const Eigen::Vector4f &cameraTop) {
    // move camera to origin(actually is origin to camera)
    Eigen::Matrix4f move;
    move << 1, 0, 0, -cameraPos.x(),
            0, 1, 0, -cameraPos.y(),
            0, 0, 1, -cameraPos.z(),
            0, 0, 0, 1;

    // rotate camera to standard 3d-axis
    // cameraX => x-axis (1,0,0) [same as cameraTop cross cameraToward -> y-axis cross z-axis]
    // cameraTop => y-axis (0,1,0)
    // cameraToward => z-axis (0,0,1)
    //
    // reverse type is:
    //    x-axis (1,0,0) => cameraX
    //    y-axis (0,1,0) => cameraTop
    //    z-axis (0,0,1) => cameraToward
    //    reverse_rotate << cameraX.x(),cameraTop.x(),cameraToward.x(),0
    //              cameraX.y(),cameraTop.y(),cameraToward.y(),0
    //              cameraX.z(),cameraTop.z(),cameraToward.z(),0
    //              0,0,0,1;
    //
    // so the not reverse type is reverse_rotate^(-1) = reverse_rotate^T
    Eigen::Vector4f cameraX = cameraTop.cross3(cameraToward);
    Eigen::Matrix4f rotate;
    rotate << cameraX.x(), cameraX.y(), cameraX.z(), 0,
            cameraTop.x(), cameraTop.y(), cameraTop.z(), 0,
            cameraToward.x(), cameraToward.y(), cameraToward.z(), 0,
            0, 0, 0, 1;

    return rotate * move;
}

Eigen::Matrix4f TransformMatrix::getProjectionMatrix(float FoV, float aspectRatio, float near, float far) {
    // top and right in the near pane
    auto top = (float) abs(near * tan(FoV / 2 / 180 * EIGEN_PI));
    float right = top * aspectRatio;
    // z-pane:
    // point => persp:
    // (r,t,n,1) => (n, n, -n, n)
    // (r,-t,n,1) => (n, -n, -n, n)
    // (-r,t,n,1) => (-n, n, -n, n)
    // (r,t,f,1) => (f, f, f, f)
    // => perspective matrix = n/r,0,0,0,
    //                         0,n/t,0,0,
    //                         ?,?,?,?
    //                         0,0,1,0
    //
    // (0,0,n,1) => (0,0,-n,n)
    // (0,0,f,1) => (0,0,f,f)
    // => perspective matrix = n/r, 0, 0, 0,
    //                         0, n/t, 0, 0,
    //                         0, 0, (f+n)/(f-n), -2fn/(f-n)
    //                         0, 0, 1, 0

    Eigen::Matrix4f perspective;
    perspective << near / right, 0, 0, 0,
            0, near / top, 0, 0,
            0, 0, (far + near) / (far - near), -2 * far * near / (far - near),
            0, 0, 1, 0;

    return perspective;
}

Eigen::Matrix4f TransformMatrix::getScalingMatrix(const Eigen::Vector3f &scalingRatio) {
    Eigen::Matrix4f scale;
    scale << scalingRatio.x(), 0, 0, 0,
            0, scalingRatio.y(), 0, 0,
            0, 0, scalingRatio.z(), 0,
            0, 0, 0, 1;
    return scale;
}

// The value of angle of clockwise rotation is positive
Eigen::Matrix4f TransformMatrix::getRotationMatrix(const Eigen::Vector4f &axis, float angleDegree) {
    Eigen::Matrix4f rotate;
    auto radian = (float) (angleDegree / 180 * EIGEN_PI);
    float cosTheta = cos(radian);
    float sinTheta = sin(radian);
    rotate << axis.x() * axis.x() * (1 - cosTheta) + cosTheta,
            axis.x() * axis.y() * (1 - cosTheta) - axis.z() * sinTheta,
            axis.x() * axis.z() * (1 - cosTheta) + axis.y() * sinTheta,
            0,
            axis.y() * axis.x() * (1 - cosTheta) + axis.z() * sinTheta,
            axis.y() * axis.y() * (1 - cosTheta) + cosTheta,
            axis.y() * axis.z() * (1 - cosTheta) - axis.x() * sinTheta,
            0,
            axis.z() * axis.x() * (1 - cosTheta) - axis.y() * sinTheta,
            axis.z() * axis.y() * (1 - cosTheta) + axis.x() * sinTheta,
            axis.z() * axis.z() * (1 - cosTheta) + cosTheta,
            0,
            0, 0, 0, 1;
    return rotate;
}

Eigen::Matrix4f TransformMatrix::getMovingMatrix(const Eigen::Vector4f &pos) {
    Eigen::Matrix4f move;
    move << 1, 0, 0, pos.x(),
            0, 1, 0, pos.y(),
            0, 0, 1, pos.z(),
            0, 0, 0, 1;
    return move;
}


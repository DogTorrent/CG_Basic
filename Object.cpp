//
// Created by admin on 2022/9/28.
//

#include "Object.h"
#include "TransformMatrix.h"

void CameraObject::rotate(const Eigen::Vector4f &axis, float angleDegree) {
    Eigen::Matrix4f rotationMatrix = TransformMatrix::getRotationMatrix(axis, angleDegree);
    toward = (rotationMatrix * toward).normalized();
    top = (rotationMatrix * top).normalized();
}

void CameraObject::moveRight(float delta) {
    Eigen::Vector4f xAxis = top.cross3(toward);
    pos += xAxis * delta;
}

void CameraObject::moveUp(float delta) {
    pos += top * delta;
}

void CameraObject::moveForward(float delta) {
    pos += toward * delta;
}

//
// Created by .torrent on 2022/9/26.
//

#ifndef CG_BASIC_SCENE_H
#define CG_BASIC_SCENE_H


#include <deque>
#include <eigen3/Eigen/Core>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "Primitive.h"
#include "Shader.h"

class Renderer;

class ScreenBuffer;

class SceneObject {
public:
    std::deque<Primitive::Geometry> geometryList;
    Eigen::Vector3f scalingRatio;
    Eigen::Vector4f rotationAxis;
    float rotationDegree = 0;
    Eigen::Vector4f modelPos;
    Eigen::Vector4f cameraPos;
    Eigen::Vector4f cameraToward;
    Eigen::Vector4f cameraTop;
    float FoV = 45;
    float aspectRatio = 1;
    float nearPaneZ = 1;
    float farPaneZ = 50;
    std::deque<Primitive::Light> lightList;
    std::function<void(Shader::VertexShaderPayload &)> vertexShader;
    std::function<void(Shader::FragmentShaderPayload &)> fragmentShader;

    void draw(Renderer &renderer);
};

class Scene {
public:
    ScreenBuffer *screenBuffer = nullptr;
    std::deque<SceneObject *> pSceneObjectList;

    void draw();
};


#endif //CG_BASIC_SCENE_H

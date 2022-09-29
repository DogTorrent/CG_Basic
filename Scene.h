//
// Created by .torrent on 2022/9/26.
//

#ifndef CG_BASIC_SCENE_H
#define CG_BASIC_SCENE_H


#include <deque>
#include "Primitive.h"
#include "Shader.h"
class ScreenBuffer;
class SceneObject;
class CameraObject;

class Scene {
public:
    ScreenBuffer *screenBuffer = nullptr;
    std::deque<SceneObject *> pSceneObjectList;
    CameraObject *cameraObject;
    std::deque<Primitive::Light> lightList;

    void draw();
};


#endif //CG_BASIC_SCENE_H

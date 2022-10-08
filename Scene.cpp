//
// Created by .torrent on 2022/9/26.
//

#include "Scene.h"
#include "TransformMatrix.h"
#include "Renderer.h"
#include "ScreenBuffer.h"
#include "Object.h"

void Scene::draw() {
    if (!screenBuffer || !cameraObject) return;
    screenBuffer->clearBuffer();

    Renderer renderer(*screenBuffer, *cameraObject);
    renderer.viewMatrix = TransformMatrix::getViewMatrix(cameraObject->pos,
                                                         cameraObject->toward,
                                                         cameraObject->top);
    renderer.projectionMatrix = TransformMatrix::getProjectionMatrix(cameraObject->FoV,
                                                                     cameraObject->aspectRatio,
                                                                     cameraObject->nearPaneZ,
                                                                     cameraObject->farPaneZ);
    for (auto pSceneObject: pSceneObjectList) {
        renderer.modelMatrix = TransformMatrix::getModelMatrix(
                TransformMatrix::getScalingMatrix(pSceneObject->scalingRatio),
                TransformMatrix::getRotationMatrix(pSceneObject->rotationAxis, pSceneObject->rotationDegree),
                TransformMatrix::getMovingMatrix(pSceneObject->modelPos));
        renderer.renderOption = pSceneObject->renderOption;

        for (auto &geometry: pSceneObject->geometryList) {
            RendererPayload rendererPayload{geometry, pSceneObject->vertexShader, pSceneObject->fragmentShader,
                                            lightList};
            renderer.renderGeometry(rendererPayload);
        }
    }
}

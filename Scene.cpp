//
// Created by .torrent on 2022/9/26.
//

#include "Scene.h"
#include "TransformMatrix.h"
#include "Renderer.h"
#include "ScreenBuffer.h"

void SceneObject::draw(Renderer &renderer) {
    renderer.modelMatrix = TransformMatrix::getModelMatrix(TransformMatrix::getScalingMatrix(scalingRatio),
                                                           TransformMatrix::getRotationMatrix(
                                                                   rotationAxis, rotationDegree),
                                                           TransformMatrix::getMovingMatrix(modelPos));
    renderer.viewMatrix = TransformMatrix::getViewMatrix(cameraPos, cameraToward, cameraTop);
    renderer.projectionMatrix = TransformMatrix::getProjectionMatrix(FoV, aspectRatio, nearPaneZ, farPaneZ);
    for (auto &geometry: geometryList) {
        RendererPayload rendererPayload{geometry, vertexShader, fragmentShader, lightList};
        renderer.renderGeometry(rendererPayload);
    }
}

void Scene::draw() {
    if (!screenBuffer) return;
    screenBuffer->clearBuffer();
    Renderer renderer(*screenBuffer);
    for (auto pSceneObject: pSceneObjectList) {
        pSceneObject->draw(renderer);
    }
}

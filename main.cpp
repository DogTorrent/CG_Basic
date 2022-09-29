

#include <iostream>
#include <thread>
#include <opencv2/highgui.hpp>
#include "ThirdParty/cvui.h"
#include "ThirdParty/OBJ_Loader.h"
#include "Primitive.h"
#include "Scene.h"
#include "ScreenBuffer.h"
#include "ToolbarComponent.h"
#include "Object.h"

std::deque<Primitive::Geometry> loadObj(const std::string &pathToObj);

int main() {
    Scene scene;
    ScreenBuffer screenBuffer(700, 700);
    scene.screenBuffer = &screenBuffer;
    SceneObject sceneObject;
    scene.pSceneObjectList.push_back(&sceneObject);
    CameraObject cameraObject;
    scene.cameraObject = &cameraObject;

    sceneObject.geometryList = loadObj(R"(Resources/Models/Spot/spot_collapse.obj)");
    sceneObject.scalingRatio = {3, 3, 3};
    sceneObject.rotationAxis = {0, 1, 0, 0};
    sceneObject.rotationDegree = 30;
    sceneObject.modelPos = {0, 0, 0, 1};
    cameraObject.pos = {0, 0, -15, 1};
    cameraObject.toward = {0, 0, 1, 0};
    cameraObject.top = {0, 1, 0, 0};
    cameraObject.FoV = 60;
    cameraObject.aspectRatio = 1;
    cameraObject.nearPaneZ = 0.1;
    cameraObject.farPaneZ = 50;
    scene.lightList.push_back({{20,  0,   -20},
                               {500, 500, 500}});
    sceneObject.vertexShader = Shader::emptyVertexShader;
    sceneObject.fragmentShader = Shader::blinnPhongFragmentShader;
    sceneObject.renderMode = DEFAULT;

    std::string windowName = "Software Renderer";
    cvui::init(windowName);
    int toolbarWidth = 400;
    int padding = 10;
    ToolbarComponent toolbarComponent{toolbarWidth, padding};
    cv::Mat frame = cv::Mat(cv::Size(screenBuffer.width + toolbarWidth, screenBuffer.height), CV_8UC3);
    cv::Mat image(screenBuffer.width, screenBuffer.height, CV_32FC3, screenBuffer.frameBuffer.data());

    bool isRendering = false;
    while (cv::getWindowProperty(windowName, cv::WINDOW_AUTOSIZE) >= 0) {
        frame = cv::Scalar(49, 52, 49);

        cvui::beginColumn(frame, screenBuffer.width + padding, 0, toolbarWidth - 2 * padding, -1, padding);
        {
            cvui::space(0);

            cvui::text("Object Position");
            toolbarComponent.f3Row(sceneObject.modelPos.x(),
                                   sceneObject.modelPos.y(),
                                   sceneObject.modelPos.z(),
                                   "x:", "y:", "z:", !isRendering);
            cvui::space(0);

            cvui::text("Object Scaling");
            toolbarComponent.f3Row(sceneObject.scalingRatio.x(),
                                   sceneObject.scalingRatio.y(),
                                   sceneObject.scalingRatio.z(),
                                   "x:", "y:", "z:", !isRendering);
            cvui::space(0);

            cvui::text("Object Rotation");
            toolbarComponent.f3Row(sceneObject.rotationAxis.x(),
                                   sceneObject.rotationAxis.y(),
                                   sceneObject.rotationAxis.z(),
                                   "x:", "y:", "z:", !isRendering);
            toolbarComponent.f1Row(sceneObject.rotationDegree, "degree:", !isRendering);
            cvui::space(0);

            cvui::text("Camera Position");
            toolbarComponent.f3Row(cameraObject.pos.x(),
                                   cameraObject.pos.y(),
                                   cameraObject.pos.z(),
                                   "x:", "y:", "z:", !isRendering);
            cvui::space(0);

            cvui::text("Camera Top Axis");
            toolbarComponent.f3Row(cameraObject.top.x(),
                                   cameraObject.top.y(),
                                   cameraObject.top.z(),
                                   "x:", "y:", "z:", !isRendering);
            cvui::space(0);

            cvui::text("Camera Toward Axis");
            toolbarComponent.f3Row(cameraObject.toward.x(),
                                   cameraObject.toward.y(),
                                   cameraObject.toward.z(),
                                   "x:", "y:", "z:", !isRendering);
            cvui::space(0);

            cvui::text("Light 0 Position");
            toolbarComponent.f3Row(scene.lightList[0].pos.x(),
                                   scene.lightList[0].pos.y(),
                                   scene.lightList[0].pos.z(),
                                   "x:", "y:", "z:", !isRendering);
            cvui::space(0);

            cvui::text("Fragment Shader");
            cvui::beginRow(toolbarWidth, -1, padding);
            {
                auto
                pFragmentShader = sceneObject.fragmentShader.target < void(*)
                (const Shader::FragmentShaderPayload &) > ();

                bool enableBlinnPhong = *pFragmentShader == Shader::blinnPhongFragmentShader;
                enableBlinnPhong = cvui::checkbox("Blinn-Phong", &enableBlinnPhong);
                if (!isRendering && enableBlinnPhong) {
                    sceneObject.fragmentShader = Shader::blinnPhongFragmentShader;
                }

                bool enableTexture = *pFragmentShader == Shader::textureFragmentShader;
                enableTexture = cvui::checkbox("Texture", &enableTexture);
                if (!isRendering && enableTexture) {
                    sceneObject.fragmentShader = Shader::textureFragmentShader;
                }

                if (!isRendering && !enableBlinnPhong && !enableTexture) {
                    sceneObject.fragmentShader = Shader::emptyFragmentShader;
                }
            }
            cvui::endRow();
            cvui::space(0);

            cvui::text("Render Mode");
            cvui::beginRow(toolbarWidth, -1, padding);
            {
                bool enableDefaultMode = sceneObject.renderMode == DEFAULT;
                enableDefaultMode = cvui::checkbox("DEFAULT", &enableDefaultMode);
                if (!isRendering && enableDefaultMode) {
                    sceneObject.renderMode = DEFAULT;
                }

                bool enableLineOnlyMode = sceneObject.renderMode == LINE_ONLY;
                enableLineOnlyMode = cvui::checkbox("LINE_ONLY", &enableLineOnlyMode);
                if (!isRendering && enableLineOnlyMode) {
                    sceneObject.renderMode = LINE_ONLY;
                }
            }
            cvui::endRow();
            cvui::space(0);

            cvui::beginRow(toolbarWidth, -1, padding);
            {
                if (cvui::button("Render")) {
                    if (!isRendering) {
                        isRendering = true;
                        std::thread t([&]() -> void {
                            scene.draw();
                            isRendering = false;
                        });
                        t.detach();
                    }
                }
                if (cvui::button("Clean")) {
                    if (!isRendering) {
                        screenBuffer.clearBuffer();
                    }
                }
                if (cvui::button("Reload .obj File")) {
                    if (!isRendering) {
                        sceneObject.geometryList = loadObj(R"(Resources/Models/Spot/spot_collapse.obj)");
                    }
                }
                if (cvui::button("Exit")) {
                    break;
                }
            }
            cvui::endRow();
            cvui::space(0);

            if (isRendering) {
                cvui::text("Rendering...");
                cvui::space(0);
            }
        }
        cvui::endColumn();

        if (!isRendering) {
        }
        image = {screenBuffer.width, screenBuffer.height, CV_32FC3, screenBuffer.frameBuffer.data()};
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
        cvui::image(frame, 0, 0, image);

        cvui::imshow(windowName, frame);

        int key = cv::waitKey(20);
        if (key == 27) {
            break;
        } else if (key == 'a' && !isRendering) {
            isRendering = true;
            cameraObject.pos.x() -= 0.5;
            std::thread t([&]() -> void {
                scene.draw();
                isRendering = false;
            });
            t.detach();
        } else if (key == 'd' && !isRendering) {
            isRendering = true;
            cameraObject.pos.x() += 0.5;
            std::thread t([&]() -> void {
                scene.draw();
                isRendering = false;
            });
            t.detach();
        }
    }
}

std::deque<Primitive::Geometry> loadObj(const std::string &pathToObj) {
    size_t pathSplitIndex = pathToObj.find_last_of('/');
    auto path = pathToObj.substr(0, pathSplitIndex + 1);
    std::deque<Primitive::Geometry> geometryList;
    objl::Loader loader;
    loader.LoadFile(pathToObj);
    std::cout << "meshes count = " << loader.LoadedMeshes.size() << std::endl;
    for (auto mesh: loader.LoadedMeshes) {
        std::cout << " vertices count = " << mesh.Vertices.size() << std::endl;
        Primitive::Geometry geometry;
        geometry.material.ka = Eigen::Vector3f(mesh.MeshMaterial.Ka.X, mesh.MeshMaterial.Ka.Y, mesh.MeshMaterial.Ka.Z);
        geometry.material.kd = Eigen::Vector3f(mesh.MeshMaterial.Kd.X, mesh.MeshMaterial.Kd.Y, mesh.MeshMaterial.Kd.Z);
        geometry.material.ks = Eigen::Vector3f(mesh.MeshMaterial.Ks.X, mesh.MeshMaterial.Ks.Y, mesh.MeshMaterial.Ks.Z);
        geometry.material.ns = mesh.MeshMaterial.Ns;
        geometry.material.diffuseTexture = Primitive::Texture(path + mesh.MeshMaterial.map_Kd);

        geometry.mesh.vertexes.resize(mesh.Vertices.size());
        for (int i = 0; i < mesh.Vertices.size(); ++i) {
            geometry.mesh.vertexes[i].pos = Eigen::Vector4f(-mesh.Vertices[i].Position.X,
                                                            mesh.Vertices[i].Position.Y,
                                                            mesh.Vertices[i].Position.Z,
                                                            1);
            geometry.mesh.vertexes[i].normal = Eigen::Vector3f(-mesh.Vertices[i].Normal.X,
                                                               mesh.Vertices[i].Normal.Y,
                                                               mesh.Vertices[i].Normal.Z);
            geometry.mesh.vertexes[i].uv = Eigen::Vector2f(mesh.Vertices[i].TextureCoordinate.X,
                                                           mesh.Vertices[i].TextureCoordinate.Y);
            geometry.mesh.vertexes[i].color = Eigen::Vector3f(128, 128, 128);
        }

        geometry.mesh.indexes.resize(mesh.Indices.size());
        for (int i = 0; i < mesh.Indices.size(); ++i) {
            geometry.mesh.indexes[i] = mesh.Indices[i];
        }

        geometryList.emplace_back(geometry);
    }
    return geometryList;
}

#define CVUI_IMPLEMENTATION

#include "ThirdParty/cvui.h"
#include <iostream>
#include <thread>
#include <opencv2/highgui.hpp>
#include "ThirdParty/OBJ_Loader.h"
#include "Primitive.h"
#include "Scene.h"
#include "ScreenBuffer.h"
#include "ToolbarComponent.h"
#include "Object.h"

class GUIContext {
public:
    Scene scene;
    std::string windowName = "Software Renderer";
    ToolbarComponent toolbarComponent;
    cv::Mat frame;
    std::mutex imageLock;
    cv::Mat image;
    std::atomic<bool> bufferBusy = false;
    std::deque<std::function<void(void)>> jobs = {[]() {}}; // Add an empty func to render at startup
    std::thread renderThread;
};

std::deque<Primitive::Geometry> loadObj(const std::string &pathToObj);

void drawGUI(GUIContext &guiContext);

void renderAndDrawImage(GUIContext &guiContext);

void guiMouseCallback(int event, int x, int y, int flags, void *userdata);

int main() {
    GUIContext guiContext;
    ScreenBuffer screenBuffer(700, 700);
    SceneObject sceneObject, floorObject;
    CameraObject cameraObject;

    std::string sceneObjectPath = R"(Resources/Models/Spot/spot_triangulated_mod.obj)";
    std::string floorObjectPath = R"(Resources/Models/Flat/floor_mod.obj)";

    guiContext.scene.screenBuffer = &screenBuffer;
    guiContext.scene.pSceneObjectList = {&sceneObject, &floorObject};
    guiContext.scene.cameraObject = &cameraObject;
    guiContext.scene.lightList = {{{-7, 4, -4}, {60, 60, 60}},
                                  {{7,  4, -4}, {60, 60, 60}}};

    floorObject.geometryList = loadObj(floorObjectPath);
    floorObject.scalingRatio = {1, 1, 1};
    floorObject.rotationAxis = {0, 1, 0, 0};
    floorObject.rotationDegree = 0;
    floorObject.modelPos = {0, 0, 0, 1};
    floorObject.vertexShader = Shader::emptyVertexShader;
    floorObject.fragmentShader = Shader::blinnPhongFragmentShader;

    sceneObject.geometryList = loadObj(sceneObjectPath);
    sceneObject.scalingRatio = {1, 1, 1};
    sceneObject.rotationAxis = {0, 1, 0, 0};
    sceneObject.rotationDegree = 30;
    sceneObject.modelPos = {0, 1, 0, 1};
    sceneObject.vertexShader = Shader::emptyVertexShader;
    sceneObject.fragmentShader = Shader::blinnPhongFragmentShader;

    cameraObject.pos = {0, 2, -18, 1};
    cameraObject.toward = {0, 0, 1, 0};
    cameraObject.top = {0, 1, 0, 0};
    cameraObject.FoV = 60;
    cameraObject.aspectRatio = 1;
    cameraObject.nearPaneZ = 0.25;
    cameraObject.farPaneZ = 100;

    guiContext.toolbarComponent.toolbarWidth = 400;
    guiContext.toolbarComponent.padding = 10;

    guiContext.frame = cv::Mat(
            cv::Size(screenBuffer.width + guiContext.toolbarComponent.toolbarWidth, screenBuffer.height), CV_8UC3);
    guiContext.image = cv::Mat(screenBuffer.width, screenBuffer.height, CV_32FC3, screenBuffer.frameBuffer.data());
    guiContext.image.convertTo(guiContext.image, CV_8UC3, 1.0f);
    cv::cvtColor(guiContext.image, guiContext.image, cv::COLOR_RGB2BGR);

    cvui::init(guiContext.windowName);
    cv::setMouseCallback(guiContext.windowName, guiMouseCallback, &guiContext);
    while (cv::getWindowProperty(guiContext.windowName, cv::WINDOW_AUTOSIZE) >= 0) {
        drawGUI(guiContext);
    }
}

void drawGUI(GUIContext &guiContext) {
    int toolbarWidth = guiContext.toolbarComponent.toolbarWidth;
    int padding = guiContext.toolbarComponent.padding;
    guiContext.frame = cv::Scalar(49, 52, 49);

    cvui::beginColumn(guiContext.frame, guiContext.scene.screenBuffer->width + padding, 0,
                      toolbarWidth - 2 * padding, -1, padding);
    {
        cvui::space(0);

        for (int i = 0; i < guiContext.scene.pSceneObjectList.size(); ++i) {
            std::string objName = "Object " + std::to_string(i);

            cvui::text(objName + " Position");
            guiContext.toolbarComponent.fRow<3>({&guiContext.scene.pSceneObjectList[i]->modelPos.x(),
                                                 &guiContext.scene.pSceneObjectList[i]->modelPos.y(),
                                                 &guiContext.scene.pSceneObjectList[i]->modelPos.z()},
                                                {"x:", "y:", "z:"}, !guiContext.bufferBusy);
            cvui::space(0);

            cvui::text(objName + " Fragment Shader");
            guiContext.toolbarComponent.checkBoxes < void(*)(const Shader::FragmentShaderPayload &), 3 > (
                    *guiContext.scene.pSceneObjectList[i]->fragmentShader.target < void(*)(
            const Shader::FragmentShaderPayload &) > (),
                    {Shader::blinnPhongFragmentShader,
                     Shader::textureFragmentShader,
                     Shader::emptyFragmentShader},
                    {"Blinn-Phong", "Texture", "Empty"},
                    !guiContext.bufferBusy);
            cvui::space(0);


            cvui::text(objName + " Render Mode");
            guiContext.toolbarComponent.checkBoxes<RenderOption::RenderMode, 2>(
                    guiContext.scene.pSceneObjectList[i]->renderOption.renderMode,
                    {RenderOption::MODE_DEFAULT,
                     RenderOption::MODE_LINE_ONLY},
                    {"MODE_DEFAULT", "MODE_LINE_ONLY"},
                    !guiContext.bufferBusy);
            cvui::space(0);

            cvui::text(objName + " Culling Mode");
            guiContext.toolbarComponent.checkBoxes<RenderOption::Culling, 3>(
                    guiContext.scene.pSceneObjectList[i]->renderOption.culling,
                    {RenderOption::CULL_BACK,
                     RenderOption::CULL_FRONT,
                     RenderOption::CULL_NONE},
                    {"CULL_BACK", "CULL_FRONT", "CULL_NONE"},
                    !guiContext.bufferBusy);
            cvui::space(0);
        }

        cvui::text("Camera Position");
        guiContext.toolbarComponent.fRow<3>(
                {&guiContext.scene.cameraObject->pos.x(),
                 &guiContext.scene.cameraObject->pos.y(),
                 &guiContext.scene.cameraObject->pos.z()},
                {"x:", "y:", "z:"}, !guiContext.bufferBusy);
        cvui::space(0);

        for (int i = 0; i < guiContext.scene.lightList.size(); ++i) {
            cvui::text("Light " + std::to_string(i) + " Position");
            guiContext.toolbarComponent.fRow<3>(
                    {&guiContext.scene.lightList[i].pos.x(),
                     &guiContext.scene.lightList[i].pos.y(),
                     &guiContext.scene.lightList[i].pos.z()},
                    {"x:", "y:", "z:"}, !guiContext.bufferBusy);
            cvui::space(0);
        }

        cvui::beginRow(toolbarWidth, -1, padding);
        {
            if (cvui::button("Render")) {
                renderAndDrawImage(guiContext);
            }
            if (cvui::button("Clean")) {
                bool expected = false;
                guiContext.bufferBusy.compare_exchange_strong(expected, true);
                if (!expected) {
                    guiContext.scene.screenBuffer->clearBuffer();
                    guiContext.imageLock.lock();
                    guiContext.image = {guiContext.scene.screenBuffer->width, guiContext.scene.screenBuffer->height,
                                        CV_32FC3, guiContext.scene.screenBuffer->frameBuffer.data()};
                    guiContext.image.convertTo(guiContext.image, CV_8UC3, 1.0f);
                    cv::cvtColor(guiContext.image, guiContext.image, cv::COLOR_RGB2BGR);
                    guiContext.imageLock.unlock();
                    guiContext.bufferBusy = false;
                }
            }
            if (cvui::button("Exit")) {
                cv::destroyAllWindows();
                exit(0);
            }
            if (guiContext.bufferBusy) {
                cvui::text("Rendering...");
            }
        }
        cvui::endRow();
        cvui::space(0);
    }
    cvui::endColumn();

    guiContext.imageLock.lock();
    cvui::image(guiContext.frame, 0, 0, guiContext.image);
    guiContext.imageLock.unlock();

    cvui::imshow(guiContext.windowName, guiContext.frame);

    int key = cv::waitKeyEx(20);
    switch (key) {
        case 27: {
            cv::destroyAllWindows();
            exit(0);
        }
        case 'w': {
            guiContext.jobs.emplace_back([&guiContext]() { guiContext.scene.cameraObject->moveForward(0.2); });
            break;
        }
        case 'a': {
            guiContext.jobs.emplace_back([&guiContext]() { guiContext.scene.cameraObject->moveRight(-0.2); });
            break;
        }
        case 's': {
            guiContext.jobs.emplace_back([&guiContext]() { guiContext.scene.cameraObject->moveForward(-0.2); });
            break;
        }
        case 'd': {
            guiContext.jobs.emplace_back([&guiContext]() { guiContext.scene.cameraObject->moveRight(0.2); });
            break;
        }
        case 32: {
            guiContext.jobs.emplace_back([&guiContext]() { guiContext.scene.cameraObject->moveUp(0.2); });
            break;
        }
        case 120: {
            guiContext.jobs.emplace_back([&guiContext]() { guiContext.scene.cameraObject->moveUp(-0.2); });
            break;
        }
        default: {
            if (key != -1) std::cout << key << std::endl;
            break;
        }
    }
    if (!guiContext.jobs.empty()) renderAndDrawImage(guiContext);
}

void guiMouseCallback(int event, int x, int y, int flags, void *userdata) {
    GUIContext &guiContext = *(GUIContext *) userdata;
    static int lastX = -1, lastY = -1;
    switch (event) {
        case cv::EVENT_LBUTTONDOWN: {
            if (!(x >= 0 && x < guiContext.image.cols && y >= 0 && y < guiContext.image.rows)) break;
            lastX = x, lastY = y;
            break;
        }
        case cv::EVENT_MOUSEMOVE: {
            if (!(x >= 0 && x < guiContext.image.cols && y >= 0 && y < guiContext.image.rows)) break;
            if (!(lastX >= 0 && lastX < guiContext.image.cols && lastY >= 0 && lastY < guiContext.image.rows)) break;
            auto deltaX = (float) (x - lastX) * 0.2f, deltaY = (float) (y - lastY) * 0.2f;
            lastX = x, lastY = y;
            guiContext.jobs.emplace_back([&guiContext, deltaX, deltaY]() {
                Eigen::Vector4f topAxis(0, 1, 0, 0);
                Eigen::Vector4f rightAxis(
                        guiContext.scene.cameraObject->top.cross3(guiContext.scene.cameraObject->toward));
                guiContext.scene.cameraObject->rotate(rightAxis, deltaY);
                guiContext.scene.cameraObject->rotate(topAxis, deltaX);
            });
            break;
        }
        case cv::EVENT_LBUTTONUP: {
            lastX = -1, lastY = -1;
            break;
        }
        default: {
            break;
        }
    }
    cvui::handleMouse(event, x, y, flags, &cvui::internal::getContext(guiContext.windowName));
}

void renderAndDrawImage(GUIContext &guiContext) {
    bool expected = false;
    guiContext.bufferBusy.compare_exchange_strong(expected, true);
    if (!expected) {
        for (auto &job: guiContext.jobs) {
            job();
        }
        guiContext.jobs.clear();
        guiContext.renderThread = std::thread([&]() -> void {
            guiContext.scene.draw();
            guiContext.imageLock.lock();
            guiContext.image = {guiContext.scene.screenBuffer->width, guiContext.scene.screenBuffer->height,
                                CV_32FC3, guiContext.scene.screenBuffer->frameBuffer.data()};
            guiContext.image.convertTo(guiContext.image, CV_8UC3, 1.0f);
            cv::cvtColor(guiContext.image, guiContext.image, cv::COLOR_RGB2BGR);
            guiContext.imageLock.unlock();
            guiContext.bufferBusy = false;
        });
        guiContext.renderThread.detach();
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
        std::cout << " indices count = " << mesh.Indices.size() << std::endl;
        Primitive::Geometry geometry;
        geometry.material.ka = Eigen::Vector3f(mesh.MeshMaterial.Ka.X, mesh.MeshMaterial.Ka.Y, mesh.MeshMaterial.Ka.Z);
        geometry.material.kd = Eigen::Vector3f(mesh.MeshMaterial.Kd.X, mesh.MeshMaterial.Kd.Y, mesh.MeshMaterial.Kd.Z);
        geometry.material.ks = Eigen::Vector3f(mesh.MeshMaterial.Ks.X, mesh.MeshMaterial.Ks.Y, mesh.MeshMaterial.Ks.Z);
        geometry.material.ns = mesh.MeshMaterial.Ns;
        if (!mesh.MeshMaterial.map_Kd.empty())
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

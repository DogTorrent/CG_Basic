#include <iostream>
#include "ThirdParty/OBJ_Loader.h"
#include "Primitive.h"
#include "Renderer.h"
#include "TransformMatrix.h"

std::deque<Primitive::Geometry> loadObj(const std::string &pathToObj) {
    size_t pathSplitIndex = pathToObj.find_last_of('/');
    auto path = pathToObj.substr(0, pathSplitIndex + 1);
    std::deque<Primitive::Geometry> geometryList;
    objl::Loader loader;
    loader.LoadFile(pathToObj);
    for (auto mesh: loader.LoadedMeshes) {
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

int main() {
    std::deque<Primitive::Geometry> geometryList = loadObj(
            R"(C:/Users/admin/CLionProjects/CG_Basic/Resources/Models/Spot/spot_triangulated_mod.obj)");
    int screenWidth = 700;
    int screenHeight = 700;
    ScreenBuffer screenBuffer(screenWidth, screenHeight);
    Renderer renderer(screenBuffer);

    Eigen::Vector3f scalingRatio(2.5, 2.5, 2.5);
    Eigen::Vector4f rotationAxis(0, 1, 0, 0);
    float rotationDegree = 0;
    Eigen::Vector4f modelPos(0, 0, 0, 1);
    Eigen::Vector4f cameraPos(0, 0, -15, 1);
    Eigen::Vector4f cameraToward(0, 0, 1, 0);
    Eigen::Vector4f cameraTop(0, 1, 0, 0);
    float FoV = 45;
    float aspectRatio = (float) screenWidth / (float) screenHeight;
    float nearPaneZ = 1;
    float farPaneZ = 50;
    std::deque<Primitive::Light> lightList;
    lightList.push_back({{25,  10,  5},
                         {500, 500, 500}});
    lightList.push_back({{0,   0,   -30},
                         {500, 500, 500}});
    std::function<void(Shader::VertexShaderPayload &)> vertexShader = Shader::emptyVertexShader;
    std::function<void(Shader::FragmentShaderPayload &)> fragmentShader = Shader::blinnPhongFragmentShader;

    int keyboardKey;
    while (true) {
        screenBuffer.clearBuffer();
        renderer.modelMatrix = TransformMatrix::getModelMatrix(TransformMatrix::getScalingMatrix(scalingRatio),
                                                               TransformMatrix::getRotationMatrix(rotationAxis,
                                                                                                  rotationDegree),
                                                               TransformMatrix::getMovingMatrix(modelPos));

        renderer.viewMatrix = TransformMatrix::getViewMatrix(cameraPos, cameraToward, cameraTop);

        renderer.projectionMatrix = TransformMatrix::getProjectionMatrix(FoV, aspectRatio, nearPaneZ, farPaneZ);

        for (auto &geometry: geometryList) {
            RendererPayload rendererPayload{geometry, vertexShader, fragmentShader, lightList};
            renderer.renderGeometry(rendererPayload);
        }

        cv::Mat image(screenWidth, screenHeight, CV_32FC3, screenBuffer.frameBuffer.data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
        cv::imshow("preview", image);

        keyboardKey = cv::waitKeyEx();
        switch (keyboardKey) {
            case 'w': {
                cameraPos.y() += 1;
                break;
            }
            case 'a': {
                cameraPos.x() -= 1;
                break;
            }
            case 's': {
                cameraPos.y() -= 1;
                break;
            }
            case 'd': {
                cameraPos.x() += 1;
                break;
            }
            case 61: { // +
                cameraPos.z() += 1;
                break;
            }
            case 45: { // -
                cameraPos.z() -= 1;
                break;
            }
            case 2490368: { // up
                modelPos.y() += 1;
                break;
            }
            case 2621440: { // down
                modelPos.y() -= 1;
                break;
            }
            case 2424832: { // left
                modelPos.x() -= 1;
                break;
            }
            case 2555904: { // right
                modelPos.x() += 1;
                break;
            }
            case 27:
            case -1: {
                return 0;
            }
            default: {
                std::cout << keyboardKey << std::endl;
                break;
            }
        }
    }

}

//
// Created by admin on 2022/9/23.
//

#ifndef CG_BASIC_SCREENBUFFER_H
#define CG_BASIC_SCREENBUFFER_H


#include <vector>
#include <eigen3/Eigen/Core>

#define MAX_DEPTH 50.f

class ScreenBuffer {
public:
    int width;
    int height;
    std::vector<Eigen::Vector3f> frameBuffer;
    std::vector<float> depthBuffer;

    ScreenBuffer(int width, int height);

    void clearBuffer();

    int getIndex(int x, int y) const;

    Eigen::Vector3f &valueInFrameBuffer(int x, int y);

    float &valueInDepthBuffer(int x, int y);
};


#endif //CG_BASIC_SCREENBUFFER_H

//
// Created by .torrent on 2022/9/27.
//
#define CVUI_IMPLEMENTATION

#include <eigen3/Eigen/Core>
#include "ThirdParty/cvui.h"
#include "ToolbarComponent.h"

void ToolbarComponent::f1Row(float &dest1, const std::string &labelName1, bool enabled) {
    cvui::beginRow(toolbarWidth, -1, padding);
    {
        double d = dest1;
        cvui::text(labelName1);
        cvui::counter(&d, 1, "%.1f");
        if (enabled) {
            dest1 = (float) d;
        }
    }
    cvui::endRow();
}

void ToolbarComponent::f2Row(float &dest1, float &dest2,
                             const std::string &labelName1, const std::string &labelName2, bool enabled) {
    cvui::beginRow(toolbarWidth, -1, padding);
    {
        Eigen::Vector2d v2d(dest1, dest2);
        cvui::text(labelName1);
        cvui::counter(&v2d.x(), 1, "%.1f");
        cvui::text(labelName2);
        cvui::counter(&v2d.y(), 1, "%.1f");
        if (enabled) {
            dest1 = (float) v2d.x();
            dest2 = (float) v2d.y();
        }
    }
    cvui::endRow();
}

void ToolbarComponent::f3Row(float &dest1, float &dest2, float &dest3,
                             const std::string &labelName1, const std::string &labelName2,
                             const std::string &labelName3, bool enabled) {
    cvui::beginRow(toolbarWidth, -1, padding);
    {
        Eigen::Vector3d v3d(dest1, dest2, dest3);
        cvui::text(labelName1);
        cvui::counter(&v3d.x(), 1, "%.1f");
        cvui::text(labelName2);
        cvui::counter(&v3d.y(), 1, "%.1f");
        cvui::text(labelName3);
        cvui::counter(&v3d.z(), 1, "%.1f");
        if (enabled) {
            dest1 = (float) v3d.x();
            dest2 = (float) v3d.y();
            dest3 = (float) v3d.z();
        }
    }
    cvui::endRow();
}


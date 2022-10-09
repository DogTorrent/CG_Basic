//
// Created by .torrent on 2022/9/27.
//

#ifndef CG_BASIC_TOOLBARCOMPONENT_H
#define CG_BASIC_TOOLBARCOMPONENT_H


#include <string>

class ToolbarComponent {
public:
    int toolbarWidth = 350;
    int padding = 5;

    template<int size>
    void fRow(const std::array<float *, size> &dests, const std::array<std::string, size> &names, bool enabled = true) {
        cvui::beginRow(toolbarWidth, -1, padding);
        {
            for (int i = 0; i < size; ++i) {
                cvui::text(names[i]);
                auto temp = (double)*dests[i];
                cvui::counter(&temp, 1, "%.1f");
                if (enabled) *dests[i] = (float)temp;
            }
        }
        cvui::endRow();
    }

    template<typename T, int size>
    void checkBoxes(T &dest, const std::array<T, size> &candidates,
                    const std::array<std::string, size> &names, bool enabled = true) {
        cvui::beginRow(toolbarWidth, -1, padding);
        {
            for (int i = 0; i < size; ++i) {
                bool enableOption = dest == candidates[i];
                enableOption = cvui::checkbox(names[i], &enableOption);
                if (enabled && enableOption) {
                    dest = candidates[i];
                }
            }
        }
        cvui::endRow();
    }
};


#endif //CG_BASIC_TOOLBARCOMPONENT_H

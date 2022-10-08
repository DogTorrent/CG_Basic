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

    void f1Row(float &dest1, const std::string &labelName1, bool enabled = true);

    void f2Row(float &dest1, float &dest2,
               const std::string &labelName1, const std::string &labelName2, bool enabled = true);

    void f3Row(float &dest1, float &dest2, float &dest3,
               const std::string &labelName1, const std::string &labelName2, const std::string &labelName3,
               bool enabled = true);

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

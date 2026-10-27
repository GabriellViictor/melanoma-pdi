#pragma once

#include <string>
#include <opencv2/core.hpp>
#include "io/DatasetManager.hpp"

namespace melanoma::core {

struct ProcessingSummary {
    std::string imageId;
    io::DatasetSplit split = io::DatasetSplit::Train;
    io::LesionClass label = io::LesionClass::Benign;

    int width = 0;
    int height = 0;
    int channels = 0;
    bool valid = false;
    bool dimensionsStandard = false;

    double meanB = 0.0;
    double meanG = 0.0;
    double meanR = 0.0;
    double meanLuminance = 0.0;
    double stdDevLuminance = 0.0;
};

class ImageProcessor {
public:
    static constexpr int kExpectedWidth = 224;
    static constexpr int kExpectedHeight = 224;
    static constexpr int kExpectedChannels = 3;

    static ProcessingSummary processBasic(const io::ImageEntry& entry, const cv::Mat& bgrImage);
};

} // namespace melanoma::core

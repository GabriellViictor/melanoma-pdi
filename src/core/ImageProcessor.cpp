#include "core/ImageProcessor.hpp"

namespace melanoma::core {

ProcessingSummary ImageProcessor::processBasic(const io::ImageEntry& entry, const cv::Mat& bgrImage) {
    ProcessingSummary summary;
    summary.imageId = entry.id;
    summary.split = entry.split;
    summary.label = entry.label;
    summary.width = bgrImage.cols;
    summary.height = bgrImage.rows;
    summary.channels = bgrImage.channels();
    summary.valid = !bgrImage.empty();
    summary.dimensionsStandard = (summary.width == kExpectedWidth &&
                                  summary.height == kExpectedHeight &&
                                  summary.channels == kExpectedChannels);
    return summary;
}

} // namespace melanoma::core

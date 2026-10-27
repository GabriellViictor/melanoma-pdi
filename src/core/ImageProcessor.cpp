#include "core/ImageProcessor.hpp"

#include <opencv2/imgproc.hpp>

namespace melanoma::core {

ProcessingSummary ImageProcessor::processBasic(const io::ImageEntry& entry, const cv::Mat& bgrImage) {
    ProcessingSummary summary;
    summary.imageId = entry.id;
    summary.split = entry.split;
    summary.label = entry.label;
    summary.width = bgrImage.cols;
    summary.height = bgrImage.rows;
    summary.channels = bgrImage.channels();
    summary.valid = ! bgrImage.empty();
    summary.dimensionsStandard = (summary.width == kExpectedWidth &&
                                  summary.height == kExpectedHeight &&
                                  summary.channels == kExpectedChannels);

    if (! summary.valid) {
        return summary;
    }

    const cv::Scalar meanBgr = cv::mean(bgrImage);
    summary.meanB = meanBgr[0];
    summary.meanG = meanBgr[1];
    summary.meanR = meanBgr[2];

    cv::Mat grayImage;
    if (bgrImage.channels() == 3) {
        cv::cvtColor(bgrImage, grayImage, cv::COLOR_BGR2GRAY);
    } else {
        grayImage = bgrImage;
    }

    cv::Scalar meanLum;
    cv::Scalar stdDevLum;
    cv::meanStdDev(grayImage, meanLum, stdDevLum);
    summary.meanLuminance = meanLum[0];
    summary.stdDevLuminance = stdDevLum[0];

    return summary;
}

} // namespace melanoma::core

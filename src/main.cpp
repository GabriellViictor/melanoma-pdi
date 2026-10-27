#include <filesystem>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

#include "core/ImageProcessor.hpp"
#include "io/DatasetManager.hpp"

int main(int argc, char** argv) {
    std::filesystem::path datasetPath = "images/input";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dataset" && i + 1 < argc) {
            datasetPath = argv[++i];
        }
    }

    std::cout << "=========================================\n"
              << " Melanoma PDI — Dermoscopic Analysis CLI\n"
              << "=========================================\n"
              << "OpenCV Version : " << CV_VERSION << "\n"
              << "Dataset Path   : " << datasetPath.string() << "\n";

    // Verify OpenCV matrix allocation works
    cv::Mat testMat(
        melanoma::core::ImageProcessor::kExpectedHeight,
        melanoma::core::ImageProcessor::kExpectedWidth,
        CV_8UC3,
        cv::Scalar(120, 140, 180)
    );
    std::cout << "OpenCV expected mat: " << testMat.cols << "x" << testMat.rows
              << "x" << testMat.channels();

    return 0;
}

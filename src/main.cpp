#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

#include "core/ImageProcessor.hpp"
#include "io/DatasetManager.hpp"

int main(int argc, char** argv) {
    std::filesystem::path datasetPath = "images/input";
    std::size_t samplesPerClass = 2;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dataset" && i + 1 < argc) {
            datasetPath = argv[++i];
        } else if (arg == "--samples" && i + 1 < argc) {
            samplesPerClass = static_cast<std::size_t>(std::stoul(argv[++i]));
        }
    }

    const std::filesystem::path execPath = (argc > 0 && argv[0] != nullptr)
                                               ? std::filesystem::path(argv[0])
                                               : std::filesystem::path{};
    datasetPath = melanoma::io::DatasetManager::resolveDatasetRoot(datasetPath, execPath);

    std::cout << "=========================================================\n"
              << " Melanoma PDI — Dermoscopic Analysis CLI\n"
              << "=========================================================\n"
              << "OpenCV Version : " << CV_VERSION << "\n"
              << "Dataset Path   : " << datasetPath.string() << "\n"
              << "---------------------------------------------------------\n";

    const auto report = melanoma::io::DatasetManager::inspectDataset(datasetPath);
    std::cout << "Dataset Integrity Check:\n"
              << "  train/Benign    : " << report.trainBenignCount
              << (report.trainBenignExists ? "" : " [MISSING]") << "\n"
              << "  train/Malignant : " << report.trainMalignantCount
              << (report.trainMalignantExists ? "" : " [MISSING]") << "\n"
              << "  test/Benign     : " << report.testBenignCount
              << (report.testBenignExists ? "" : " [MISSING]") << "\n"
              << "  test/Malignant  : " << report.testMalignantCount
              << (report.testMalignantExists ? "" : " [MISSING]") << "\n"
              << "  Total Images    : " << report.totalCount() << "\n"
              << "---------------------------------------------------------\n";

    if (! report.isValid()) {
        std::cerr << "Erro: Dataset ausente ou incompleto em '" << datasetPath.string()
                  << "'. Execute ./scripts/download_dataset.sh primeiro.\n";
        return 1;
    }

    const auto samples = melanoma::io::DatasetManager::listImages(datasetPath, std::nullopt, samplesPerClass);
    std::cout << "Sample Processing (" << samples.size() << " images loaded):\n";
    std::cout << std::left
              << std::setw(8)  << "ID"
              << std::setw(8)  << "Split"
              << std::setw(12) << "Class"
              << std::setw(12) << "Dims"
              << std::setw(22) << "Mean RGB"
              << "Luminance (Mean +/- Std)\n";

    std::cout << std::fixed << std::setprecision(1);
    for (const auto& entry : samples) {
        const cv::Mat img = melanoma::io::DatasetManager::loadImage(entry);
        const auto summary = melanoma::core::ImageProcessor::processBasic(entry, img);

        if (! summary.valid) {
            std::cerr << "Falha ao ler imagem: " << entry.filePath.string() << "\n";
            return 1;
        }

        const std::string dims = std::to_string(summary.width) + "x" +
                                 std::to_string(summary.height) + "x" +
                                 std::to_string(summary.channels);

        std::ostringstream rgbStream;
        rgbStream << std::fixed << std::setprecision(1)
                  << "(" << summary.meanR << "," << summary.meanG << "," << summary.meanB << ")";

        std::cout << std::left
                  << std::setw(8)  << summary.imageId
                  << std::setw(8)  << melanoma::io::DatasetManager::toString(summary.split)
                  << std::setw(12) << melanoma::io::DatasetManager::toString(summary.label)
                  << std::setw(12) << dims
                  << std::setw(22) << rgbStream.str()
                  << summary.meanLuminance << " +/- " << summary.stdDevLuminance << "\n";
    }
    std::cout << "=========================================================\n";

    return 0;
}

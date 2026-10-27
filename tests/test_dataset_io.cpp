#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

#include "core/ImageProcessor.hpp"
#include "io/DatasetManager.hpp"

namespace {

bool runDatasetIntegrityTest(const std::filesystem::path& datasetRoot) {
    std::cout << "[TEST] Verificando presenca e contagem de arquivos do dataset...\n";
    std::cout << "       Caminho: " << datasetRoot.string() << "\n";

    const auto report = melanoma::io::DatasetManager::inspectDataset(datasetRoot);

    if (! report.hasAllDirectories()) {
        std::cerr << "[FAIL] Diretorios obrigatorios (train/Benign, train/Malignant, "
                     "test/Benign, test/Malignant) nao encontrados.\n";
        return false;
    }

    std::cout << "       train/Benign    : " << report.trainBenignCount
              << " (esperado: " << melanoma::io::DatasetManager::kExpectedTrainBenign << ")\n"
              << "       train/Malignant : " << report.trainMalignantCount
              << " (esperado: " << melanoma::io::DatasetManager::kExpectedTrainMalignant << ")\n"
              << "       test/Benign     : " << report.testBenignCount
              << " (esperado: " << melanoma::io::DatasetManager::kExpectedTestBenign << ")\n"
              << "       test/Malignant  : " << report.testMalignantCount
              << " (esperado: " << melanoma::io::DatasetManager::kExpectedTestMalignant << ")\n"
              << "       Total           : " << report.totalCount()
              << " (esperado: " << melanoma::io::DatasetManager::kExpectedTotal << ")\n";

    if (report.trainBenignCount != melanoma::io::DatasetManager::kExpectedTrainBenign ||
        report.trainMalignantCount != melanoma::io::DatasetManager::kExpectedTrainMalignant ||
        report.testBenignCount != melanoma::io::DatasetManager::kExpectedTestBenign ||
        report.testMalignantCount != melanoma::io::DatasetManager::kExpectedTestMalignant ||
        report.totalCount() != melanoma::io::DatasetManager::kExpectedTotal) {
        std::cerr << "[FAIL] Contagem de arquivos diverge do esperado no dataset.\n";
        return false;
    }

    std::cout << "[PASS] Dataset presente e integro com todas as "
              << report.totalCount() << " imagens de treino e teste.\n\n";
    return true;
}

bool runProcessingSmokeTest(const std::filesystem::path& datasetRoot) {
    std::cout << "[TEST] Verificando leitura de imagens e processamento OpenCV...\n";

    constexpr std::size_t kSamplesPerClass = 3;
    const auto samples = melanoma::io::DatasetManager::listImages(datasetRoot, std::nullopt, kSamplesPerClass);
    const std::size_t expectedSamples = kSamplesPerClass * 4;

    if (samples.size() != expectedSamples) {
        std::cerr << "[FAIL] Quantidade de amostras listadas (" << samples.size()
                  << ") difere do esperado (" << expectedSamples << ").\n";
        return false;
    }

    for (const auto& entry : samples) {
        const cv::Mat image = melanoma::io::DatasetManager::loadImage(entry);
        if (image.empty()) {
            std::cerr << "[FAIL] Falha ao carregar matriz OpenCV para: " << entry.filePath.string() << "\n";
            return false;
        }

        const auto summary = melanoma::core::ImageProcessor::processBasic(entry, image);
        if (! summary.valid || ! summary.dimensionsStandard) {
            std::cerr << "[FAIL] Dimensoes invalidas para imagem ID " << summary.imageId
                      << ": " << summary.width << "x" << summary.height << "x" << summary.channels << "\n";
            return false;
        }

        if (summary.meanLuminance <= 0.0 || summary.meanLuminance >= 255.0 ||
            summary.stdDevLuminance <= 0.0) {
            std::cerr << "[FAIL] Estatisticas de luminancia invalidas para imagem ID "
                      << summary.imageId << "\n";
            return false;
        }

        std::cout << "       [OK] ID=" << std::left << std::setw(6) << summary.imageId
                  << " | Split=" << std::setw(6) << melanoma::io::DatasetManager::toString(summary.split)
                  << " | Class=" << std::setw(10) << melanoma::io::DatasetManager::toString(summary.label)
                  << " | Dims=" << summary.width << "x" << summary.height << "x" << summary.channels
                  << " | Lum=" << std::fixed << std::setprecision(1)
                  << summary.meanLuminance << " +/- " << summary.stdDevLuminance << "\n";
    }

    std::cout << "[PASS] Leitura e processamento OpenCV verificados com sucesso em "
              << samples.size() << " amostras.\n\n";
    return true;
}

} // namespace

int main(int argc, char** argv) {
    std::filesystem::path requestedDataset = "images/input";
    std::string mode = "all";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dataset" && i + 1 < argc) {
            requestedDataset = argv[++i];
        } else if (arg == "--test-dataset") {
            mode = "dataset";
        } else if (arg == "--test-processing") {
            mode = "processing";
        }
    }

    const std::filesystem::path execPath = (argc > 0 && argv[0] != nullptr)
                                               ? std::filesystem::path(argv[0])
                                               : std::filesystem::path{};
    const auto datasetRoot = melanoma::io::DatasetManager::resolveDatasetRoot(requestedDataset, execPath);

    bool ok = true;
    if (mode == "all" || mode == "dataset") {
        ok = runDatasetIntegrityTest(datasetRoot) && ok;
    }
    if (mode == "all" || mode == "processing") {
        ok = runProcessingSmokeTest(datasetRoot) && ok;
    }

    return ok ? 0 : 1;
}

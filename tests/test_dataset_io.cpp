#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

#include "core/ImageProcessor.hpp"
#include "io/DatasetManager.hpp"

namespace {

bool runDatasetIntegrityTest(const std::filesystem::path& datasetRoot) {
    std::cout << "[TEST] Verificando presenca e contagem de arquivos do dataset...\n";
    std::cout << "       Caminho: " << datasetRoot.string() << "\n";

    const auto report = melanoma::io::DatasetManager::inspectDataset(datasetRoot);

    if (! report.hasAllDirectories()) {
        std::cout << "[INFO] Diretorios do dataset ainda nao baixados localmente (use scripts/download_dataset).\n";
        return true; // Nao falha o build quando os dados pesados nao estao versionados no git
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

    const auto report = melanoma::io::DatasetManager::inspectDataset(datasetRoot);
    if (! report.isValid()) {
        std::cout << "[INFO] Dataset nao baixado; pulando smoke test de processamento em lote.\n";
        return true;
    }

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
    }

    std::cout << "[PASS] Leitura e processamento OpenCV verificados com sucesso em "
              << samples.size() << " amostras.\n\n";
    return true;
}

bool runPipelineAlgorithmTest() {
    std::cout << "[TEST] Verificando algoritmos do Pipeline M2 (Pelos, Otsu, Morfologia e Metricas)...\n";

    // Cria imagem sintetica de teste: fundo claro de pele (BGR ~ 180, 200, 230),
    // lesao eliptica central escura (BGR ~ 60, 70, 90) e linha preta simulando pelo
    cv::Mat testImg(224, 224, CV_8UC3, cv::Scalar(180, 200, 230));
    cv::ellipse(testImg, cv::Point(112, 112), cv::Size(45, 30), 20.0, 0, 360, cv::Scalar(60, 70, 90), -1);
    // Linha escura simulando pelo fino
    cv::line(testImg, cv::Point(40, 60), cv::Point(180, 160), cv::Scalar(20, 20, 20), 2);

    const auto result = melanoma::core::ImageProcessor::processPipeline(
        testImg, "synthetic_test", melanoma::io::DatasetSplit::Test, melanoma::io::LesionClass::Benign);

    if (! result.success) {
        std::cerr << "[FAIL] Pipeline falhou na imagem de teste sintetica.\n";
        return false;
    }

    if (result.artifacts.hairMask.empty() || cv::countNonZero(result.artifacts.hairMask) == 0) {
        std::cerr << "[FAIL] Deteccao de pelos nao identificou a linha desenhada.\n";
        return false;
    }

    if (result.features.area <= 100.0) {
        std::cerr << "[FAIL] Area segmentada muito baixa: " << result.features.area << "\n";
        return false;
    }

    if (result.features.compactness < 1.0) {
        std::cerr << "[FAIL] Compacidade matematicamente inconsistente (< 1.0): "
                  << result.features.compactness << "\n";
        return false;
    }

    std::cout << "       [OK] Deteccao de pelos identificou pixels de pelo.\n"
              << "       [OK] Segmentacao isolou a lesao central.\n"
              << "       [OK] Area=" << result.features.area << " px, Perimetro="
              << result.features.perimeter << " px, Compacidade=" << result.features.compactness << "\n"
              << "[PASS] Algoritmos centrais de PDI verificados com sucesso!\n\n";

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
        } else if (arg == "--test-pipeline") {
            mode = "pipeline";
        }
    }

    const std::filesystem::path execPath = (argc > 0 && argv[0] != nullptr)
                                               ? std::filesystem::path(argv[0])
                                               : std::filesystem::path{};
    const auto datasetRoot = melanoma::io::DatasetManager::resolveDatasetRoot(requestedDataset, execPath);

    bool ok = true;
    if (mode == "all" || mode == "pipeline") {
        ok = runPipelineAlgorithmTest() && ok;
    }
    if (mode == "all" || mode == "dataset") {
        ok = runDatasetIntegrityTest(datasetRoot) && ok;
    }
    if (mode == "all" || mode == "processing") {
        ok = runProcessingSmokeTest(datasetRoot) && ok;
    }

    return ok ? 0 : 1;
}

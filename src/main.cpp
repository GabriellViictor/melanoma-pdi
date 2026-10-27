#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

#include "core/ImageProcessor.hpp"
#include "io/DatasetManager.hpp"

namespace {

void printHelp(const char* progName) {
    std::cout << "Uso: " << progName << " [opcoes]\n\n"
              << "Opcoes disponiveis:\n"
              << "  --image <path>       Processa uma unica imagem especifica\n"
              << "  --input-dir <path>   Processa todas as imagens encontradas no diretorio informado\n"
              << "  --dataset <path>     Caminho para a raiz do dataset (padrao: images/input)\n"
              << "  --samples <n>        Numero de amostras por classe ao ler o dataset (padrao: 2)\n"
              << "  --output-dir <path>  Diretorio para salvar saidas e artefatos (padrao: images/results)\n"
              << "  --channel <lab|gray> Canal para segmentacao por Otsu (padrao: lab)\n"
              << "  --no-inpaint         Desativa a remocao de pelos (util para experimentos comparativos)\n"
              << "  --csv <path>         Caminho do arquivo CSV de metricas (padrao: <output-dir>/metrics.csv)\n"
              << "  -h, --help           Exibe esta mensagem de ajuda\n\n"
              << "Exemplos:\n"
              << "  " << progName << " --image docs/7280.jpg --output-dir images/results\n"
              << "  " << progName << " --dataset images/input --samples 2 --channel lab\n"
              << "  " << progName << " --image docs/7280.jpg --no-inpaint (para comparacao)\n";
}

struct ImageItem {
    std::filesystem::path path;
    std::string id;
    melanoma::io::DatasetSplit split = melanoma::io::DatasetSplit::Test;
    melanoma::io::LesionClass label = melanoma::io::LesionClass::Benign;
};

void appendMetricsToCsv(const std::filesystem::path& csvPath,
                        const ImageItem& item,
                        const melanoma::core::PipelineResult& result) {
    const bool fileExists = std::filesystem::exists(csvPath);
    std::ofstream out(csvPath, std::ios::app);
    if (! out.is_open()) {
        std::cerr << "[AVISO] Nao foi possivel abrir/criar CSV em " << csvPath.string() << "\n";
        return;
    }

    if (! fileExists) {
        out << "image_id,split,label,area_px,perimeter_px,compactness,circularity,solidity,"
            << "equiv_diameter_px,aspect_ratio,asymmetry_index,mean_luminance,std_luminance,"
            << "mean_b,mean_g,mean_r,bbox_x,bbox_y,bbox_w,bbox_h,success\n";
    }

    const auto& f = result.features;
    const auto& s = result.basicSummary;

    out << item.id << ","
        << melanoma::io::DatasetManager::toString(item.split) << ","
        << melanoma::io::DatasetManager::toString(item.label) << ","
        << std::fixed << std::setprecision(2)
        << f.area << ","
        << f.perimeter << ","
        << f.compactness << ","
        << f.circularity << ","
        << f.solidity << ","
        << f.equivalentDiameter << ","
        << f.aspectRatio << ","
        << f.asymmetryIndex << ","
        << s.meanLuminance << ","
        << s.stdDevLuminance << ","
        << s.meanB << ","
        << s.meanG << ","
        << s.meanR << ","
        << f.boundingBox.x << ","
        << f.boundingBox.y << ","
        << f.boundingBox.width << ","
        << f.boundingBox.height << ","
        << (result.success ? "1" : "0") << "\n";
}

} // namespace

int main(int argc, char** argv) {
    std::filesystem::path datasetPath = "images/input";
    std::filesystem::path singleImagePath;
    std::filesystem::path inputDirPath;
    std::filesystem::path outputDir = "images/results";
    std::filesystem::path csvPath;
    std::size_t samplesPerClass = 2;

    melanoma::core::HairRemovalOptions hairOptions;
    melanoma::core::SegmentationOptions segOptions;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--dataset" && i + 1 < argc) {
            datasetPath = argv[++i];
        } else if (arg == "--image" && i + 1 < argc) {
            singleImagePath = argv[++i];
        } else if (arg == "--input-dir" && i + 1 < argc) {
            inputDirPath = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            outputDir = argv[++i];
        } else if (arg == "--csv" && i + 1 < argc) {
            csvPath = argv[++i];
        } else if (arg == "--samples" && i + 1 < argc) {
            samplesPerClass = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--channel" && i + 1 < argc) {
            std::string ch = argv[++i];
            if (ch == "gray" || ch == "grayscale") {
                segOptions.channel = melanoma::core::SegmentationChannel::Grayscale;
            } else {
                segOptions.channel = melanoma::core::SegmentationChannel::LabL;
            }
        } else if (arg == "--no-inpaint") {
            hairOptions.enabled = false;
        }
    }

    const std::filesystem::path execPath = (argc > 0 && argv[0] != nullptr)
                                               ? std::filesystem::path(argv[0])
                                               : std::filesystem::path{};
    datasetPath = melanoma::io::DatasetManager::resolveDatasetRoot(datasetPath, execPath);

    std::filesystem::path projectRoot = std::filesystem::current_path();
    if (! std::filesystem::exists(projectRoot / "docs" / "7280.jpg")) {
        if (! datasetPath.empty() && std::filesystem::exists(datasetPath.parent_path() / "docs" / "7280.jpg")) {
            projectRoot = datasetPath.parent_path();
        } else if (! execPath.empty() && std::filesystem::exists(execPath.parent_path().parent_path() / "docs" / "7280.jpg")) {
            projectRoot = execPath.parent_path().parent_path();
        }
    }

    if (outputDir.is_relative() && ! std::filesystem::exists(outputDir)) {
        outputDir = projectRoot / outputDir;
    }

    if (csvPath.empty()) {
        csvPath = outputDir / "metrics.csv";
    }

    std::cout << "===============================================================================\n"
              << " Melanoma PDI — Dermoscopic Analysis & Segmentation CLI (Etapa M2)\n"
              << "===============================================================================\n"
              << "OpenCV Version    : " << CV_VERSION << "\n"
              << "Project Root      : " << projectRoot.string() << "\n"
              << "Output Directory  : " << outputDir.string() << "\n"
              << "Metrics CSV       : " << csvPath.string() << "\n"
              << "Segment Channel   : "
              << (segOptions.channel == melanoma::core::SegmentationChannel::LabL ? "CIE L* (Inverted)" : "Grayscale (Inverted)") << "\n"
              << "Hair Inpainting   : " << (hairOptions.enabled ? "Ativado (Black-Hat + Telea)" : "Desativado") << "\n"
              << "-------------------------------------------------------------------------------\n";

    std::vector<ImageItem> imagesToProcess;

    if (! singleImagePath.empty()) {
        std::filesystem::path resolvedSingle = singleImagePath;
        if (! std::filesystem::exists(resolvedSingle) && std::filesystem::exists(projectRoot / singleImagePath)) {
            resolvedSingle = projectRoot / singleImagePath;
        }
        if (! std::filesystem::exists(resolvedSingle)) {
            std::cerr << "[ERRO] Arquivo de imagem nao encontrado: " << singleImagePath.string() << "\n";
            return 1;
        }
        ImageItem item;
        item.path = resolvedSingle;
        item.id = resolvedSingle.stem().string();
        item.split = melanoma::io::DatasetSplit::Test;
        item.label = (item.id.find("mal") != std::string::npos || item.id == "7280")
                         ? melanoma::io::LesionClass::Malignant
                         : melanoma::io::LesionClass::Benign;
        imagesToProcess.push_back(item);
    } else if (! inputDirPath.empty()) {
        std::filesystem::path resolvedDir = inputDirPath;
        if (! std::filesystem::exists(resolvedDir) && std::filesystem::exists(projectRoot / inputDirPath)) {
            resolvedDir = projectRoot / inputDirPath;
        }
        if (! std::filesystem::exists(resolvedDir) || ! std::filesystem::is_directory(resolvedDir)) {
            std::cerr << "[ERRO] Diretorio de entrada invalido: " << inputDirPath.string() << "\n";
            return 1;
        }
        for (const auto& entry : std::filesystem::directory_iterator(resolvedDir)) {
            if (entry.is_regular_file()) {
                const auto ext = entry.path().extension().string();
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
                    ImageItem item;
                    item.path = entry.path();
                    item.id = entry.path().stem().string();
                    item.split = melanoma::io::DatasetSplit::Test;
                    item.label = melanoma::io::LesionClass::Benign;
                    imagesToProcess.push_back(item);
                }
            }
        }
    } else {
        const auto report = melanoma::io::DatasetManager::inspectDataset(datasetPath);
        if (report.isValid()) {
            std::cout << "Dataset completo encontrado com " << report.totalCount() << " imagens.\n"
                      << "Carregando " << samplesPerClass << " amostras por classe...\n";
            const auto samples = melanoma::io::DatasetManager::listImages(datasetPath, std::nullopt, samplesPerClass);
            for (const auto& s : samples) {
                ImageItem item;
                item.path = s.filePath;
                item.id = s.id;
                item.split = s.split;
                item.label = s.label;
                imagesToProcess.push_back(item);
            }
        } else {
            std::cout << "[INFO] Dataset completo ausente em '" << datasetPath.string() << "'.\n"
                      << "       Carregando imagens de demonstracao em 'docs/' e 'images/input/'...\n";
            std::vector<std::filesystem::path> demoCandidates = {
                projectRoot / "docs" / "7280.jpg",
                projectRoot / "docs" / "6068.jpg",
                projectRoot / "images" / "input" / "7280.jpg",
                projectRoot / "images" / "input" / "6068.jpg"
            };

            for (const auto& candidate : demoCandidates) {
                if (std::filesystem::exists(candidate)) {
                    ImageItem item;
                    item.path = candidate;
                    item.id = candidate.stem().string();
                    item.split = melanoma::io::DatasetSplit::Test;
                    item.label = (item.id == "7280") ? melanoma::io::LesionClass::Malignant
                                                     : melanoma::io::LesionClass::Benign;
                    imagesToProcess.push_back(item);
                }
            }

            if (imagesToProcess.empty()) {
                std::cerr << "\n[ERRO] Nenhuma imagem encontrada para processamento.\n"
                          << "Execute ./scripts/download_dataset.sh para obter o dataset ou passe --image <caminho>.\n";
                return 1;
            }
        }
    }

    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    if (ec) {
        std::cerr << "[ERRO] Nao foi possivel criar diretorio de saida: " << outputDir.string() << "\n";
        return 1;
    }

    std::cout << "Processando " << imagesToProcess.size() << " imagens...\n";
    std::cout << "---------------------------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(8)  << "ID"
              << std::setw(12) << "Classe"
              << std::setw(10) << "Area(px)"
              << std::setw(11) << "Perim(px)"
              << std::setw(10) << "Compac"
              << std::setw(8)  << "Circ"
              << std::setw(8)  << "Solid"
              << std::setw(8)  << "Asymm"
              << "Status\n";
    std::cout << "---------------------------------------------------------------------------------------------------\n";

    std::size_t successCount = 0;

    for (const auto& item : imagesToProcess) {
        cv::Mat bgr = cv::imread(item.path.string(), cv::IMREAD_COLOR);
        if (bgr.empty()) {
            std::cerr << std::left << std::setw(8) << item.id
                      << " [FALHA DE LEITURA] " << item.path.string() << "\n";
            continue;
        }

        const auto result = melanoma::core::ImageProcessor::processPipeline(
            bgr, item.id, item.split, item.label, hairOptions, segOptions);

        if (result.success) {
            ++successCount;
        }

        // Salva artefatos intermediarios e finais
        const std::string prefix = (outputDir / item.id).string();
        cv::imwrite(prefix + "_1_hair_mask.png", result.artifacts.hairMask);
        cv::imwrite(prefix + "_2_inpainted.png", result.artifacts.inpainted);
        cv::imwrite(prefix + "_3_channel.png", result.artifacts.segmentationChannel);
        cv::imwrite(prefix + "_4_raw_otsu.png", result.artifacts.rawOtsuMask);
        cv::imwrite(prefix + "_5_final_mask.png", result.artifacts.finalMask);
        cv::imwrite(prefix + "_6_contour_overlay.png", result.artifacts.contourOverlay);

        appendMetricsToCsv(csvPath, item, result);

        const auto& f = result.features;
        std::cout << std::left << std::fixed
                  << std::setw(8)  << item.id
                  << std::setw(12) << melanoma::io::DatasetManager::toString(item.label)
                  << std::setw(10) << std::setprecision(0) << f.area
                  << std::setw(11) << std::setprecision(1) << f.perimeter
                  << std::setw(10) << std::setprecision(2) << f.compactness
                  << std::setw(8)  << std::setprecision(2) << f.circularity
                  << std::setw(8)  << std::setprecision(2) << f.solidity
                  << std::setw(8)  << std::setprecision(2) << f.asymmetryIndex
                  << (result.success ? "[OK]" : "[AVISO: Sem Regiao]") << "\n";
    }

    std::cout << "---------------------------------------------------------------------------------------------------\n";
    std::cout << "Processamento concluido: " << successCount << "/" << imagesToProcess.size()
              << " imagens segmentadas com sucesso.\n";
    std::cout << "Artefatos visuais e metricas salvos em:\n"
              << "  -> Imagens intermediarias : " << outputDir.string() << "\n"
              << "  -> Relatorio de metricas  : " << csvPath.string() << "\n"
              << "===============================================================================\n";

    return 0;
}

#pragma once

#include <cmath>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include <opencv2/core.hpp>

#include "io/DatasetManager.hpp"

namespace melanoma::core {

enum class SegmentationChannel {
    Grayscale,
    LabL
};

struct HairRemovalOptions {
    int kernelSize = 17;        // Tamanho do elemento estruturante Black-Hat
    double threshold = 12.0;    // Limiar sobre a resposta do Black-Hat
    int inpaintRadius = 3;      // Raio de vizinhança para inpainting
    bool enabled = true;
};

struct SegmentationOptions {
    SegmentationChannel channel = SegmentationChannel::LabL;
    int morphCloseKernelSize = 9;  // Fechamento morfológico para unir regiões
    int morphOpenKernelSize = 5;   // Abertura morfológica para eliminar espículas
    bool keepLargestComponentOnly = true;
    bool fillHoles = true;
};

struct MorphologicalFeatures {
    double area = 0.0;               // Área em pixels
    double perimeter = 0.0;          // Perímetro em pixels
    double compactness = 0.0;        // P^2 / (4 * pi * A) (Polsby-Popper, >= 1.0)
    double circularity = 0.0;        // 4 * pi * A / P^2 (<= 1.0, circulo = 1.0)
    double equivalentDiameter = 0.0; // sqrt(4 * A / pi)
    double solidity = 0.0;           // Area / Area do Fecho Convexo
    double aspectRatio = 0.0;        // Largura / Altura da Bounding Box
    cv::Rect boundingBox;
    cv::Point2d centroid{0.0, 0.0};
    double asymmetryIndex = 0.0;     // Razão de não-sobreposição ao dobrar sobre o eixo principal
    bool valid = false;
};

struct PipelineArtifacts {
    cv::Mat original;
    cv::Mat hairMask;
    cv::Mat inpainted;
    cv::Mat segmentationChannel;
    cv::Mat rawOtsuMask;
    cv::Mat finalMask;
    cv::Mat contourOverlay;
};

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

struct PipelineResult {
    ProcessingSummary basicSummary;
    MorphologicalFeatures features;
    PipelineArtifacts artifacts;
    bool success = false;
    std::string errorMessage;
};

class ImageProcessor {
public:
    static constexpr int kExpectedWidth = 224;
    static constexpr int kExpectedHeight = 224;
    static constexpr int kExpectedChannels = 3;

    // Processamento estatístico básico (compatibilidade M1)
    static ProcessingSummary processBasic(const io::ImageEntry& entry, const cv::Mat& bgrImage);

    // 1. Detecção e remoção morfológica de pelos (Black-Hat + Inpainting)
    static cv::Mat detectHairs(const cv::Mat& bgrImage, const HairRemovalOptions& options = {});
    static cv::Mat inpaintHairs(const cv::Mat& bgrImage, const cv::Mat& hairMask, int inpaintRadius = 3);
    static std::pair<cv::Mat, cv::Mat> removeHairs(const cv::Mat& bgrImage, const HairRemovalOptions& options = {});

    // 2. Segmentação (Otsu em L* ou Cinza) e Pós-processamento Morfológico
    static cv::Mat prepareSegmentationChannel(const cv::Mat& bgrImage, SegmentationChannel channel);
    static cv::Mat segmentOtsu(const cv::Mat& channelImage);
    static cv::Mat applyMorphologyAndFilter(const cv::Mat& rawMask, const SegmentationOptions& options = {});
    static cv::Mat keepLargestConnectedComponent(const cv::Mat& binaryMask);
    static cv::Mat fillMaskHoles(const cv::Mat& binaryMask);

    // 3. Extração de características e descritores morfológicos
    static MorphologicalFeatures extractFeatures(const cv::Mat& binaryMask);
    static cv::Mat createContourOverlay(const cv::Mat& backgroundBgr, const cv::Mat& binaryMask,
                                        const cv::Scalar& contourColor = cv::Scalar(0, 255, 0),
                                        int thickness = 2);

    // Pipeline completo integrado (Entrada -> Pré-processamento -> Segmentação -> Morfologia -> Métricas)
    static PipelineResult processPipeline(const cv::Mat& bgrImage,
                                          const std::string& imageId = "sample",
                                          io::DatasetSplit split = io::DatasetSplit::Train,
                                          io::LesionClass label = io::LesionClass::Benign,
                                          const HairRemovalOptions& hairOptions = {},
                                          const SegmentationOptions& segOptions = {});
};

} // namespace melanoma::core

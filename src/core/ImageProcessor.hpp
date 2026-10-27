#pragma once

#include <array>
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

enum class SegmentationMethod {
    Otsu,
    GrabCut
};

struct HairRemovalOptions {
    int kernelSize = 17;            // Comprimento dos elementos estruturantes lineares Black-Hat
    double threshold = 12.0;        // Limiar sobre a resposta direcional do Black-Hat
    int inpaintRadius = 3;          // Raio de vizinhança para inpainting de Telea
    bool applyBilateral = true;     // Suavização bilateral preservadora de bordas pós-inpainting
    bool enabled = true;
};

struct SegmentationOptions {
    SegmentationChannel channel = SegmentationChannel::LabL;
    SegmentationMethod method = SegmentationMethod::Otsu;
    int morphCloseKernelSize = 9;   // Fechamento morfológico para unir regiões
    int morphOpenKernelSize = 5;    // Abertura morfológica para eliminar espículas
    bool suppressVignette = true;   // Supressão de vinhetamento periférico e correção de iluminação
    bool keepLargestComponentOnly = true;
    bool fillHoles = true;
};

struct MorphologicalFeatures {
    // D - Dimensões e Diâmetro
    double area = 0.0;                  // Área em pixels
    double perimeter = 0.0;             // Perímetro em pixels
    double equivalentDiameter = 0.0;    // sqrt(4 * A / pi)
    double majorAxisPx = 0.0;           // Eixo maior da elipse ajustada (cv::fitEllipse)
    double minorAxisPx = 0.0;           // Eixo menor da elipse ajustada (cv::fitEllipse)
    double aspectRatio = 0.0;           // Largura / Altura da Bounding Box
    cv::Rect boundingBox;
    cv::Point2d centroid{0.0, 0.0};

    // B - Irregularidade de Borda
    double compactness = 0.0;           // P^2 / (4 * pi * A) (Polsby-Popper, >= 1.0)
    double circularity = 0.0;           // 4 * pi * A / P^2 (<= 1.0, círculo = 1.0)
    double solidity = 0.0;              // Área / Área do Fecho Convexo
    double borderRadialVariance = 0.0;  // Variância radial normalizada do centroide ao contorno

    // A - Assimetria
    double asymmetryMajor = 0.0;        // Assimetria sobre o eixo principal maior [0, 1]
    double asymmetryMinor = 0.0;        // Assimetria sobre o eixo principal menor [0, 1]
    double asymmetryIndex = 0.0;        // Índice médio de assimetria biaxial [0, 1]
    std::array<double, 7> huMoments{};  // 7 Momentos Invariantes de Hu (escala log)

    bool valid = false;
};

struct ColorFeatures {
    // C - Variação Cromática restrita à máscara da lesão (ROI)
    double lesionMeanB = 0.0;
    double lesionMeanG = 0.0;
    double lesionMeanR = 0.0;
    double lesionStdB = 0.0;
    double lesionStdG = 0.0;
    double lesionStdR = 0.0;

    double lesionMeanL = 0.0;
    double lesionStdL = 0.0;
    double lesionMeanA = 0.0;
    double lesionStdA = 0.0;
    double lesionMeanBStar = 0.0;
    double lesionStdBStar = 0.0;

    double lesionMeanH = 0.0;
    double lesionStdH = 0.0;
    double lesionMeanS = 0.0;
    double lesionStdS = 0.0;
    double lesionMeanV = 0.0;
    double lesionStdV = 0.0;

    int colorClusters = 0;              // Quantidade de grupos cromáticos clínicos ABCD (1 a 6)
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
    ColorFeatures colorFeatures;
    PipelineArtifacts artifacts;
    bool success = false;
    std::string errorMessage;
};

struct SegmentationComparisonMetrics {
    double iou = 0.0;                   // Intersection over Union (Jaccard) em [0, 1]
    double dice = 0.0;                  // Coeficiente de Sorensen-Dice em [0, 1]
    double precision = 0.0;             // Precisão de pixels em [0, 1]
    double recall = 0.0;                // Sensibilidade (Recall) de pixels em [0, 1]
    int intersectionPixels = 0;
    int unionPixels = 0;
};

class ImageProcessor {
public:
    static constexpr int kExpectedWidth = 224;
    static constexpr int kExpectedHeight = 224;
    static constexpr int kExpectedChannels = 3;

    // Processamento estatístico básico da imagem inteira
    static ProcessingSummary processBasic(const io::ImageEntry& entry, const cv::Mat& bgrImage);

    // 1. Detecção e remoção morfológica de pelos (DullRazor multi-angular + Inpainting + Bilateral)
    static cv::Mat detectHairs(const cv::Mat& bgrImage, const HairRemovalOptions& options = {});
    static cv::Mat inpaintHairs(const cv::Mat& bgrImage, const cv::Mat& hairMask, int inpaintRadius = 3);
    static cv::Mat applyBilateralFilter(const cv::Mat& bgrImage);
    static std::pair<cv::Mat, cv::Mat> removeHairs(const cv::Mat& bgrImage, const HairRemovalOptions& options = {});

    // 2. Segmentação (Otsu / GrabCut com supressão de vinhetamento) e Pós-processamento Morfológico
    static cv::Mat createFieldOfViewMask(const cv::Mat& bgrImage);
    static cv::Mat prepareSegmentationChannel(const cv::Mat& bgrImage,
                                              SegmentationChannel channel,
                                              bool suppressVignette = true);
    static cv::Mat segmentOtsu(const cv::Mat& channelImage, const cv::Mat& fovMask = {});
    static cv::Mat segmentGrabCut(const cv::Mat& bgrImage, const cv::Mat& seedMask);
    static cv::Mat applyMorphologyAndFilter(const cv::Mat& rawMask, const SegmentationOptions& options = {});
    static cv::Mat keepLargestConnectedComponent(const cv::Mat& binaryMask);
    static cv::Mat fillMaskHoles(const cv::Mat& binaryMask);

    // 3. Extração de características e descritores ABCD (Forma, Assimetria, Borda, Cor e Diâmetro)
    static MorphologicalFeatures extractFeatures(const cv::Mat& binaryMask);
    static ColorFeatures extractColorFeatures(const cv::Mat& bgrImage, const cv::Mat& binaryMask);
    static SegmentationComparisonMetrics compareMasks(const cv::Mat& predictedMask, const cv::Mat& referenceMask);
    static cv::Mat createContourOverlay(const cv::Mat& backgroundBgr, const cv::Mat& binaryMask,
                                        const cv::Scalar& contourColor = cv::Scalar(0, 255, 0),
                                        int thickness = 2);

    // Pipeline completo integrado (Entrada -> Pré-processamento -> Segmentação -> Morfologia -> ABCD)
    static PipelineResult processPipeline(const cv::Mat& bgrImage,
                                          const std::string& imageId = "sample",
                                          io::DatasetSplit split = io::DatasetSplit::Train,
                                          io::LesionClass label = io::LesionClass::Benign,
                                          const HairRemovalOptions& hairOptions = {},
                                          const SegmentationOptions& segOptions = {});
};

} // namespace melanoma::core

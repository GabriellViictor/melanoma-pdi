#include "core/ImageProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>

namespace melanoma::core {

namespace {
constexpr double kPi = 3.14159265358979323846;
} // namespace

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

cv::Mat ImageProcessor::detectHairs(const cv::Mat& bgrImage, const HairRemovalOptions& options) {
    if (bgrImage.empty()) {
        return cv::Mat();
    }

    cv::Mat gray;
    if (bgrImage.channels() == 3) {
        cv::cvtColor(bgrImage, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = bgrImage.clone();
    }

    int ksize = options.kernelSize;
    if (ksize < 3) ksize = 3;
    if (ksize % 2 == 0) ksize += 1;

    // Operação morfológica Black-Hat para destacar estruturas escuras finas e alongadas (pelos)
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(ksize, ksize));
    cv::Mat blackHat;
    cv::morphologyEx(gray, blackHat, cv::MORPH_BLACKHAT, kernel);

    // Binarização da resposta do Black-Hat
    cv::Mat hairMask;
    cv::threshold(blackHat, hairMask, options.threshold, 255, cv::THRESH_BINARY);

    // Dilatação leve para englobar as bordas dos pelos durante o inpainting
    cv::Mat dilateKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(hairMask, hairMask, dilateKernel);

    return hairMask;
}

cv::Mat ImageProcessor::inpaintHairs(const cv::Mat& bgrImage, const cv::Mat& hairMask, int inpaintRadius) {
    if (bgrImage.empty()) {
        return cv::Mat();
    }
    if (hairMask.empty() || cv::countNonZero(hairMask) == 0) {
        return bgrImage.clone();
    }

    cv::Mat inpainted;
    cv::inpaint(bgrImage, hairMask, inpainted, static_cast<double>(inpaintRadius), cv::INPAINT_TELEA);
    return inpainted;
}

std::pair<cv::Mat, cv::Mat> ImageProcessor::removeHairs(const cv::Mat& bgrImage, const HairRemovalOptions& options) {
    if (! options.enabled) {
        cv::Mat emptyMask = cv::Mat::zeros(bgrImage.size(), CV_8UC1);
        return {emptyMask, bgrImage.clone()};
    }

    cv::Mat hairMask = detectHairs(bgrImage, options);
    cv::Mat inpainted = inpaintHairs(bgrImage, hairMask, options.inpaintRadius);
    return {hairMask, inpainted};
}

cv::Mat ImageProcessor::prepareSegmentationChannel(const cv::Mat& bgrImage, SegmentationChannel channel) {
    if (bgrImage.empty()) {
        return cv::Mat();
    }

    if (channel == SegmentationChannel::LabL) {
        cv::Mat lab;
        cv::cvtColor(bgrImage, lab, cv::COLOR_BGR2Lab);
        std::vector<cv::Mat> planes;
        cv::split(lab, planes);
        // Canal L representa luminância (0 a 255). A lesão é escura e a pele sadia é clara.
        // Invertemos (255 - L) para que a lesão se torne o primeiro plano brilhante.
        cv::Mat invL = 255 - planes[0];
        return invL;
    }

    // Default: Escala de Cinza invertida
    cv::Mat gray;
    if (bgrImage.channels() == 3) {
        cv::cvtColor(bgrImage, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = bgrImage.clone();
    }
    cv::Mat invGray = 255 - gray;
    return invGray;
}

cv::Mat ImageProcessor::segmentOtsu(const cv::Mat& channelImage) {
    if (channelImage.empty()) {
        return cv::Mat();
    }

    // Suavização gaussiana preliminar para atenuar ruídos de alta frequência
    cv::Mat blurred;
    cv::GaussianBlur(channelImage, blurred, cv::Size(5, 5), 0);

    cv::Mat rawMask;
    cv::threshold(blurred, rawMask, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    return rawMask;
}

cv::Mat ImageProcessor::keepLargestConnectedComponent(const cv::Mat& binaryMask) {
    if (binaryMask.empty()) {
        return cv::Mat();
    }

    cv::Mat labels, stats, centroids;
    int nLabels = cv::connectedComponentsWithStats(binaryMask, labels, stats, centroids);
    if (nLabels <= 1) {
        return binaryMask.clone();
    }

    int bestLabel = -1;
    int maxArea = 0;
    for (int i = 1; i < nLabels; ++i) {
        int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area > maxArea) {
            maxArea = area;
            bestLabel = i;
        }
    }

    if (bestLabel < 0) {
        return cv::Mat::zeros(binaryMask.size(), CV_8UC1);
    }

    cv::Mat cleanMask = (labels == bestLabel);
    return cleanMask;
}

cv::Mat ImageProcessor::fillMaskHoles(const cv::Mat& binaryMask) {
    if (binaryMask.empty()) {
        return cv::Mat();
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binaryMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat filled = cv::Mat::zeros(binaryMask.size(), CV_8UC1);
    if (! contours.empty()) {
        int largestIdx = 0;
        double largestArea = 0.0;
        for (std::size_t i = 0; i < contours.size(); ++i) {
            double a = cv::contourArea(contours[i]);
            if (a > largestArea) {
                largestArea = a;
                largestIdx = static_cast<int>(i);
            }
        }
        cv::drawContours(filled, contours, largestIdx, cv::Scalar(255), cv::FILLED);
    } else {
        filled = binaryMask.clone();
    }
    return filled;
}

cv::Mat ImageProcessor::applyMorphologyAndFilter(const cv::Mat& rawMask, const SegmentationOptions& options) {
    if (rawMask.empty()) {
        return cv::Mat();
    }

    cv::Mat morph = rawMask.clone();

    // 1. Fechamento morfológico para conectar descontinuidades causadas por pelos ou despigmentação
    if (options.morphCloseKernelSize > 1) {
        int csize = options.morphCloseKernelSize;
        if (csize % 2 == 0) csize += 1;
        cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(csize, csize));
        cv::morphologyEx(morph, morph, cv::MORPH_CLOSE, closeKernel);
    }

    // 2. Abertura morfológica para remover espículas e ilhas de falso positivo
    if (options.morphOpenKernelSize > 1) {
        int osize = options.morphOpenKernelSize;
        if (osize % 2 == 0) osize += 1;
        cv::Mat openKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(osize, osize));
        cv::morphologyEx(morph, morph, cv::MORPH_OPEN, openKernel);
    }

    // 3. Reter somente o maior componente conexo (elimina artefatos de vinheta periférica)
    if (options.keepLargestComponentOnly) {
        morph = keepLargestConnectedComponent(morph);
    }

    // 4. Preencher orifícios residuais na máscara da lesão
    if (options.fillHoles) {
        morph = fillMaskHoles(morph);
    }

    return morph;
}

MorphologicalFeatures ImageProcessor::extractFeatures(const cv::Mat& binaryMask) {
    MorphologicalFeatures feat;
    if (binaryMask.empty()) {
        return feat;
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binaryMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return feat;
    }

    int bestIdx = 0;
    double bestArea = 0.0;
    for (std::size_t i = 0; i < contours.size(); ++i) {
        double a = cv::contourArea(contours[i]);
        if (a > bestArea) {
            bestArea = a;
            bestIdx = static_cast<int>(i);
        }
    }

    const auto& mainContour = contours[bestIdx];
    double area = bestArea;
    if (area <= 0.0) {
        area = static_cast<double>(cv::countNonZero(binaryMask));
    }

    double perimeter = cv::arcLength(mainContour, true);

    feat.area = area;
    feat.perimeter = perimeter;
    // Compacidade (Polsby-Popper): P^2 / (4 * pi * A). Circulo perfeito = 1.0; bordas recortadas > 1.0.
    feat.compactness = (area > 0.0) ? (perimeter * perimeter) / (4.0 * kPi * area) : 0.0;
    // Circularidade: 4 * pi * A / P^2. Circulo perfeito = 1.0; formas irregulares < 1.0.
    feat.circularity = (perimeter > 0.0) ? (4.0 * kPi * area) / (perimeter * perimeter) : 0.0;
    feat.equivalentDiameter = (area > 0.0) ? std::sqrt(4.0 * area / kPi) : 0.0;
    feat.boundingBox = cv::boundingRect(mainContour);
    feat.aspectRatio = (feat.boundingBox.height > 0)
                           ? static_cast<double>(feat.boundingBox.width) / feat.boundingBox.height
                           : 0.0;

    // Fecho convexo e Solidez (Area / Area do Hull)
    std::vector<cv::Point> hull;
    cv::convexHull(mainContour, hull);
    double hullArea = cv::contourArea(hull);
    feat.solidity = (hullArea > 0.0) ? (area / hullArea) : 0.0;

    // Momentos espaciais e Centroide
    cv::Moments m = cv::moments(mainContour);
    if (m.m00 > 0.0) {
        feat.centroid = cv::Point2d(m.m10 / m.m00, m.m01 / m.m00);
    }

    // Índice de Assimetria via alinhamento pelo eixo principal e cálculo de XOR
    if (area > 20.0 && m.m00 > 0.0) {
        double angle = 0.5 * std::atan2(2.0 * m.mu11, m.mu20 - m.mu02);
        cv::Mat rotMat = cv::getRotationMatrix2D(feat.centroid, angle * 180.0 / kPi, 1.0);
        cv::Mat alignedMask;
        cv::warpAffine(binaryMask, alignedMask, rotMat, binaryMask.size(), cv::INTER_NEAREST);

        cv::Mat flippedH;
        cv::flip(alignedMask, flippedH, 1);
        cv::Mat diff;
        cv::bitwise_xor(alignedMask, flippedH, diff);

        double diffCount = static_cast<double>(cv::countNonZero(diff));
        double totalCount = static_cast<double>(cv::countNonZero(alignedMask));
        feat.asymmetryIndex = (totalCount > 0.0) ? (diffCount / totalCount) : 0.0;
    }

    feat.valid = (feat.area > 0.0);
    return feat;
}

cv::Mat ImageProcessor::createContourOverlay(const cv::Mat& backgroundBgr, const cv::Mat& binaryMask,
                                            const cv::Scalar& contourColor, int thickness) {
    if (backgroundBgr.empty()) {
        return cv::Mat();
    }

    cv::Mat overlay = backgroundBgr.clone();
    if (binaryMask.empty()) {
        return overlay;
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binaryMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (! contours.empty()) {
        cv::drawContours(overlay, contours, -1, contourColor, thickness);
    }

    return overlay;
}

PipelineResult ImageProcessor::processPipeline(const cv::Mat& bgrImage,
                                              const std::string& imageId,
                                              io::DatasetSplit split,
                                              io::LesionClass label,
                                              const HairRemovalOptions& hairOptions,
                                              const SegmentationOptions& segOptions) {
    PipelineResult res;
    io::ImageEntry mockEntry{"", imageId, split, label};
    res.basicSummary = processBasic(mockEntry, bgrImage);

    if (! res.basicSummary.valid) {
        res.success = false;
        res.errorMessage = "Imagem de entrada vazia ou invalida";
        return res;
    }

    res.artifacts.original = bgrImage.clone();

    // 1. Remoção de pelos
    auto [hairMask, inpainted] = removeHairs(bgrImage, hairOptions);
    res.artifacts.hairMask = hairMask;
    res.artifacts.inpainted = inpainted;

    // 2. Preparação do canal e segmentação Otsu
    cv::Mat channelImg = prepareSegmentationChannel(inpainted, segOptions.channel);
    res.artifacts.segmentationChannel = channelImg;

    cv::Mat rawOtsu = segmentOtsu(channelImg);
    res.artifacts.rawOtsuMask = rawOtsu;

    // 3. Pós-processamento morfológico
    cv::Mat finalMask = applyMorphologyAndFilter(rawOtsu, segOptions);
    res.artifacts.finalMask = finalMask;

    // 4. Extração de características
    res.features = extractFeatures(finalMask);

    // 5. Overlay com contorno
    res.artifacts.contourOverlay = createContourOverlay(inpainted, finalMask, cv::Scalar(0, 255, 0), 2);

    res.success = res.features.valid;
    if (! res.success) {
        res.errorMessage = "Nenhum contorno ou regiao valida foi detectada na segmentacao";
    }

    return res;
}

} // namespace melanoma::core

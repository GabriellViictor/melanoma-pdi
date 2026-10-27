#include "core/ImageProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#if __has_include(<opencv2/geometry.hpp>)
#include <opencv2/geometry.hpp>
#endif

namespace melanoma::core {

namespace {

constexpr double kPi = 3.14159265358979323846;

std::vector<cv::Mat> createDirectionalHairKernels(int ksize) {
    std::vector<cv::Mat> kernels;
    kernels.reserve(8);
    const int center = ksize / 2;
    const double radius = ksize * 0.5;

    // 8 orientações (0 a 157.5 graus, passo de 22.5 graus) para abertura linear
    for (int i = 0; i < 8; ++i) {
        const double angle = i * (kPi / 8.0);
        const int dx = static_cast<int>(std::round(std::cos(angle) * radius));
        const int dy = static_cast<int>(std::round(std::sin(angle) * radius));
        cv::Mat k = cv::Mat::zeros(ksize, ksize, CV_8UC1);
        cv::line(k,
                 cv::Point(std::clamp(center - dx, 0, ksize - 1), std::clamp(center - dy, 0, ksize - 1)),
                 cv::Point(std::clamp(center + dx, 0, ksize - 1), std::clamp(center + dy, 0, ksize - 1)),
                 cv::Scalar(1), 1);
        kernels.push_back(k);
    }

    return kernels;
}

double computeMedianInMask(const cv::Mat& singleChannel8U, const cv::Mat& mask) {
    std::vector<int> hist(256, 0);
    int total = 0;

    for (int y = 0; y < singleChannel8U.rows; ++y) {
        const uchar* rowPtr = singleChannel8U.ptr<uchar>(y);
        const uchar* maskPtr = mask.empty() ? nullptr : mask.ptr<uchar>(y);
        for (int x = 0; x < singleChannel8U.cols; ++x) {
            if (maskPtr == nullptr || maskPtr[x] > 0) {
                ++hist[rowPtr[x]];
                ++total;
            }
        }
    }

    if (total == 0) {
        return 128.0;
    }

    int cumulative = 0;
    const int half = total / 2;
    for (int i = 0; i < 256; ++i) {
        cumulative += hist[i];
        if (cumulative >= half) {
            return static_cast<double>(i);
        }
    }
    return 128.0;
}

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

    // Metodologia DullRazor em dois estágios com proteção de núcleo de lesão:
    // Estágio 1 — Black-Hat isotrópico de seção estreita (7x7):
    //   Um elemento elíptico 7x7 fecha vales com espessura <= 5 px (todos os fios de cabelo),
    //   mas NÃO fecha corpos sólidos mais largos (> 6 px, como o núcleo escuro de uma lesão).
    //   Assim, `thinValleys` responde intensamente sobre fios finos e ignora o interior da lesão!
    cv::Mat crossSectionKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    cv::Mat thinValleys;
    cv::morphologyEx(gray, thinValleys, cv::MORPH_BLACKHAT, crossSectionKernel);

    // Estágio 2 — Realce de colinearidade direcional (8 orientações, comprimento 9 px):
    //   Para cada orientação theta, compara a continuidade do vale na direção paralela (theta)
    //   com a direção ortogonal (theta + 90 graus).
    //   - Em fios de cabelo: `thinValleys` é contínuo ao longo de theta (fechamento paralelo alto)
    //     e estreito na direção ortogonal (erosão ortogonal zero).
    //   - Nas bordas ou espículas curtas da lesão: a resposta ortogonal também é significativa,
    //     suprimindo falsos positivos ao redor do núcleo pigmentado.
    const auto lineKernels = createDirectionalHairKernels(9);
    cv::Mat fiberResponse = cv::Mat::zeros(gray.size(), CV_8UC1);

    for (int i = 0; i < 8; ++i) {
        const cv::Mat& kParallel = lineKernels[i];
        const cv::Mat& kOrtho = lineKernels[(i + 4) % 8];

        cv::Mat parallelClosed, orthoEroded;
        cv::morphologyEx(thinValleys, parallelClosed, cv::MORPH_CLOSE, kParallel);
        cv::erode(thinValleys, orthoEroded, kOrtho);

        cv::Mat dirDiff;
        cv::subtract(parallelClosed, orthoEroded, dirDiff);
        cv::max(fiberResponse, dirDiff, fiberResponse);
    }

    // Limita `fiberResponse` aos pixels que realmente eram vales no Black-Hat original
    // (impede que o fechamento linear crie pontes falsas através do interior da lesão)
    cv::min(fiberResponse, thinValleys, fiberResponse);

    // Proteção explícita de núcleo sólido de lesão: uma mancha muito escura e compacta que sobrevive
    // ao fechamento 7x7 E à abertura 7x7 com queda abrupta (> 55 níveis abaixo da pele) é um núcleo
    // maciço de lesão (ex.: nevo muito escuro), onde não existem fios mais escuros distinguíveis.
    // Já em lesões castanhas intermediárias cobertas por fios pretos, `thinValleys` é alto sobre os fios
    // (>= 14), permitindo remover os fios mesmo sobre o centro da lesão!
    cv::Mat closedDisk7, solidDarkBlob;
    cv::morphologyEx(gray, closedDisk7, cv::MORPH_CLOSE, crossSectionKernel);
    cv::morphologyEx(closedDisk7, solidDarkBlob, cv::MORPH_OPEN, crossSectionKernel);
    const cv::Scalar meanGray = cv::mean(gray);
    cv::Mat solidLesionCore;
    cv::threshold(solidDarkBlob, solidLesionCore, meanGray[0] - 48.0, 255, cv::THRESH_BINARY_INV);
    cv::Mat coreErode = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::erode(solidLesionCore, solidLesionCore, coreErode);

    const double effectiveThresh = std::min(options.threshold, 8.0);
    cv::Mat rawCandidates;
    cv::threshold(fiberResponse, rawCandidates, effectiveThresh, 255, cv::THRESH_BINARY);
    rawCandidates.setTo(0, solidLesionCore);

    // Zera candidatos na moldura externa de 2 px
    rawCandidates.rowRange(0, 2).setTo(0);
    rawCandidates.rowRange(rawCandidates.rows - 2, rawCandidates.rows).setTo(0);
    rawCandidates.colRange(0, 2).setTo(0);
    rawCandidates.colRange(rawCandidates.cols - 2, rawCandidates.cols).setTo(0);

    // Estágio 3 — Filtragem geométrica de componentes conexos (sem preencher laços fechados):
    //   IMPORTANTE: nunca usar cv::drawContours(..., cv::FILLED), pois dois pelos que se cruzam
    //   formam um polígono fechado e cv::FILLED pintaria toda a pele/lesão entre eles!
    //   Usamos connectedComponentsWithStats para copiar apenas os pixels exatos dos fios validados.
    cv::Mat labels, stats, centroids;
    const int numLabels = cv::connectedComponentsWithStats(rawCandidates, labels, stats, centroids, 8, CV_32S);

    cv::Mat verifiedHairMask = cv::Mat::zeros(gray.size(), CV_8UC1);
    for (int label = 1; label < numLabels; ++label) {
        const int area = stats.at<int>(label, cv::CC_STAT_AREA);
        const int bw = stats.at<int>(label, cv::CC_STAT_WIDTH);
        const int bh = stats.at<int>(label, cv::CC_STAT_HEIGHT);
        const int maxSpan = std::max(bw, bh);

        // Descarta poros puntiformes e manchas curtas
        if (area < 12 || maxSpan < 11) {
            continue;
        }

        const double boxArea = static_cast<double>(bw * bh);
        const double fillRatio = (boxArea > 0.0) ? (static_cast<double>(area) / boxArea) : 1.0;
        const double aspect = static_cast<double>(maxSpan) / std::max(1, std::min(bw, bh));

        // Aceita fios retos/inclinados (aspect >= 2.2 ou fillRatio <= 0.45) e redes de fios longos
        const bool isHairStructure = (aspect >= 2.2) || (fillRatio <= 0.45) || (maxSpan >= 18 && fillRatio <= 0.60);
        if (isHairStructure) {
            verifiedHairMask.setTo(255, labels == label);
        }
    }

    // Dilatação suave (3x3) estritamente sobre as fibras capilares validadas
    if (cv::countNonZero(verifiedHairMask) > 0) {
        cv::Mat dilateKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::dilate(verifiedHairMask, verifiedHairMask, dilateKernel);
        verifiedHairMask.setTo(0, solidLesionCore);
    }

    return verifiedHairMask;
}

cv::Mat ImageProcessor::inpaintHairs(const cv::Mat& bgrImage, const cv::Mat& hairMask, int inpaintRadius) {
    if (bgrImage.empty()) {
        return cv::Mat();
    }
    if (hairMask.empty() || cv::countNonZero(hairMask) == 0) {
        return bgrImage.clone();
    }

    cv::Mat inpainted;
    const double radius = static_cast<double>(std::max(1, inpaintRadius));
    cv::inpaint(bgrImage, hairMask, inpainted, radius, cv::INPAINT_TELEA);
    return inpainted;
}

cv::Mat ImageProcessor::applyBilateralFilter(const cv::Mat& bgrImage) {
    if (bgrImage.empty() || bgrImage.channels() != 3) {
        return bgrImage.clone();
    }

    cv::Mat smoothed;
    cv::bilateralFilter(bgrImage, smoothed, 5, 25.0, 25.0);
    return smoothed;
}

std::pair<cv::Mat, cv::Mat> ImageProcessor::removeHairs(const cv::Mat& bgrImage, const HairRemovalOptions& options) {
    if (bgrImage.empty()) {
        return {cv::Mat(), cv::Mat()};
    }

    if (! options.enabled) {
        cv::Mat emptyMask = cv::Mat::zeros(bgrImage.size(), CV_8UC1);
        return {emptyMask, bgrImage.clone()};
    }

    // Mantém a imagem inpainted 100% fiel aos pixels originais fora da máscara de pelos
    cv::Mat hairMask = detectHairs(bgrImage, options);
    cv::Mat inpainted = inpaintHairs(bgrImage, hairMask, options.inpaintRadius);

    return {hairMask, inpainted};
}

cv::Mat ImageProcessor::createFieldOfViewMask(const cv::Mat& bgrImage) {
    if (bgrImage.empty()) {
        return cv::Mat();
    }

    cv::Mat gray;
    if (bgrImage.channels() == 3) {
        cv::cvtColor(bgrImage, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = bgrImage.clone();
    }

    // Identifica regiões válidas excluindo cantos extremamente brancos (ex.: 6068.jpg) ou pretos (vinheta)
    cv::Mat validPixels = (gray > 25) & (gray < 242);

    // Erosão para desconectar o anel periférico de sombra da lente do dermatoscópio e marcas de régua
    cv::Mat erodeKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(15, 15));
    cv::erode(validPixels, validPixels, erodeKernel);

    // Restringe a uma abertura elíptica central segura
    cv::Mat centralAperture = cv::Mat::zeros(bgrImage.size(), CV_8UC1);
    const cv::Point center(bgrImage.cols / 2, bgrImage.rows / 2);
    const cv::Size axes(static_cast<int>(bgrImage.cols * 0.42),
                        static_cast<int>(bgrImage.rows * 0.42));
    cv::ellipse(centralAperture, center, axes, 0.0, 0.0, 360.0, cv::Scalar(255), -1);

    cv::Mat fovMask;
    cv::bitwise_and(validPixels, centralAperture, fovMask);
    if (cv::countNonZero(fovMask) < 400) {
        return centralAperture;
    }
    return fovMask;
}

cv::Mat ImageProcessor::prepareSegmentationChannel(const cv::Mat& bgrImage,
                                                   SegmentationChannel channel,
                                                   bool suppressVignette) {
    if (bgrImage.empty()) {
        return cv::Mat();
    }

    cv::Mat rawSignal;
    if (channel == SegmentationChannel::LabL && bgrImage.channels() == 3) {
        cv::Mat lab;
        cv::cvtColor(bgrImage, lab, cv::COLOR_BGR2Lab);
        std::vector<cv::Mat> planes;
        cv::split(lab, planes);
        // Combina luminância invertida (255 - L*) com cromaticidade melanocítica (a* e b*)
        // para realçar lesões pigmentadas e rejeitar marcações cirúrgicas verdes/azuis
        cv::Mat invL = 255 - planes[0];
        cv::Mat bBoost;
        cv::subtract(planes[2], cv::Scalar(128), bBoost);
        cv::Mat greenPenalty;
        cv::subtract(cv::Scalar(124), planes[1], greenPenalty);
        cv::addWeighted(invL, 0.75, bBoost, 0.65, 0.0, rawSignal);
        cv::subtract(rawSignal, greenPenalty * 2, rawSignal);
    } else {
        cv::Mat gray;
        if (bgrImage.channels() == 3) {
            cv::cvtColor(bgrImage, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = bgrImage.clone();
        }
        rawSignal = 255 - gray;
    }

    if (! suppressVignette) {
        return rawSignal;
    }

    const cv::Mat fovMask = createFieldOfViewMask(bgrImage);
    const double skinMedian = computeMedianInMask(rawSignal, fovMask);

    // Substitui cantos de vinheta fora do FoV pela mediana da pele para não enviesar o filtro de iluminação
    cv::Mat maskedSignal = rawSignal.clone();
    maskedSignal.setTo(cv::Scalar(skinMedian), fovMask == 0);

    // Correção de iluminação (flat-field): atenua gradientes lentos de sombra nas bordas
    cv::Mat illuminationBg;
    cv::GaussianBlur(maskedSignal, illuminationBg, cv::Size(0, 0), 30.0);

    cv::Mat corrected;
    cv::addWeighted(maskedSignal, 1.0, illuminationBg, -0.70, skinMedian * 0.70, corrected);
    corrected.setTo(cv::Scalar(0), fovMask == 0);

    return corrected;
}

cv::Mat ImageProcessor::segmentOtsu(const cv::Mat& channelImage, const cv::Mat& fovMask) {
    if (channelImage.empty()) {
        return cv::Mat();
    }

    cv::Mat blurred;
    cv::GaussianBlur(channelImage, blurred, cv::Size(5, 5), 0);

    // Calcula o limiar de Otsu considerando apenas os pixels válidos dentro do FoV
    std::vector<uchar> validPixels;
    validPixels.reserve(static_cast<std::size_t>(blurred.rows * blurred.cols));
    double sum = 0.0;
    double sumSq = 0.0;

    for (int y = 0; y < blurred.rows; ++y) {
        const uchar* rowPtr = blurred.ptr<uchar>(y);
        const uchar* maskPtr = fovMask.empty() ? nullptr : fovMask.ptr<uchar>(y);
        for (int x = 0; x < blurred.cols; ++x) {
            if ((maskPtr == nullptr || maskPtr[x] > 0) && rowPtr[x] > 0) {
                const double val = static_cast<double>(rowPtr[x]);
                validPixels.push_back(rowPtr[x]);
                sum += val;
                sumSq += val * val;
            }
        }
    }

    cv::Mat rawMask;
    if (validPixels.size() >= 100) {
        const double n = static_cast<double>(validPixels.size());
        const double mean = sum / n;
        const double variance = std::max(0.0, (sumSq / n) - (mean * mean));
        const double stdDev = std::sqrt(variance);

        cv::Mat sampleMat(static_cast<int>(validPixels.size()), 1, CV_8UC1, validPixels.data());
        cv::Mat dummy;
        const double otsuThresh = cv::threshold(sampleMat, dummy, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        // Garante que lesões pequenas (ex.: 6068.jpg) não façam o Otsu bipartir a própria pele saudável
        const double minContrastThresh = mean + 0.85 * stdDev;
        const double effectiveThresh = std::max(otsuThresh, minContrastThresh);

        cv::threshold(blurred, rawMask, effectiveThresh, 255, cv::THRESH_BINARY);
        if (! fovMask.empty()) {
            rawMask.setTo(cv::Scalar(0), fovMask == 0);
        }
    } else {
        cv::threshold(blurred, rawMask, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    }

    return rawMask;
}

cv::Mat ImageProcessor::segmentGrabCut(const cv::Mat& bgrImage, const cv::Mat& seedMask) {
    if (bgrImage.empty() || bgrImage.channels() != 3 || seedMask.empty()) {
        return seedMask.clone();
    }

    const int nonZero = cv::countNonZero(seedMask);
    const int totalPixels = seedMask.rows * seedMask.cols;
    if (nonZero < 50 || nonZero > totalPixels - 100) {
        return seedMask.clone();
    }

    cv::Mat gcMask(seedMask.size(), CV_8UC1, cv::Scalar(cv::GC_PR_BGD));

    // Margem externa como fundo garantido (GC_BGD)
    const int margin = std::max(4, std::min(bgrImage.cols, bgrImage.rows) / 16);
    gcMask(cv::Rect(0, 0, bgrImage.cols, margin)).setTo(cv::GC_BGD);
    gcMask(cv::Rect(0, bgrImage.rows - margin, bgrImage.cols, margin)).setTo(cv::GC_BGD);
    gcMask(cv::Rect(0, 0, margin, bgrImage.rows)).setTo(cv::GC_BGD);
    gcMask(cv::Rect(bgrImage.cols - margin, 0, margin, bgrImage.rows)).setTo(cv::GC_BGD);

    // Semente de provável frente a partir do Otsu e núcleo erodido como frente garantida (GC_FGD)
    gcMask.setTo(cv::GC_PR_FGD, seedMask > 0);
    cv::Mat sureFg;
    cv::Mat erodeKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::erode(seedMask, sureFg, erodeKernel);
    if (cv::countNonZero(sureFg) >= 20) {
        gcMask.setTo(cv::GC_FGD, sureFg > 0);
    }

    cv::Mat bgdModel, fgdModel;
    cv::grabCut(bgrImage, gcMask, cv::Rect(), bgdModel, fgdModel, 3, cv::GC_INIT_WITH_MASK);

    cv::Mat resultMask = (gcMask == cv::GC_FGD) | (gcMask == cv::GC_PR_FGD);
    return resultMask;
}

cv::Mat ImageProcessor::keepLargestConnectedComponent(const cv::Mat& binaryMask) {
    if (binaryMask.empty()) {
        return cv::Mat();
    }

    cv::Mat labels, stats, centroids;
    const int nLabels = cv::connectedComponentsWithStats(binaryMask, labels, stats, centroids);
    if (nLabels <= 1) {
        return binaryMask.clone();
    }

    const double centerX = binaryMask.cols * 0.5;
    const double centerY = binaryMask.rows * 0.5;
    const double sigma = std::min(binaryMask.cols, binaryMask.rows) * 0.28;
    const double twoSigmaSq = 2.0 * sigma * sigma;

    int bestLabel = -1;
    double bestScore = -1.0;

    for (int i = 1; i < nLabels; ++i) {
        const int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area < 25) {
            continue;
        }

        const double cx = centroids.at<double>(i, 0);
        const double cy = centroids.at<double>(i, 1);
        const double distSq = (cx - centerX) * (cx - centerX) + (cy - centerY) * (cy - centerY);

        // Ponderação gaussiana central: prioriza a lesão central sobre artefatos periféricos
        const double centralWeight = std::exp(-distSq / twoSigmaSq);
        const double score = static_cast<double>(area) * centralWeight;

        if (score > bestScore) {
            bestScore = score;
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
            const double a = cv::contourArea(contours[i]);
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

    // 1. Abertura morfológica preliminar para quebrar pontes finas com ruídos periféricos
    if (options.morphOpenKernelSize > 1) {
        int osize = options.morphOpenKernelSize;
        if (osize % 2 == 0) osize += 1;
        cv::Mat openKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(osize, osize));
        cv::morphologyEx(morph, morph, cv::MORPH_OPEN, openKernel);
    }

    // 2. Fechamento morfológico para conectar descontinuidades internas da lesão
    if (options.morphCloseKernelSize > 1) {
        int csize = options.morphCloseKernelSize;
        if (csize % 2 == 0) csize += 1;
        cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(csize, csize));
        cv::morphologyEx(morph, morph, cv::MORPH_CLOSE, closeKernel);
    }

    // 3. Reter o componente conexo central principal (elimina artefatos periféricos e marcas de tinta)
    if (options.keepLargestComponentOnly) {
        morph = keepLargestConnectedComponent(morph);
    }

    // 4. Preencher orifícios internos na máscara da lesão
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
        const double a = cv::contourArea(contours[i]);
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

    const double perimeter = cv::arcLength(mainContour, true);

    feat.area = area;
    feat.perimeter = perimeter;
    feat.compactness = (area > 0.0) ? (perimeter * perimeter) / (4.0 * kPi * area) : 0.0;
    feat.circularity = (perimeter > 0.0) ? (4.0 * kPi * area) / (perimeter * perimeter) : 0.0;
    feat.equivalentDiameter = (area > 0.0) ? std::sqrt(4.0 * area / kPi) : 0.0;
    feat.boundingBox = cv::boundingRect(mainContour);
    feat.aspectRatio = (feat.boundingBox.height > 0)
                           ? static_cast<double>(feat.boundingBox.width) / feat.boundingBox.height
                           : 0.0;

    // Ajuste de elipse (D - Diâmetro e Eixos Maior/Menor)
    if (mainContour.size() >= 5) {
        const cv::RotatedRect ellipse = cv::fitEllipse(mainContour);
        feat.majorAxisPx = std::max(ellipse.size.width, ellipse.size.height);
        feat.minorAxisPx = std::min(ellipse.size.width, ellipse.size.height);
    } else {
        feat.majorAxisPx = std::max(feat.boundingBox.width, feat.boundingBox.height);
        feat.minorAxisPx = std::min(feat.boundingBox.width, feat.boundingBox.height);
    }

    // Fecho convexo e Solidez
    std::vector<cv::Point> hull;
    cv::convexHull(mainContour, hull);
    const double hullArea = cv::contourArea(hull);
    feat.solidity = (hullArea > 0.0) ? (area / hullArea) : 0.0;

    // Momentos espaciais, Centroide e 7 Momentos Invariantes de Hu
    const cv::Moments m = cv::moments(binaryMask, true);
    if (m.m00 > 0.0) {
        feat.centroid = cv::Point2d(m.m10 / m.m00, m.m01 / m.m00);

        double rawHu[7] = {0.0};
        cv::HuMoments(m, rawHu);
        for (std::size_t i = 0; i < 7; ++i) {
            if (std::abs(rawHu[i]) > 1e-30) {
                feat.huMoments[i] = -std::copysign(1.0, rawHu[i]) * std::log10(std::abs(rawHu[i]));
            } else {
                feat.huMoments[i] = 0.0;
            }
        }
    }

    // Variância radial normalizada das distâncias do centroide aos vértices do contorno (B - Borda)
    if (! mainContour.empty()) {
        std::vector<double> radii;
        radii.reserve(mainContour.size());
        double sumR = 0.0;
        for (const auto& pt : mainContour) {
            const double dx = pt.x - feat.centroid.x;
            const double dy = pt.y - feat.centroid.y;
            const double r = std::sqrt(dx * dx + dy * dy);
            radii.push_back(r);
            sumR += r;
        }
        const double meanR = sumR / static_cast<double>(radii.size());
        if (meanR > 1e-6) {
            double varSum = 0.0;
            for (const double r : radii) {
                const double diff = r - meanR;
                varSum += diff * diff;
            }
            const double variance = varSum / static_cast<double>(radii.size());
            feat.borderRadialVariance = variance / (meanR * meanR);
        }
    }

    // A - Assimetria Biaxial: translada o centroide para o centro exato do canvas e alinha pelos eixos de inércia
    if (area > 20.0 && m.m00 > 0.0) {
        const double angleRad = 0.5 * std::atan2(2.0 * m.mu11, m.mu20 - m.mu02);
        cv::Mat rotMat = cv::getRotationMatrix2D(feat.centroid, angleRad * 180.0 / kPi, 1.0);
        const double targetCenterX = (binaryMask.cols - 1) * 0.5;
        const double targetCenterY = (binaryMask.rows - 1) * 0.5;
        rotMat.at<double>(0, 2) += (targetCenterX - feat.centroid.x);
        rotMat.at<double>(1, 2) += (targetCenterY - feat.centroid.y);

        cv::Mat alignedMask;
        cv::warpAffine(binaryMask, alignedMask, rotMat, binaryMask.size(), cv::INTER_NEAREST);

        const double totalCount = static_cast<double>(cv::countNonZero(alignedMask));
        if (totalCount > 0.0) {
            cv::Mat flippedMajor, flippedMinor, diffMajor, diffMinor;
            // Reflexão sobre o eixo maior (horizontal após alinhamento -> flip vertical)
            cv::flip(alignedMask, flippedMajor, 0);
            // Reflexão sobre o eixo menor (vertical após alinhamento -> flip horizontal)
            cv::flip(alignedMask, flippedMinor, 1);

            cv::bitwise_xor(alignedMask, flippedMajor, diffMajor);
            cv::bitwise_xor(alignedMask, flippedMinor, diffMinor);

            // Normaliza por (2 * totalCount) ou razão de área não sobreposta em [0, 1]
            feat.asymmetryMajor = std::min(1.0, static_cast<double>(cv::countNonZero(diffMajor)) / (2.0 * totalCount));
            feat.asymmetryMinor = std::min(1.0, static_cast<double>(cv::countNonZero(diffMinor)) / (2.0 * totalCount));
            feat.asymmetryIndex = 0.5 * (feat.asymmetryMajor + feat.asymmetryMinor);
        }
    }

    feat.valid = (feat.area > 0.0);
    return feat;
}

ColorFeatures ImageProcessor::extractColorFeatures(const cv::Mat& bgrImage, const cv::Mat& binaryMask) {
    ColorFeatures cf;
    if (bgrImage.empty() || bgrImage.channels() != 3 || binaryMask.empty() || cv::countNonZero(binaryMask) == 0) {
        return cf;
    }

    // Estatísticas BGR restritas à lesão
    cv::Scalar meanBgr, stdBgr;
    cv::meanStdDev(bgrImage, meanBgr, stdBgr, binaryMask);
    cf.lesionMeanB = meanBgr[0];
    cf.lesionMeanG = meanBgr[1];
    cf.lesionMeanR = meanBgr[2];
    cf.lesionStdB = stdBgr[0];
    cf.lesionStdG = stdBgr[1];
    cf.lesionStdR = stdBgr[2];

    // Estatísticas CIE L*a*b* restritas à lesão
    cv::Mat lab;
    cv::cvtColor(bgrImage, lab, cv::COLOR_BGR2Lab);
    cv::Scalar meanLab, stdLab;
    cv::meanStdDev(lab, meanLab, stdLab, binaryMask);
    cf.lesionMeanL = meanLab[0];
    cf.lesionMeanA = meanLab[1];
    cf.lesionMeanBStar = meanLab[2];
    cf.lesionStdL = stdLab[0];
    cf.lesionStdA = stdLab[1];
    cf.lesionStdBStar = stdLab[2];

    // Estatísticas HSV restritas à lesão
    cv::Mat hsv;
    cv::cvtColor(bgrImage, hsv, cv::COLOR_BGR2HSV);
    cv::Scalar meanHsv, stdHsv;
    cv::meanStdDev(hsv, meanHsv, stdHsv, binaryMask);
    cf.lesionMeanH = meanHsv[0];
    cf.lesionMeanS = meanHsv[1];
    cf.lesionMeanV = meanHsv[2];
    cf.lesionStdH = stdHsv[0];
    cf.lesionStdS = stdHsv[1];
    cf.lesionStdV = stdHsv[2];

    // Quantificação das 6 cores dermatoscópicas da regra ABCD (Stolz):
    // 0: Branco, 1: Vermelho, 2: Castanho-claro, 3: Castanho-escuro, 4: Azul-acinzentado, 5: Preto
    std::array<int, 6> colorCounts = {0, 0, 0, 0, 0, 0};
    int lesionPixels = 0;

    for (int y = 0; y < bgrImage.rows; ++y) {
        const uchar* maskPtr = binaryMask.ptr<uchar>(y);
        const cv::Vec3b* bgrPtr = bgrImage.ptr<cv::Vec3b>(y);
        const cv::Vec3b* hsvPtr = hsv.ptr<cv::Vec3b>(y);
        for (int x = 0; x < bgrImage.cols; ++x) {
            if (maskPtr[x] == 0) {
                continue;
            }
            ++lesionPixels;
            const int b = bgrPtr[x][0];
            const int g = bgrPtr[x][1];
            const int r = bgrPtr[x][2];
            const int h = hsvPtr[x][0]; // [0..180]
            const int s = hsvPtr[x][1]; // [0..255]
            const int v = hsvPtr[x][2]; // [0..255]

            if (v < 60) {
                ++colorCounts[5]; // Preto
            } else if (v > 205 && s < 45) {
                ++colorCounts[0]; // Branco
            } else if (b >= g && (r - b) < 25 && v < 175) {
                ++colorCounts[4]; // Azul-acinzentado
            } else if ((h < 10 || h > 165) && s > 100 && r > 150) {
                ++colorCounts[1]; // Vermelho
            } else if (v < 135) {
                ++colorCounts[3]; // Castanho-escuro
            } else {
                ++colorCounts[2]; // Castanho-claro
            }
        }
    }

    if (lesionPixels > 0) {
        const int minClusterPixels = std::max(10, static_cast<int>(lesionPixels * 0.03));
        int activeClusters = 0;
        for (const int c : colorCounts) {
            if (c >= minClusterPixels) {
                ++activeClusters;
            }
        }
        cf.colorClusters = std::max(1, activeClusters);
    }

    return cf;
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

    // 1. Remoção de pelos (Black-Hat anisotrópico + Filtragem geométrica + Inpainting de Telea)
    auto [hairMask, inpainted] = removeHairs(bgrImage, hairOptions);
    res.artifacts.hairMask = hairMask;
    res.artifacts.inpainted = inpainted;

    // 2. Suavização bilateral preservadora de bordas para o ramo de segmentação
    const cv::Mat segInput = hairOptions.applyBilateral
                                 ? applyBilateralFilter(inpainted)
                                 : inpainted;

    // 3. Máscara de Campo de Visão (FoV) e preparação do canal de segmentação
    const cv::Mat fovMask = segOptions.suppressVignette
                                ? createFieldOfViewMask(segInput)
                                : cv::Mat();
    cv::Mat channelImg = prepareSegmentationChannel(segInput, segOptions.channel, segOptions.suppressVignette);
    res.artifacts.segmentationChannel = channelImg;

    // 3. Segmentação (Otsu ou GrabCut)
    cv::Mat rawSeg = segmentOtsu(channelImg, fovMask);
    if (segOptions.method == SegmentationMethod::GrabCut) {
        rawSeg = segmentGrabCut(inpainted, rawSeg);
    }
    res.artifacts.rawOtsuMask = rawSeg;

    // 4. Pós-processamento morfológico e seleção da lesão central
    cv::Mat finalMask = applyMorphologyAndFilter(rawSeg, segOptions);
    res.artifacts.finalMask = finalMask;

    // 5. Extração de características ABCD (Morfologia + Cor na máscara da lesão)
    res.features = extractFeatures(finalMask);
    res.colorFeatures = extractColorFeatures(inpainted, finalMask);

    // Sincroniza estatísticas de cor da lesão no basicSummary quando a máscara é válida
    if (res.features.valid) {
        res.basicSummary.meanB = res.colorFeatures.lesionMeanB;
        res.basicSummary.meanG = res.colorFeatures.lesionMeanG;
        res.basicSummary.meanR = res.colorFeatures.lesionMeanR;
    }

    // 6. Overlay com contorno
    res.artifacts.contourOverlay = createContourOverlay(inpainted, finalMask, cv::Scalar(0, 255, 0), 2);

    res.success = res.features.valid;
    if (! res.success) {
        res.errorMessage = "Nenhum contorno ou regiao valida foi detectada na segmentacao";
    }

    return res;
}

} // namespace melanoma::core

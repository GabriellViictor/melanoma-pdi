#include "io/DatasetManager.hpp"

#include <algorithm>
#include <cctype>
#include <opencv2/imgcodecs.hpp>

namespace melanoma::io {

namespace {

bool isJpegFile(const std::filesystem::path& path) {
    if (! path.has_extension()) {
        return false;
    }
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return ext == ".jpg" || ext == ".jpeg";
}

std::size_t countSubdirImages(const std::filesystem::path& dirPath, bool& dirExists) {
    dirExists = std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath);
    if (! dirExists) {
        return 0;
    }

    std::size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file() && isJpegFile(entry.path())) {
            ++count;
        }
    }
    return count;
}

void collectFromSubdir(
    const std::filesystem::path& dirPath,
    DatasetSplit split,
    LesionClass label,
    std::size_t maxPerClass,
    std::vector<ImageEntry>& outEntries
) {
    if (! std::filesystem::exists(dirPath) || ! std::filesystem::is_directory(dirPath)) {
        return;
    }

    std::vector<std::filesystem::path> filePaths;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file() && isJpegFile(entry.path())) {
            filePaths.push_back(entry.path());
        }
    }

    std::sort(filePaths.begin(), filePaths.end());

    std::size_t count = 0;
    for (const auto& path : filePaths) {
        if (maxPerClass > 0 && count >= maxPerClass) {
            break;
        }
        outEntries.push_back(ImageEntry{
            .filePath = path,
            .id = path.stem().string(),
            .split = split,
            .label = label
        });
        ++count;
    }
}

} // namespace

std::filesystem::path DatasetManager::resolveDatasetRoot(
    const std::filesystem::path& requestedPath,
    const std::filesystem::path& executablePath
) {
    if (requestedPath.is_absolute() ||
        (std::filesystem::exists(requestedPath / "train") && std::filesystem::exists(requestedPath / "test"))) {
        return requestedPath;
    }

    auto tryFindFromBase = [&](std::filesystem::path base) -> std::optional<std::filesystem::path> {
        std::error_code ec;
        base = std::filesystem::weakly_canonical(base, ec);
        for (int depth = 0; depth < 5 && ! base.empty(); ++depth) {
            const auto candidate = base / requestedPath;
            if (std::filesystem::exists(candidate / "train", ec) ||
                std::filesystem::exists(base / "CMakeLists.txt", ec)) {
                if (std::filesystem::exists(candidate, ec)) {
                    return candidate;
                }
            }
            const auto parent = base.parent_path();
            if (parent == base) {
                break;
            }
            base = parent;
        }
        return std::nullopt;
    };

    if (auto found = tryFindFromBase(std::filesystem::current_path())) {
        return *found;
    }

    if (! executablePath.empty()) {
        if (auto found = tryFindFromBase(executablePath.parent_path())) {
            return *found;
        }
    }

    return requestedPath;
}

DatasetReport DatasetManager::inspectDataset(const std::filesystem::path& datasetRoot) {
    DatasetReport report;
    report.datasetRoot = datasetRoot;
    report.rootExists = std::filesystem::exists(datasetRoot) && std::filesystem::is_directory(datasetRoot);

    if (! report.rootExists) {
        return report;
    }

    report.trainBenignCount = countSubdirImages(datasetRoot / "train" / "Benign", report.trainBenignExists);
    report.trainMalignantCount = countSubdirImages(datasetRoot / "train" / "Malignant", report.trainMalignantExists);
    report.testBenignCount = countSubdirImages(datasetRoot / "test" / "Benign", report.testBenignExists);
    report.testMalignantCount = countSubdirImages(datasetRoot / "test" / "Malignant", report.testMalignantExists);

    return report;
}

std::vector<ImageEntry> DatasetManager::listImages(
    const std::filesystem::path& datasetRoot,
    std::optional<DatasetSplit> splitFilter,
    std::size_t maxPerClass
) {
    std::vector<ImageEntry> entries;

    if (! splitFilter.has_value() || splitFilter.value() == DatasetSplit::Train) {
        collectFromSubdir(datasetRoot / "train" / "Benign", DatasetSplit::Train, LesionClass::Benign, maxPerClass, entries);
        collectFromSubdir(datasetRoot / "train" / "Malignant", DatasetSplit::Train, LesionClass::Malignant, maxPerClass, entries);
    }

    if (! splitFilter.has_value() || splitFilter.value() == DatasetSplit::Test) {
        collectFromSubdir(datasetRoot / "test" / "Benign", DatasetSplit::Test, LesionClass::Benign, maxPerClass, entries);
        collectFromSubdir(datasetRoot / "test" / "Malignant", DatasetSplit::Test, LesionClass::Malignant, maxPerClass, entries);
    }

    return entries;
}

cv::Mat DatasetManager::loadImage(const ImageEntry& entry) {
    return cv::imread(entry.filePath.string(), cv::IMREAD_COLOR);
}

std::string DatasetManager::toString(DatasetSplit split) {
    return (split == DatasetSplit::Train) ? "Train" : "Test";
}

std::string DatasetManager::toString(LesionClass label) {
    return (label == LesionClass::Benign) ? "Benign" : "Malignant";
}

} // namespace melanoma::io

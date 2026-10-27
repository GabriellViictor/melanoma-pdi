#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <opencv2/core.hpp>

namespace melanoma::io {

enum class DatasetSplit {
    Train,
    Test
};

enum class LesionClass {
    Benign,
    Malignant
};

struct ImageEntry {
    std::filesystem::path filePath;
    std::string id;
    DatasetSplit split;
    LesionClass label;
};

struct DatasetReport {
    std::filesystem::path datasetRoot;
    bool rootExists = false;
    bool trainBenignExists = false;
    bool trainMalignantExists = false;
    bool testBenignExists = false;
    bool testMalignantExists = false;

    std::size_t trainBenignCount = 0;
    std::size_t trainMalignantCount = 0;
    std::size_t testBenignCount = 0;
    std::size_t testMalignantCount = 0;

    std::size_t totalTrainCount() const noexcept {
        return trainBenignCount + trainMalignantCount;
    }

    std::size_t totalTestCount() const noexcept {
        return testBenignCount + testMalignantCount;
    }

    std::size_t totalCount() const noexcept {
        return totalTrainCount() + totalTestCount();
    }

    bool hasAllDirectories() const noexcept {
        return rootExists && trainBenignExists && trainMalignantExists &&
               testBenignExists && testMalignantExists;
    }

    bool isValid() const noexcept {
        return hasAllDirectories() &&
               trainBenignCount > 0 && trainMalignantCount > 0 &&
               testBenignCount > 0 && testMalignantCount > 0;
    }
};

class DatasetManager {
public:
    static constexpr std::size_t kExpectedTrainBenign = 6289;
    static constexpr std::size_t kExpectedTrainMalignant = 5590;
    static constexpr std::size_t kExpectedTestBenign = 1000;
    static constexpr std::size_t kExpectedTestMalignant = 1000;
    static constexpr std::size_t kExpectedTotal = 13879;

    static std::filesystem::path resolveDatasetRoot(
        const std::filesystem::path& requestedPath,
        const std::filesystem::path& executablePath = {}
    );

    static DatasetReport inspectDataset(const std::filesystem::path& datasetRoot);

    static std::vector<ImageEntry> listImages(
        const std::filesystem::path& datasetRoot,
        std::optional<DatasetSplit> splitFilter = std::nullopt,
        std::size_t maxPerClass = 0
    );

    static cv::Mat loadImage(const ImageEntry& entry);

    static std::string toString(DatasetSplit split);
    static std::string toString(LesionClass label);
};

} // namespace melanoma::io

#include "io/DatasetManager.hpp"

namespace melanoma::io {

DatasetReport DatasetManager::inspectDataset(const std::filesystem::path& datasetRoot) {
    DatasetReport report;
    report.datasetRoot = datasetRoot;
    report.rootExists = std::filesystem::exists(datasetRoot) && std::filesystem::is_directory(datasetRoot);
    return report;
}

std::vector<ImageEntry> DatasetManager::listImages(
    const std::filesystem::path& /*datasetRoot*/,
    std::optional<DatasetSplit> /*splitFilter*/,
    std::size_t /*maxPerClass*/
) {
    return {};
}

cv::Mat DatasetManager::loadImage(const ImageEntry& /*entry*/) {
    return {};
}

std::string DatasetManager::toString(DatasetSplit split) {
    return (split == DatasetSplit::Train) ? "Train" : "Test";
}

std::string DatasetManager::toString(LesionClass label) {
    return (label == LesionClass::Benign) ? "Benign" : "Malignant";
}

} // namespace melanoma::io

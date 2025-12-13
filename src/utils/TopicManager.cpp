#include "TopicManager.h"
#include "Embedding.h"
#include "PhrasesCollectorUtils.h"

#include <cctype>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

using WordEmbeddingPtr = std::shared_ptr<class WordEmbedding>;

namespace {
bool containsNoLatin(const std::string& str) {
    for (char ch : str) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            return false;
        }
    }
    return true;
}

} // namespace

// Инициализация при первом вызове
const std::unordered_set<std::string>& TopicManager::getTopics() {
    auto& manager = getInstance();

    // Ленивая инициализация
    if (manager.topics.empty()) {
        manager.loadTopics();
    }

    return manager.topics;
}

const std::unordered_map<std::string, WordEmbeddingPtr>& TopicManager::getTopicVectors() {
    auto& manager = getInstance();

    // Ленивая инициализация
    if (manager.topicVectors.empty()) {
        auto& topics = getTopics(); // Сначала загружаем темы

        // C++20 ranges для создания embeddings
        for (const auto& topic : topics) {
            manager.topicVectors[topic] = std::make_shared<WordEmbedding>(topic);
        }
    }

    return manager.topicVectors;
}

// Вспомогательные статические методы
std::string TopicManager::trimTrailingDigitsAndSpaces(std::string line) {
    auto notDigit = [](unsigned char ch) { return !std::isdigit(ch); };
    line.erase(std::find_if(line.rbegin(), line.rend(), notDigit).base(), line.end());

    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    line.erase(std::find_if(line.rbegin(), line.rend(), notSpace).base(), line.end());

    return line;
}

bool TopicManager::isValidTopic(const std::string& line) {
    return line.size() > 3;
}

void TopicManager::loadTopics() {
    auto& options = Options::getOptions();
    std::filesystem::path inputPath = options.tagsAndHubsFile;

    std::ifstream file(inputPath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + inputPath.string());
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!containsNoLatin(line)) {
            continue;
        }

        line = trimTrailingDigitsAndSpaces(line);

        const auto lowerLine = GetLowerCase(line);
        if (isValidTopic(lowerLine)) {
            topics.insert(lowerLine);
        }
    }
}

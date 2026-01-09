#include "TopicManager.h"
#include "Embedding.h"
#include "Options.h"
#include "StringUtils.h"

#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>

#include <cctype>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

using WordEmbeddingPtr = std::shared_ptr<class WordEmbedding>;

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

        for (const auto& topic : topics) {
            manager.topicVectors[topic] = std::make_shared<WordEmbedding>(topic);
        }
    }

    return manager.topicVectors;
}

bool TopicManager::isValidTopic(const std::string& line) {
    return line.size() > 3;
}

// todo add keyWords
void TopicManager::loadTopics() {
    auto& options = Options::getOptions();
    std::filesystem::path inputPath{};

    std::ifstream file(inputPath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + inputPath.string());
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!StringUtils::containsNoLatin(line)) {
            continue;
        }

        auto trimmedLine = std::string(StringUtils::trim(line));
        auto lowerLine = StringUtils::toLowerCase(trimmedLine);
        if (isValidTopic(lowerLine)) {
            topics.insert(lowerLine);
        }
    }
}

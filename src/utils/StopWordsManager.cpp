#include "StopWordsManager.h"
#include "Options.h"
#include "StringUtils.h"

#include <cctype>
#include <fstream>

// Получить все stop words
const std::unordered_set<std::string>& StopWordsManager::getStopWords() {
    auto& manager = getInstance();

    // Ленивая инициализация
    if (manager.stopWords.empty()) {
        manager.loadStopWords();
    }

    return manager.stopWords;
}

// Проверить, является ли слово stop word
bool StopWordsManager::isStopWord(const std::string& word) {
    const auto& stopWords = getStopWords();
    const auto lowerWord = StringUtils::toLowerCase(word);
    return stopWords.contains(lowerWord);
}

// Загрузка stop words из файла
void StopWordsManager::loadStopWords() {
    auto& options = Options::getOptions();
    std::filesystem::path inputPath = options.stopWordsFile;

    std::ifstream file(inputPath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + inputPath.string());
    }

    std::string line;
    while (std::getline(file, line)) {
        const auto lowerLine = StringUtils::toLowerCase(line);
        stopWords.insert(lowerLine);
    }
}

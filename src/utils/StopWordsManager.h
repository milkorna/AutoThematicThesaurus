#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>

class StopWordsManager {
  public:
    // Удаляем копирование
    StopWordsManager(const StopWordsManager&) = delete;
    StopWordsManager& operator=(const StopWordsManager&) = delete;

    // Статический метод для получения единственного экземпляра
    static StopWordsManager& getInstance() {
        static StopWordsManager instance;
        return instance;
    }

    // Получить набор stop words
    static const std::unordered_set<std::string>& getStopWords();

    // Проверить, является ли слово stop word
    static bool isStopWord(const std::string& word);

  private:
    // Приватный конструктор для Singleton паттерна
    StopWordsManager() = default;

    // Загрузка stop words из файла
    void loadStopWords();

  private:
    std::unordered_set<std::string> stopWords;
};

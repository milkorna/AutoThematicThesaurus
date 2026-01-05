#pragma once

#include "TextCorpus.h"

#include <nlohmann/json.hpp>

class TextCorpusLoader {
  public:
    /**
     * @brief Сохраняет корпус в JSON файл
     * @param corpus Корпус для сохранения
     * @param filename Путь к выходному файлу
     */
    static void save(const TextCorpus& corpus, const std::string& filename);

    /**
     * @brief Загружает корпус из JSON файла БЕЗ фильтрации
     * @param corpus Ссылка на корпус для заполнения
     * @param filename Путь к JSON файлу
     */
    static void load(TextCorpus& corpus, const std::string& filename);

  private:
    /**
     * @brief Преобразует корпус в JSON объект
     */
    static nlohmann::json serialize(const TextCorpus& corpus);

    /**
     * @brief Загружает корпус из JSON объекта БЕЗ фильтрации
     */
    static void deserialize(TextCorpus& corpus, const nlohmann::json& j);

    static void readWordFrequencies(TextCorpus& corpus, const nlohmann::json& j);
    static void readDocumentFrequencies(TextCorpus& corpus, const nlohmann::json& j);
    static void readTexts(TextCorpus& corpus, const nlohmann::json& j);
};
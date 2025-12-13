#pragma once

#include "TextCorpus.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

class TextCorpusDeserializer {
  public:
    /**
     * @brief Загружает корпус из JSON файла БЕЗ фильтрации
     * @param corpus Ссылка на корпус для заполнения
     * @param filename Путь к JSON файлу
     */
    static void deserialize(TextCorpus& corpus, const std::string& filename);

    /**
     * @brief Загружает корпус из JSON объекта БЕЗ фильтрации
     */
    static void loadFromJson(TextCorpus& corpus, const json& j);

  private:
    static void loadWordFrequencies(TextCorpus& corpus, const json& j);
    static void loadDocumentFrequencies(TextCorpus& corpus, const json& j);
    static void loadTexts(TextCorpus& corpus, const json& j);
};
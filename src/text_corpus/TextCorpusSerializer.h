#pragma once

#include "TextCorpus.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class TextCorpusSerializer {
  public:
    /**
     * @brief Сохраняет корпус в JSON файл
     * @param corpus Корпус для сохранения
     * @param filename Путь к выходному файлу
     */
    static void serialize(const TextCorpus& corpus, const std::string& filename);

    /**
     * @brief Преобразует корпус в JSON объект
     */
    static json corpusToJson(const TextCorpus& corpus);
};
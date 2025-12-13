#pragma once

#include "TextCorpus.h"
#include "Logger.h"
#include "StringFilters.h"

class TextCorpusFilter {
  public:
    /**
     * @brief Фильтрует тексты по длине и наличию пробелов
     * @param corpus Ссылка на корпус для фильтрации
     * @param minLength Минимальная длина текста (по умолчанию 40)
     * @param requireSpaces Требовать ли пробелы в тексте
     */
    static void filterTextsByLength(TextCorpus& corpus,
                                    size_t minLength = 40,
                                    bool requireSpaces = true);

    /**
     * @brief Фильтрует стоп-слова из словаря корпуса
     * @param corpus Ссылка на корпус
     */
    static void filterStopWords(TextCorpus& corpus);

  private:
    /**
     * @brief Пересчитывает TF/IDF статистику после фильтрации
     * @param corpus Корпус для пересчёта
     */
    static void recalculateStatistics(TextCorpus& corpus);
};
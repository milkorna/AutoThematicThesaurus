#pragma once

#include "Logger.h"
#include "PatternPhrasesStorage.h"
#include <string>
#include <unordered_map>

/**
 * @brief Класс для слияния похожих кластеров фраз
 *
 * Следует паттерну TextCorpusFilter для операций с PatternPhrasesStorage.
 * Используется для объединения морфологических вариантов одной фразы
 * (например "смежный вектора" и "смежный вектор").
 */
class ClusterMerger {
  public:
    /**
     * @brief Выполняет слияние похожих кластеров в хранилище
     *
     * Объединяет кластеры, которые имеют морфологические вариации,
     * рассчитанные на основе сходства ключей.
     *
     * @param storage Ссылка на хранилище фраз для обработки
     * @param maxDiff Максимальное количество различающихся символов (по умолчанию 3)
     * @param endLength Длина суффикса для сравнения (по умолчанию 2)
     */
    static void mergeClusters(PatternPhrasesStorage& storage, size_t maxDiff = 2, size_t endLength = 4);

  private:
    /**
     * @brief Проверяет, похожи ли два ключа кластеров
     * @param key1 Первый ключ
     * @param key2 Второй ключ
     * @param maxDiff Максимальное количество различающихся символов
     * @param endLength Длина суффикса для сравнения
     * @param checkFirstOnly Проверять ли только первое слово
     * @return true, если ключи похожи
     */
    static bool AreKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff = 2,
                               size_t endLength = 4, bool checkFirstOnly = false);

    /**
     * @brief Вспомогательный метод для логирования статистики слияния
     * @param originalCount Количество кластеров до слияния
     * @param mergedCount Количество кластеров после слияния
     */
    static void logMergeStatistics(size_t originalCount, size_t mergedCount);
};

#pragma once

#include "Logger.h"
#include "StringUtils.h"
#include "TextCorpus.h"

/**
 * @brief Filters corpus texts and vocabularies based on quality metrics
 * @details Applies length-based filtering, removes stop words, and recalculates corpus statistics.
 * Modifies corpus in-place using reference parameters.
 */
class TextCorpusFilter {
  public:
    /**
     * @brief Filters texts by minimum length and space requirements
     * @details Removes texts shorter than threshold and optionally those without whitespace.
     * Updates corpus statistics after filtering.
     *
     * @param corpus Reference to the text corpus for in-place filtering
     * @param minLength Minimum text length in characters (default: 40)
     * @param requireSpaces If true, removes texts without spaces (default: true)
     */
    static void filterTextsByLength(TextCorpus& corpus, size_t minLength = 40, bool requireSpaces = true);

    /**
     * @brief Filters stop words from corpus vocabulary
     * @details Removes stop words from word and document frequency maps.
     * Uses StringUtils for stop word detection. Recalculates statistics after removal.
     *
     * @param corpus Reference to the text corpus for in-place filtering
     */
    static void filterStopWords(TextCorpus& corpus);

  private:
    /**
     * @brief Recalculates TF/IDF statistics after filtering operations
     * @details Invokes corpus statistics recalculation to maintain consistency
     * after vocabulary or text modifications.
     *
     * @param corpus Reference to the corpus to recalculate
     */
    static void recalculateStatistics(TextCorpus& corpus);
};
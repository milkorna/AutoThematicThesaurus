
#pragma once

#include "GrammarPatternManager.h"
#include "PhrasesCollectorUtils.h"
#include "xmorphy/morph/WordForm.h"

class MorphAnalyzer {
  public:
    static MorphAnalyzer& getInstance() {
        static MorphAnalyzer instance;
        return instance;
    }

    // // Проверка ошибки морфологического анализа
    // bool isMorphAnalysisError(const X::WordFormPtr& token) const;

    // // Проверка наличия нужной части речи
    // bool hasDesiredPOS(const std::unordered_set& morphInfo) const;

    const std::string getLemma(const X::WordFormPtr& form) const;

    const std::string getNormalForm(const X::WordFormPtr& form) const;

  private:
    MorphAnalyzer() = default;
    ~MorphAnalyzer() = default;

    MorphAnalyzer(const MorphAnalyzer&) = delete;
    MorphAnalyzer& operator=(const MorphAnalyzer&) = delete;

    /**
     * \brief Retrieves the most probable morphological information from a set.
     * \param morphSet A set of morphological information.
     * \return The most probable MorphInfo object.
     */
    X::MorphInfo getMostProbableMorphInfo(const std::unordered_set<X::MorphInfo>& morphSet) const;

    // const std::unordered_set desiredPOS = {"ADJ", "NOUN", "PROPN", "VERB"};
};

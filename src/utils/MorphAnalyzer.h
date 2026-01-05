#pragma once

#include "xmorphy/morph/WordForm.h"

class MorphAnalyzer {
  public:
    /**
     * @brief Gets the singleton instance of MorphAnalyzer.
     *
     * @return Reference to the static MorphAnalyzer instance.
     */
    static MorphAnalyzer& getInstance() {
        static MorphAnalyzer instance;
        return instance;
    }

    /**
     * @brief Checks for morphological analysis error.
     *
     * @param token The word form to check.
     * @return True if morphological analysis error detected, false otherwise.
     */
    bool isMorphAnalysisError(const X::WordFormPtr& token) const;

    /**
     * @brief Checks if token has desired part of speech.
     *
     * @param morphInfo Set of morphological information.
     * @return True if desired POS found, false otherwise.
     */
    // bool hasDesiredPOS(const std::unordered_set& morphInfo) const;

    /**
     * @brief Retrieves the lemma (dictionary form) of a word.
     *
     * @param form The word form to extract lemma from.
     * @return The lemma string.
     */
    const std::string getLemma(const X::WordFormPtr& form) const;

    /**
     * @brief Retrieves the normal form of a word.
     *
     * @param form The word form to extract normal form from.
     * @return The normal form string.
     */
    const std::string getNormalForm(const X::WordFormPtr& form) const;

  private:
    MorphAnalyzer() = default;
    ~MorphAnalyzer() = default;

    MorphAnalyzer(const MorphAnalyzer&) = delete;
    MorphAnalyzer& operator=(const MorphAnalyzer&) = delete;

    /**
     * @brief Retrieves the most probable morphological information from a set.
     *
     * @param morphSet A set of morphological information.
     * @return The most probable MorphInfo object.
     */
    X::MorphInfo getMostProbableMorphInfo(const std::unordered_set<X::MorphInfo>& morphSet) const;

    /**
     * @brief Set of desired parts of speech for filtering.
     */
    // const std::unordered_set desiredPOS = {"ADJ", "NOUN", "PROPN", "VERB"};
};

#pragma once

#include "Embedding.h"
#include "Phrase.h"
#include "SentenceCorpus.h"

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * @struct LemmaMetrics
 * @brief Metrics for individual lemma in a phrase cluster
 * @details Stores TF, IDF, TF-IDF for single lemma
 */
struct LemmaMetrics {
    /// @brief The lemma text
    std::string text;

    /// @brief Term Frequency for this lemma
    double tf = 0.0;

    /// @brief Inverse Document Frequency for this lemma
    double idf = 0.0;

    /// @brief TF-IDF combined metric for this lemma
    double tfidf = 0.0;
};

/**
 * @struct PhraseCluster
 * @brief Represents a cluster of word complexes with linguistic metrics and embeddings
 * @details Contains metrics per lemma, FastText vectors, semantic relations, and context information
 */
struct PhraseCluster {
    /// @brief Size of the phrase (number of words)
    size_t phraseSize = 0;

    /// @brief Indicates if the cluster matches a topic tag
    bool tagMatch = false;

    /// @brief Frequency value for the cluster (total occurrences)
    double frequency = 0.0;

    /// @brief Relevance score relative to document topics
    double topicRelevance = 0.0;

    /// @brief Centrality measure within the cluster
    double centralityScore = 0.0;

    /// @brief String with normalized words (cluster key)
    std::string key;

    /// @brief Name of the embedding model associated with the cluster
    std::string modelName;

    /// @brief Vector of lemmas with their metrics
    std::vector<LemmaMetrics> lemmas;

    /// @brief Vector of word complexes (phrases) in the cluster
    std::vector<PhrasePtr> phrases;

    /// @brief Vector of FastText word embeddings for each word
    std::vector<WordEmbeddingPtr> wordVectors;

    /// @brief Map of hypernyms for each word in the phrase
    std::unordered_map<std::string, std::set<std::string>> hypernyms;

    /// @brief Map of hyponyms for each word in the phrase
    std::unordered_map<std::string, std::set<std::string>> hyponyms;

    /// @brief Set of synonyms for the cluster
    std::unordered_set<std::string> synonyms;

    /// @brief Vector of sentences where the cluster appears
    std::vector<TokenizedSentence> contexts;

    /// @brief Flag indicating if the cluster is recognized as a term
    bool is_term = false;

    // // ========== HELPER METHODS ==========

    // /**
    //  * @brief Get vector of just lemma texts (for backward compatibility)
    //  * @return Vector of lemma strings
    //  */
    // [[nodiscard]] std::vector<std::string> getLemmaTexts() const {
    //     std::vector<std::string> texts;
    //     texts.reserve(lemmas.size());
    //     for (const auto& lm : lemmas) {
    //         texts.push_back(lm.text);
    //     }
    //     return texts;
    // }

    // /**
    //  * @brief Get vector of TF values
    //  * @return Vector of TF values in lemma order
    //  */
    // [[nodiscard]] std::vector<double> getTfValues() const {
    //     std::vector<double> values;
    //     values.reserve(lemmas.size());
    //     for (const auto& lm : lemmas) {
    //         values.push_back(lm.tf);
    //     }
    //     return values;
    // }

    // /**
    //  * @brief Get vector of IDF values
    //  * @return Vector of IDF values in lemma order
    //  */
    // [[nodiscard]] std::vector<double> getIdfValues() const {
    //     std::vector<double> values;
    //     values.reserve(lemmas.size());
    //     for (const auto& lm : lemmas) {
    //         values.push_back(lm.idf);
    //     }
    //     return values;
    // }

    // /**
    //  * @brief Get vector of TF-IDF values
    //  * @return Vector of TF-IDF values in lemma order
    //  */
    // [[nodiscard]] std::vector<double> getTfidfValues() const {
    //     std::vector<double> values;
    //     values.reserve(lemmas.size());
    //     for (const auto& lm : lemmas) {
    //         values.push_back(lm.tfidf);
    //     }
    //     return values;
    // }
};

using PhraseClusters = std::unordered_map<std::string, PhraseCluster>;
#pragma once

#include "Embedding.h"
#include "TokenizedSentenceCorpus.h"
#include "WordComplex.h"

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * @struct WordComplexCluster
 * @brief Represents a cluster of word complexes with linguistic metrics and embeddings
 * @details Contains TF, IDF, TF-IDF values, FastText vectors, semantic relations, and context information
 */
struct WordComplexCluster {
    /// @brief Size of the phrase (number of words)
    size_t phraseSize;

    /// @brief Indicates if the cluster matches a topic tag
    bool tagMatch;

    /// @brief Frequency value for the cluster
    double frequency;

    /// @brief Relevance score relative to document topics
    double topicRelevance;

    /// @brief Centrality measure within the cluster
    double centralityScore;

    /// @brief String with normalized words (cluster key)
    std::string key;

    /// @brief Name of the embedding model associated with the cluster
    std::string modelName;

    /// @brief Vector of lemmas in the cluster
    std::vector<std::string> lemmas;

    /// @brief Vector of word complexes (phrases) in the cluster
    std::vector<WordComplexPtr> wordComplexes;

    /// @brief Vector of TF (Term Frequency) values for each word
    std::vector<double> tf;

    /// @brief Vector of IDF (Inverse Document Frequency) values for each word
    std::vector<double> idf;

    /// @brief Vector of TF-IDF (combined) values for each word
    std::vector<double> tfidf;

    /// @brief Vector of FastText word embeddings for each word
    std::vector<WordEmbeddingPtr> wordVectors;

    /// @brief Map of hypernyms for each word in the phrase
    std::unordered_map<std::string, std::set<std::string>> hypernyms;

    /// @brief Map of hyponyms for each word in the phrase
    std::unordered_map<std::string, std::set<std::string>> hyponyms;

    /// @brief Set of synonyms for the cluster
    std::unordered_set<std::string> synonyms;

    /// @brief Vector of tokenized sentences where the cluster appears
    std::vector<TokenizedSentence> contexts;

    /// @brief Flag indicating if the cluster is recognized as a term
    bool is_term;
};
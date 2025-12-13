#pragma once

#include "PatternPhrasesStorage.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

/**
 * @brief Serializes WordComplexCluster objects to JSON format
 * @details Takes only the data needed, not the whole singleton
 */
class ClusterSerializer {
  public:
    /**
     * @brief Serialize a single cluster to JSON
     * @param cluster The cluster to serialize
     * @param frequency Calculated frequency for the cluster
     * @return JSON object with cluster data
     */
    json serialize(const WordComplexCluster& cluster, double frequency) const;

    /**
     * @brief Serialize collection of clusters
     * @param clusters Map of clusters to serialize
     * @param frequencies Map of cluster frequencies (key -> frequency)
     * @param mergeNested If true, nest clusters with substring keys
     * @return JSON object with all clusters
     */
    json serializeClusterMap(const std::unordered_map<std::string, WordComplexCluster>& clusters,
                             const std::unordered_map<std::string, double>& frequencies,
                             bool mergeNested = false) const;

    /**
     * @brief Serialize only lemmas from a cluster
     * @param cluster The cluster with lemmas
     * @return JSON array of lemma objects
     */
    json serializeLemmas(const WordComplexCluster& cluster) const;

    /**
     * @brief Serialize word complexes (phrases) in a cluster
     * @param cluster The cluster with phrases
     * @return JSON array of phrase objects
     */
    json serializeWordComplexes(const WordComplexCluster& cluster) const;

    /**
     * @brief Serialize semantic relations for a lemma
     * @param lemma The lemma string
     * @param cluster The cluster with relation data
     * @return JSON object with hypernyms and hyponyms
     */
    json serializeSemanticRelations(const std::string& lemma, const WordComplexCluster& cluster) const;

  private:
    /**
     * @brief Create JSON object for a single lemma
     * @param lemma The lemma string
     * @param index Position index in cluster.lemmas
     * @param cluster Reference to cluster for metrics
     * @return JSON object with lemma data
     */
    json createLemmaObject(const std::string& lemma, size_t index, const WordComplexCluster& cluster) const;

    /**
     * @brief Create JSON object for a single phrase (word complex)
     * @param wordComplex The phrase to serialize
     * @param contexts Vector of contexts to search for matching one
     * @return JSON object with phrase data
     */
    json createPhraseObject(const WordComplexPtr& wordComplex, const std::vector<TokenizedSentence>& contexts) const;

    /**
     * @brief Sort cluster keys and merge nested ones if needed
     * @param clusterMap All clusters
     * @param mergeNested Whether to perform nesting
     * @return Vector of keys in sorted order (or with nesting structure)
     */
    std::vector<std::string>
    sortKeysForSerialization(const std::unordered_map<std::string, WordComplexCluster>& clusterMap,
                             bool mergeNested) const;
};

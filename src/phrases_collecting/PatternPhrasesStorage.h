#pragma once

#include "Embedding.h"
#include "LSA.h"
#include "PhrasesCollectorUtils.h"
#include "ThreadController.h"
#include "WordComplexCluster.h"

#include <nlohmann/json.hpp>
#include <filesystem>

// Forward declarations
class ClusterMerger;

/**
 * @brief Manages storage and processing of pattern phrases with linguistic metrics
 * @details Handles phrase collection, clustering, TF-IDF/LSA metric computation,
 * semantic relation loading, and output to JSON format. Implements singleton pattern.
 */
class PatternPhrasesStorage {
    friend class ClusterMerger;

  public:
    /**
     * @brief Retrieves the singleton instance of PatternPhrasesStorage
     *
     * @return Reference to the singleton instance
     */
    static PatternPhrasesStorage& getStorage() {
        static PatternPhrasesStorage storage;
        return storage;
    }

    /**
     * @brief Processes input file and outputs results to specified directory
     *
     * @param inputFile Path to the input file
     * @param outputDir Path to output directory
     */
    void processFile(const std::filesystem::path& inputFile, const std::filesystem::path& outputDir);

    /**
     * @brief Builds and finalizes the phrase storage
     * @details Performs post-processing operations on collected phrases
     */
    void build();

    /**
     * @brief Adds a cluster to the storage
     *
     * @param key Unique identifier for the cluster
     * @param cluster The WordComplexCluster to store
     */
    void addCluster(const std::string& key, const WordComplexCluster& cluster);

    /**
     * @brief Reserves space for clusters
     *
     * @param count Expected number of clusters
     */
    void reserveClusters(size_t count);

    /**
     * @brief Finds a cluster by its key
     *
     * @param key Cluster identifier
     * @return Pointer to the cluster if found, nullptr otherwise
     */
    [[nodiscard]] WordComplexCluster* findCluster(const std::string& key);

    /**
     * @brief Associates tokenized sentences with their containing clusters
     * @details Links context sentences from corpus to word complexes in clusters
     */
    void addContextsToClusters();

    /**
     * @brief Collects phrases from word forms using simple and complex phrase patterns
     * @details Delegates to phrase collectors for pattern-based phrase extraction
     *
     * @param forms Vector of word forms with morphological information from the sentence
     * @param process Processing context containing document ID and output path
     */
    void collect(const std::vector<X::WordFormPtr>& forms, Process& process);

    /**
     * @brief Finalizes processing for the current document
     * @details Performs document-level cleanup and finalization tasks
     */
    void finalizeDocumentProcessing();

    /**
     * @brief Computes TF, IDF, and TF-IDF text metrics for all clusters
     * @details Calculates term frequency, inverse document frequency, and combined TF-IDF values
     */
    void computeTextMetrics();

    /**
     * @brief Calculates topic relevance and centrality metrics using LSA analysis
     * @details Uses U matrix, singular values, and topic words to compute semantic metrics
     *
     * @param U Left singular vectors matrix from LSA decomposition
     * @param words Vector of words in the corpus (row indices for U matrix)
     * @param Sigma Singular values matrix (diagonal)
     * @param config Configuration parameters for metric calculation
     */
    void calculateLSAMetrics(const Eigen::MatrixXd& U, const std::vector<std::string>& words,
                             const Eigen::MatrixXd& Sigma, const LSA_MetricsConfig& config);

    /**
     * @brief Simplified LSA metrics calculation using topic assignments
     * @details Alternative implementation using pre-computed topic word lists
     *
     * @param U Left singular vectors matrix from LSA decomposition
     * @param words Vector of words in the corpus
     * @param topics Map of topic ID to list of words in that topic
     */
    void calculateLSAMetrics(const MatrixXd& U, const std::vector<std::string>& words,
                             const std::unordered_map<int, std::vector<std::string>>& topics);

    /**
     * @brief Saves clusters to JSON file with optional filtering and nesting
     * @details Serializes clusters to JSON format with configurable output options
     *
     * @param filename Path to output JSON file
     * @param mergeNestedClusters If true, nests clusters whose keys are prefixes of other keys
     * @param termsOnly If true, saves only terminology candidates (from clustersToInclude);
     *                  if false, saves all clusters
     *
     * @throws std::runtime_error If file cannot be written
     */
    void saveClusters(const std::string& filename, bool mergeNestedClusters = false, bool termsOnly = false) const;

    /**
     * @brief Loads hypernym and hyponym relations from WikiWordNet database
     * @details Retrieves semantic relations for cluster lemmas with caching
     */
    void loadWikiWNRelations();

    /**
     * @brief Evaluates term relevance based on LSA topics
     *
     * @param lsa LSA analyzer containing computed topics
     */
    void evaluateTermRelevance(const LSA& lsa);

    /**
     * @brief Retrieves all clusters in storage
     *
     * @return Const reference to the clusters map
     */
    [[nodiscard]] const std::unordered_map<std::string, WordComplexCluster> getClusters() const;

    /**
     * @brief Collects and filters terminology candidates from clusters
     * @details Applies multiple filtering criteria to identify valid terms:
     * - Excludes clusters with Roman numerals
     * - Filters by TF-IDF and frequency thresholds
     * - Applies classified phrase labels
     * - Checks model-based prefix relationships
     *
     * @param tfidfThreshold Minimum TF-IDF value for inclusion (default: 0.0000088)
     */
    void collectTerms(double tfidfThreshold = 0.0000088);

    /// @brief Thread controller for managing synchronization
    ThreadController threadController;

  private:
    /**
     * @brief Calculates topic relevance using "max^2 / sum^2" method
     * @details Computes how much a cluster is concentrated in single topics
     *
     * @param cluster Cluster to evaluate
     * @param U Left singular vectors from LSA
     * @param Sigma Singular values matrix
     * @param words Corpus words (row indices)
     * @param config Metric configuration
     * @return Topic relevance score [0, 1]
     */
    [[nodiscard]] static double calculateTopicRelevance(const WordComplexCluster& cluster, const Eigen::MatrixXd& U,
                                                        const Eigen::MatrixXd& Sigma,
                                                        const std::vector<std::string>& words,
                                                        const LSA_MetricsConfig& config);

    /**
     * @brief Calculates centrality score using vector centroid method
     * @details Measures average proximity of cluster lemma vectors to cluster centroid
     *
     * @param cluster Cluster to evaluate
     * @param U Left singular vectors from LSA
     * @param Sigma Singular values matrix
     * @param words Corpus words (row indices)
     * @param config Metric configuration
     * @return Centrality score [0, 1] or similar scale
     */
    [[nodiscard]] static double calculateCentrality(const WordComplexCluster& cluster, const Eigen::MatrixXd& U,
                                                    const Eigen::MatrixXd& Sigma, const std::vector<std::string>& words,
                                                    const LSA_MetricsConfig& config);

    /**
     * @brief Calculates topic relevance as proportion of lemmas in top words
     * @details Counts how many cluster lemmas appear in identified topics
     *
     * @param cluster Cluster to evaluate
     * @param topics Map of topic ID to word list
     * @return Proportion of lemmas found in topics [0, 1]
     */
    [[nodiscard]] double calculateTopicRelevance(const WordComplexCluster& cluster,
                                                 const std::unordered_map<int, std::vector<std::string>>& topics);

    /**
     * @brief Calculates centrality as average pairwise cosine similarity
     * @details Computes mean cosine similarity between cluster lemma vectors
     *
     * @param cluster Cluster to evaluate
     * @param U Left singular vectors from LSA
     * @param words Corpus words (row indices)
     * @return Average cosine similarity [-1, 1]
     */
    [[nodiscard]] double calculateCentrality(const WordComplexCluster& cluster, const MatrixXd& U,
                                             const std::vector<std::string>& words);

    /**
     * @brief Initializes cluster set and applies TF-IDF based filtering
     * @details First stage of term collection: excludes low-value clusters
     *
     * @param tfidfThreshold Minimum TF-IDF threshold
     * @param sortedKeys Output: sorted set of candidate cluster keys
     * @param clustersToInclude Output: set of clusters passing initial filtering
     */
    void initializeAndFilterClusters(double tfidfThreshold, std::set<std::string>& sortedKeys,
                                     std::unordered_set<std::string>& clustersToInclude);

    /**
     * @brief Applies classifier labels to filter clusters
     * @details Second stage of term collection: uses external classification labels
     *
     * @param phraseLabels JSON with phrase classification labels
     * @param sortedKeys Modified: keys adjusted based on labels
     * @param clustersToInclude Modified: clusters removed if misclassified
     */
    void applyClassifiedPhrases(const nlohmann::json& phraseLabels, std::set<std::string>& sortedKeys,
                                std::unordered_set<std::string>& clustersToInclude);

    /**
     * @brief Checks and removes prefix-related clusters
     * @details Third stage of term collection: handles model-based relationships
     * Removes clusters that are prefixes of others if metrics don't support inclusion
     *
     * @param sortedKeys Modified: related clusters removed
     * @param clustersToInclude Modified: clusters removed based on relationships
     */
    void checkModelPrefixRelationships(std::set<std::string>& sortedKeys,
                                       std::unordered_set<std::string>& clustersToInclude);

    /// @brief Cache of hypernym relations to avoid repeated DB queries
    std::unordered_map<std::string, std::set<std::string>> hypernymCache;

    /// @brief Cache of hyponym relations to avoid repeated DB queries
    std::unordered_map<std::string, std::set<std::string>> hyponymCache;

    /// @brief Set of cluster keys selected as terminology candidates
    std::unordered_set<std::string> clustersToInclude;

    /**
     * @brief Default constructor (private for singleton pattern)
     */
    PatternPhrasesStorage() {
    }

    /**
     * @brief Default destructor (private for singleton pattern)
     */
    ~PatternPhrasesStorage() {
    }

    /// @brief Deleted copy constructor to enforce singleton pattern
    PatternPhrasesStorage(const PatternPhrasesStorage&) = delete;

    /// @brief Deleted assignment operator to enforce singleton pattern
    PatternPhrasesStorage& operator=(const PatternPhrasesStorage&) = delete;

    /// @brief Map of cluster key to WordComplexCluster data
    std::unordered_map<std::string, WordComplexCluster> clusters;
};
#pragma once

#include "Embedding.h"
#include "LSA.h"
#include "PhrasesCollectorUtils.h"
#include "ThreadController.h"
#include "WordComplexCluster.h"

#include <nlohmann/json.hpp>

// Forward declarations
class ClusterMerger;

// \class PatternPhrasesStorage
// \brief This class manages the storage and processing of pattern phrases. It includes methods for collecting phrases,
//        adding word complexes, computing text metrics, and outputting data to text and JSON files.
class PatternPhrasesStorage {
    friend class ClusterMerger;

  public:
    // \brief Gets the singleton instance of PatternPhrasesStorage.
    // \return          Reference to the singleton instance of PatternPhrasesStorage.
    static PatternPhrasesStorage& GetStorage() {
        static PatternPhrasesStorage storage;
        return storage;
    }

    void ProcessFile(const fs::path& inputFile, const fs::path& outputDir);

    void build();

    void AddCluster(const std::string& key, const WordComplexCluster& cluster);

    void ReserveClusters(size_t count);

    WordComplexCluster* FindCluster(const std::string& key);

    void AddContextsToClusters();

    // \brief Collects phrases from the provided word forms and process.
    // \param forms     A vector of WordFormPtr representing the sentence to analyze.
    // \param process   The process used for phrase collection.
    void Collect(const std::vector<X::WordFormPtr>& forms, Process& process);

    void FinalizeDocumentProcessing();

    // \brief Computes text metrics such as TF, IDF, and TF-IDF for the stored word complexes.
    void ComputeTextMetrics();

    // Сalculates topicRelevance and centralityScore metrics for all clusters after an LSA analysis.
    void CalculateLSAMetrics(const Eigen::MatrixXd& U, const std::vector<std::string>& words,
                             const Eigen::MatrixXd& Sigma, const LSA_MetricsConfig& config);

    // Simplified implementation
    void CalculateLSAMetrics(const MatrixXd& U, const std::vector<std::string>& words,
                             const std::unordered_map<int, std::vector<std::string>>& topics);

    /**
     * @brief Saves clusters to a JSON file with optional filtering
     *
     * @param filename Path to output JSON file
     * @param mergeNestedClusters If true, nests clusters whose keys are prefixes of other keys
     * @param termsOnly If true, saves only terminology candidates;
     *                  if false, saves all clusters
     *
     * @throws std::runtime_error If file cannot be written
     */
    void saveClusters(const std::string& filename, bool mergeNestedClusters = false, bool termsOnly = false) const;

    void LoadWikiWNRelations();

    void EvaluateTermRelevance(const LSA& lsa);
    const std::unordered_map<std::string, WordComplexCluster> GetClusters() const;

    void CollectTerms(double tfidfThreshold = 0.0000088);

    ThreadController threadController; ///< Controller for managing thread synchronization.

  private:
    // Topic relevance calculation for a cluster based on row vectors U (and optionally Sigma) using the "max^2 /
    // sum^2" method
    static double CalculateTopicRelevance(const WordComplexCluster& cluster, const Eigen::MatrixXd& U,
                                          const Eigen::MatrixXd& Sigma, const std::vector<std::string>& words,
                                          const LSA_MetricsConfig& config);

    // Centrality score calculation through the mean cosine or through the Euclidean measure
    static double CalculateCentrality(const WordComplexCluster& cluster, const Eigen::MatrixXd& U,
                                      const Eigen::MatrixXd& Sigma, const std::vector<std::string>& words,
                                      const LSA_MetricsConfig& config);

    // Proportion of cluster lemmas that occur among the top words for the identified topics
    double CalculateTopicRelevance(const WordComplexCluster& cluster,
                                   const std::unordered_map<int, std::vector<std::string>>& topics);

    // Average pairwise cosine proximity of the vectors (rows) of the matrix U corresponding to the cluster lemmas.
    double CalculateCentrality(const WordComplexCluster& cluster, const MatrixXd& U,
                               const std::vector<std::string>& words);

  private:
    Options& options = Options::getOptions();
    void InitializeAndFilterClusters(double tfidfThreshold, std::set<std::string>& sortedKeys,
                                     std::unordered_set<std::string>& clustersToInclude);

    void ApplyClassifiedPhrases(const nlohmann::json& phraseLabels, std::set<std::string>& sortedKeys,
                                std::unordered_set<std::string>& clustersToInclude);

    void CheckModelPrefixRelationships(std::set<std::string>& sortedKeys,
                                       std::unordered_set<std::string>& clustersToInclude);

    std::unordered_map<std::string, std::set<std::string>> hypernymCache;
    std::unordered_map<std::string, std::set<std::string>> hyponymCache;

    std::unordered_set<std::string> clustersToInclude;

    // \brief Default constructor.
    PatternPhrasesStorage() {
    }

    // \brief Default destructor.
    ~PatternPhrasesStorage() {
    }

    // \brief Deleted copy constructor to enforce singleton pattern.
    PatternPhrasesStorage(const PatternPhrasesStorage&) = delete;

    // \brief Deleted assignment operator to enforce singleton pattern.
    PatternPhrasesStorage& operator=(const PatternPhrasesStorage&) = delete;
    std::unordered_map<std::string, WordComplexCluster> clusters; ///< Map of word complex clusters.
};

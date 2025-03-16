#ifndef PATTERN_PHRASES_STORAGE_H
#define PATTERN_PHRASES_STORAGE_H

#include <ComplexPhrasesCollector.h>
#include <Embedding.h>
#include <LSA.h>
#include <SemanticRelations.h>
#include <TextCorpus.h>
#include <ThreadController.h>
#include <regex>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <condition_variable>
#include <mutex>

using namespace PhrasesCollectorUtils;
// using CoOccurrenceMap = std::unordered_map<std::string, std::unordered_map<std::string, int>>;

// \struct WordComplexCluster
// \brief This structure represents a cluster of word complexes, including their TF, IDF, and TF-IDF values, as well
// as FastText vectors.
struct WordComplexCluster {
    size_t phraseSize; ///< Size of the phrase.
    bool tagMatch;     ///< Indicates if the cluster matches a topic.
    double frequency;
    double topicRelevance;
    double centralityScore;
    std::string key;       ///< String with normalized words.
    std::string modelName; ///< Name of the model associated with the cluster.
    std::vector<std::string> lemmas;
    std::vector<WordComplexPtr> wordComplexes;                        ///< Vector of word complexes in the cluster.
    std::vector<double> tf;                                           ///< Vector of TF values for the words.
    std::vector<double> idf;                                          ///< Vector of IDF values for the words.
    std::vector<double> tfidf;                                        ///< Vector of TF-IDF values for the words.
    std::vector<WordEmbeddingPtr> wordVectors;                        ///< Vector of FastText vectors for the words.
    std::unordered_map<std::string, std::set<std::string>> hypernyms; ///< Hypernyms for each word in the phrase.
    std::unordered_map<std::string, std::set<std::string>> hyponyms;  ///< Hyponyms for each word in the phrase.
    std::unordered_set<std::string> synonyms;
    std::vector<TokenizedSentence> contexts;
    bool is_term;
};

// \class PatternPhrasesStorage
// \brief This class manages the storage and processing of pattern phrases. It includes methods for collecting phrases,
//        adding word complexes, computing text metrics, and outputting data to text and JSON files.
class PatternPhrasesStorage {
public:
    // \brief Gets the singleton instance of PatternPhrasesStorage.
    // \return          Reference to the singleton instance of PatternPhrasesStorage.
    static PatternPhrasesStorage& GetStorage()
    {
        static PatternPhrasesStorage storage;
        return storage;
    }

    void AddCluster(const std::string& key, const WordComplexCluster& cluster);

    void ReserveClusters(size_t count);

    WordComplexCluster* FindCluster(const std::string& key);

    void AddContextsToClusters();

    void MergeSimilarClusters();

    bool AreKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff = 3, size_t endLength = 2,
                        bool CheckFirstOnly = false);

    // \brief Collects phrases from the provided word forms and process.
    // \param forms     A vector of WordFormPtr representing the sentence to analyze.
    // \param process   The process used for phrase collection.
    void Collect(const std::vector<WordFormPtr>& forms, Process& process);

    void FinalizeDocumentProcessing();

    // \brief Computes text metrics such as TF, IDF, and TF-IDF for the stored word complexes.
    void ComputeTextMetrics();

    // Сalculates topicRelevance and centralityScore metrics for all clusters after an LSA analysis.
    void CalculateLSAMetrics(const Eigen::MatrixXd& U, const std::vector<std::string>& words,
                             const Eigen::MatrixXd& Sigma, const LSA_MetricsConfig& config);

    // Simplified implementation
    void CalculateLSAMetrics(const MatrixXd& U, const std::vector<std::string>& words,
                             const std::unordered_map<int, std::vector<std::string>>& topics);

    // \brief Outputs the clusters to a JSON file.
    // \param filename  The path to the output JSON file.
    void OutputClustersToJsonFile(const std::string& filename, bool mergeNestedClusters = false,
                                  bool termsOnly = false) const;

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
    PatternPhrasesStorage()
    {
    }

    // \brief Default destructor.
    ~PatternPhrasesStorage()
    {
    }

    int lastDocumentId = -1;
    std::unordered_set<std::string> uniqueLemmasInDoc;

    // \brief Deleted copy constructor to enforce singleton pattern.
    PatternPhrasesStorage(const PatternPhrasesStorage&) = delete;

    // \brief Deleted assignment operator to enforce singleton pattern.
    PatternPhrasesStorage& operator=(const PatternPhrasesStorage&) = delete;
    std::unordered_map<std::string, WordComplexCluster> clusters; ///< Map of word complex clusters.
};

#endif // PATTERN_PHRASES_STORAGE_H

#ifndef PATTERN_PHRASES_STORAGE_H
#define PATTERN_PHRASES_STORAGE_H

#include <ComplexPhrasesCollector.h>
#include <Embedding.h>
#include <SemanticRelations.h>
#include <TextCorpus.h>
#include <ThreadController.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <condition_variable>
#include <mutex>

using namespace PhrasesCollectorUtils;
// using CoOccurrenceMap = std::unordered_map<std::string, std::unordered_map<std::string, int>>;

// \class PatternPhrasesStorage
// \brief This class manages the storage and processing of pattern phrases. It includes methods for collecting phrases,
//        adding word complexes, computing text metrics, and outputting data to text and JSON files.
class PatternPhrasesStorage {
    // \struct WordComplexCluster
    // \brief This structure represents a cluster of word complexes, including their TF, IDF, and TF-IDF values, as well
    // as FastText vectors.
    struct WordComplexCluster {
        size_t phraseSize;     ///< Size of the phrase.
        double m_weight;       ///< Weight of the cluster.
        bool topicMatch;       ///< Indicates if the cluster matches a topic.
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
    };

public:
    // \brief Gets the singleton instance of PatternPhrasesStorage.
    // \return          Reference to the singleton instance of PatternPhrasesStorage.
    static PatternPhrasesStorage& GetStorage()
    {
        static PatternPhrasesStorage storage;
        return storage;
    }

    void Deserialize(const json& j);

    void LoadStorageFromFile(const std::string& filename);

    void LoadPhraseStorageFromResultsDir();

    void MergeSimilarClusters();

    bool AreKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff = 3);

    // \brief Collects phrases from the provided word forms and process.
    // \param forms     A vector of WordFormPtr representing the sentence to analyze.
    // \param process   The process used for phrase collection.
    void Collect(const std::vector<WordFormPtr>& forms, Process& process);

    void FinalizeDocumentProcessing();

    // void AddSemanticRelationsToCluster(WordComplexCluster& cluster);

    // \brief Computes text metrics such as TF, IDF, and TF-IDF for the stored word complexes.
    void ComputeTextMetrics();

    // \brief Outputs the clusters to a JSON file.
    // \param filename  The path to the output JSON file.
    void OutputClustersToJsonFile(const std::string& filename) const;

    // \brief Calculates weights for the word complexes (not used).
    void CalculateWeights();

    ThreadController threadController; ///< Controller for managing thread synchronization.

private:
    // TextCorpus corpus; ///< The text corpus used for analysis and metrics computation.
    //  CoOccurrenceMap coOccurrenceMap;
    ::Embedding embedding;
    SemanticRelationsDB semanticDB;

    std::unordered_map<std::string, std::set<std::string>> hypernymCache;
    std::unordered_map<std::string, std::set<std::string>> hyponymCache;

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

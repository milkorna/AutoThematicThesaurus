#ifndef PATTERN_PHRASES_STORAGE_H
#define PATTERN_PHRASES_STORAGE_H

#include <ComplexPhrasesCollector.h>
#include <TextCorpus.h>
#include <ThreadController.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <condition_variable>
#include <mutex>

using namespace PhrasesCollectorUtils;

class PatternPhrasesStorage {

    struct WordComplexCluster {
        size_t phraseSize;
        double m_weight;
        bool topicMatch;
        std::string key; // string with normalized words
        std::string modelName;
        std::vector<WordComplexPtr> wordComplexes; // maybe set
        std::vector<double> tf;
        std::vector<double> idf;
        std::vector<double> tfidf;
        std::vector<std::shared_ptr<fasttext::Vector>> wordVectors;
    };

public:
    static PatternPhrasesStorage& GetStorage()
    {
        static PatternPhrasesStorage storage;
        return storage;
    }

    TextCorpus& GetCorpus()
    {
        return corpus;
    }

    void Collect(const std::vector<WordFormPtr>& forms, Process& process);

    void AddPhrase(const std::string& phrase);

    const std::vector<std::string>& GetPhrases() const;

    void AddWordComplex(const WordComplexPtr& wc);
    void AddWordComplexes(const std::vector<PhrasesCollectorUtils::WordComplexPtr> collection);

    void ComputeTextMetrics();
    void OutputClustersToTextFile(const std::string& filename) const;
    void OutputClustersToJsonFile(const std::string& filename) const;

    // not used
    void CalculateWeights();

    ThreadController::ThreadController threadController;

private:
    TextCorpus corpus;

    PatternPhrasesStorage()
    {
    }
    ~PatternPhrasesStorage()
    {
    }
    PatternPhrasesStorage(const PatternPhrasesStorage&) = delete;
    PatternPhrasesStorage& operator=(const PatternPhrasesStorage&) = delete;

    std::vector<std::string> phrases;
    std::unordered_map<std::string, WordComplexCluster> clusters;
};

#endif // PATTERN_PHRASES_STORAGE_H
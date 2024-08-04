#ifndef PATTERN_PHRASES_STORAGE_H
#define PATTERN_PHRASES_STORAGE_H

#include <ComplexPhrasesCollector.h>

#include <condition_variable>
#include <mutex>

#include <ThreadController.h>

using namespace PhrasesCollectorUtils;

class PatternPhrasesStorage {

    struct WordComplexCluster {
        size_t phraseSize;
        double m_weight;
        bool topicMatch;
        std::string key; // string with normalized words
        std::string modelName;
        std::vector<WordComplexPtr> wordComplexes; // maybe set
    };

public:
    static PatternPhrasesStorage& GetStorage()
    {
        static PatternPhrasesStorage storage;
        return storage;
    }

    void Collect(const std::vector<WordFormPtr>& forms, Process& process);

    void AddPhrase(const std::string& phrase);

    const std::vector<std::string>& GetPhrases() const;

    void AddWordComplex(const WordComplexPtr& wc);
    void AddWordComplexes(const std::vector<PhrasesCollectorUtils::WordComplexPtr> collection);

    void OutputClustersToFile(const std::string& filename) const;

    ThreadController::ThreadController threadController;

private:
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
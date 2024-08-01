#ifndef PATTERN_PHRASES_STORAGE_H
#define PATTERN_PHRASES_STORAGE_H

#include <ComplexPhrasesCollector.h>

#include <mutex>

using namespace PhrasesCollectorUtils;

class PatternPhrasesStorage {

    struct WordComplexCluster {
        size_t phraseSize;
        double m_weight;
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
    std::mutex mutex_;
};

#endif // PATTERN_PHRASES_STORAGE_H
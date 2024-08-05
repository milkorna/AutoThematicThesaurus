#ifndef PHRASES_COLLECTOR_UTILS_H
#define PHRASES_COLLECTOR_UTILS_H

#include <xmorphy/morph/WordForm.h>

#include <ModelComponent.h>
#include <PatternParser.h>
#include <PhrasesCollectorUtils.h>
#include <WordComplex.h>

#include <filesystem>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace PhrasesCollectorUtils {

    struct Options {
        bool multithreading = true;
        bool cleaningStopWords = true;
        bool tags = false;
    };

    extern Options g_options;

    struct CurrentPhraseStatus {
        size_t correct = 0;
        bool headIsMatched = false;
        bool headIsChecked = false;
        bool foundLex = false;
    };

    std::vector<fs::path> GetFilesToProcess();

    void ProcessFile(const fs::path& inputFile, const fs::path& outputDir, int& counter, std::mutex& counterMutex);
    void BuildPhraseStorage();

    MorphInfo GetMostProbableMorphInfo(const std::unordered_set<X::MorphInfo>& morphSet);
    bool MorphAnanlysisError(const WordFormPtr& token);
    bool HaveSp(const std::unordered_set<X::MorphInfo>& currFormMorphInfo);
    bool CheckForMisclassifications(const X::WordFormPtr& form);

    void LogCurrentSimplePhrase(const WordComplexPtr& curSimplePhr);
    void LogCurrentComplexModel(const std::string& name);

    void UpdatePhraseStatus(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase,
                            CurrentPhraseStatus& curPhrStatus, bool isLeft);

    const std::unordered_set<std::string> GetTopics();
    const std::unordered_set<std::string> GetStopWords();
    void OutputResults(const std::vector<WordComplexPtr>& collection, Process& process);
}

#endif // PHRASES_COLLECTOR_UTILS_H
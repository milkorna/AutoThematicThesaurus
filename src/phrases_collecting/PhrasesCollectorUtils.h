#ifndef PHRASES_COLLECTOR_UTILS_H
#define PHRASES_COLLECTOR_UTILS_H

#include <xmorphy/morph/WordForm.h>

#include <Embedding.h>
#include <ModelComponent.h>
#include <PatternParser.h>
#include <PhrasesCollectorUtils.h>
#include <TextCorpus.h>
#include <WordComplex.h>

#include <filesystem>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace PhrasesCollectorUtils {

    // \struct Options
    // \brief This structure holds configuration options for phrase collection.
    struct Options {
        int textToProcessCount;
        int tresholdTopicsCount;
        bool cleanStopWords; ///< Indicates if stop words should be cleaned.
        bool validateBoundaries;
        float topicsThreshold;
        float topicsHyponymThreshold;
        float freqTresholdCoeff;

        fs::path myDataDir;
        fs::path corpusDir;
        fs::path textsDir;
        fs::path patternsFile;
        fs::path stopWordsFile;
        fs::path tagsAndHubsFile;
        fs::path resDir;
        fs::path corpusFile;
        fs::path filteredCorpusFile;
        fs::path sentencesFile;
        fs::path embeddingModelFile;

        static Options& getOptions()
        {
            static Options options;
            return options;
        }

        Options(const Options&) = delete;
        Options& operator=(const Options&) = delete;

    private:
        Options();
    };

    // \struct CurrentPhraseStatus
    // \brief This structure holds the status of the current phrase during processing.
    struct CurrentPhraseStatus {
        size_t correct = 0;         ///< Number of correctly identified components in the phrase.
        bool headIsMatched = false; ///< Indicates if the head of the phrase is matched.
        bool headIsChecked = false; ///< Indicates if the head of the phrase is checked.
        bool foundLex = false;      ///< Indicates if a lexical item was found.
    };

    // \brief Retrieves a list of files to process.
    // \return              A vector of paths to the files to be processed.
    std::vector<fs::path> GetFilesToProcess();

    std::vector<fs::path> GetResFiles();

    // \brief Processes a single file and outputs the results to the specified directory.
    // \param inputFile     The path to the input file.
    // \param outputDir     The directory where the output will be saved.
    // \param counter       A reference to a counter for processed files.
    // \param counterMutex  A mutex for synchronizing access to the counter.
    void ProcessFile(const fs::path& inputFile, const fs::path& outputDir, int& counter, std::mutex& counterMutex);

    // \brief Builds the phrase storage for processing.
    void BuildPhraseStorage();

    void BuildTokenizedSentenceCorpus();

    // \brief Retrieves the most probable morphological information from a set.
    // \param morphSet      A set of morphological information.
    // \return              The most probable MorphInfo object.
    MorphInfo GetMostProbableMorphInfo(const std::unordered_set<X::MorphInfo>& morphSet);

    // \brief Checks if there is an error in morphological analysis.
    // \param token         The WordFormPtr token to check.
    // \return              True if there is an error, false otherwise.
    bool MorphAnanlysisError(const WordFormPtr& token);

    // \brief Checks if the current form has a specific morphological property.
    // \param currFormMorphInfo A set of morphological information of the current form.
    // \return                  True if the property is found, false otherwise.
    bool HaveSp(const std::unordered_set<X::MorphInfo>& currFormMorphInfo);

    // \brief Logs the current simple phrase being processed.
    // \param curSimplePhr  A shared pointer to the current simple phrase.
    void LogCurrentSimplePhrase(const WordComplexPtr& curSimplePhr);

    // \brief Logs the current complex model being processed.
    // \param name          The name of the complex model.
    void LogCurrentComplexModel(const std::string& name);

    // \brief Updates the status of the current phrase based on the adjacent phrase.
    // \param wc            A shared pointer to the current word complex.
    // \param asidePhrase   A shared pointer to the adjacent phrase.
    // \param curPhrStatus  A reference to the current phrase status.
    // \param isLeft        Boolean indicating if the update is for the left side.
    void UpdatePhraseStatus(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase,
                            CurrentPhraseStatus& curPhrStatus, bool isLeft);

    // \brief Retrieves the set of topics for phrase collection.
    // \return              A set of topic strings.
    const std::unordered_set<std::string> GetTopics();

    const std::unordered_map<std::string, WordEmbeddingPtr>& GetTopicVectors();

    // \brief Retrieves the set of stop words for cleaning.
    // \return              A set of stop word strings.
    const std::unordered_set<std::string> GetStopWords();

    // \brief Outputs the results of the phrase collection process.
    // \param collection    A vector of collected word complexes.
    // \param process       The process associated with the phrase collection.
    void OutputResults(const std::vector<WordComplexPtr>& collection, Process& process);

    const std::string GetLemma(const WordFormPtr& form);
}

#endif // PHRASES_COLLECTOR_UTILS_H
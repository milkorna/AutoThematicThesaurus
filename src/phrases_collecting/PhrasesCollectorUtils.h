#pragma once

#include "xmorphy/morph/WordForm.h"

#include "Embedding.h"
#include "WordComplex.h"

#include <filesystem>
#include <mutex>
#include <unordered_set>

namespace fs = std::filesystem;

// \struct Options
// \brief This structure holds configuration options for phrase collection.
struct Options {
    int textToProcessCount;
    int tresholdTopicsCount;
    bool cleanStopWords; ///< Indicates if stop words should be cleaned.
    bool validateBoundaries;
    double topicsThreshold;
    double topicsHyponymThreshold;
    double freqTresholdCoeff;

    fs::path dataDir;
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
    fs::path totalResultsPath;
    fs::path termsCandidatesPath;

    static Options& getOptions() {
        static Options options;
        return options;
    }

    Options(const Options&) = delete;
    Options& operator=(const Options&) = delete;

    void recomputeCorpusDependenciesPaths();
    void updateFileCount();

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

// \brief Checks if there is an error in morphological analysis.
// \param token         The WordFormPtr token to check.
// \return              True if there is an error, false otherwise.
bool MorphAnanlysisError(const X::WordFormPtr& token);

void RemoveSeparatorTokens(std::vector<X::WordFormPtr>& forms);

// \brief Checks if the current form has a specific morphological property.
// \param currFormMorphInfo A set of morphological information of the current form.
// \return                  True if the property is found, false otherwise.
bool HaveSp(const std::unordered_set<X::MorphInfo>& currFormMorphInfo);

const std::string GetLowerCase(const std::string& line);

// \brief Outputs the results of the phrase collection process.
// \param collection    A vector of collected word complexes.
// \param process       The process associated with the phrase collection.
void OutputResults(const std::vector<WordComplexPtr>& collection, Process& process);

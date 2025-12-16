#pragma once

#include "xmorphy/morph/WordForm.h"

#include "Logger.h"

#include <filesystem>

namespace fs = std::filesystem;

/**
 * @struct Options
 * @brief Configuration options for phrase collection and text processing.
 * @details Implements Singleton pattern to manage global configuration settings
 *          including file paths, thresholds, and processing parameters.
 */
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

    /**
     * @brief Gets the singleton instance of Options.
     *
     * @return Reference to the static Options instance.
     */
    [[nodiscard]] static Options& getOptions() {
        static Options options;
        return options;
    }

    /**
     * @brief Deleted copy constructor.
     */
    Options(const Options&) = delete;

    /**
     * @brief Deleted copy assignment operator.
     */
    Options& operator=(const Options&) = delete;

    /**
     * @brief Recomputes corpus-dependent file paths.
     * @details Updates paths if corpusDir has been changed from default.
     */
    void recomputeCorpusDependenciesPaths();

    /**
     * @brief Updates the count of files to process.
     * @details Iterates through textsDir and counts regular files.
     *          Exits application if no files found or on error.
     */
    void updateFileCount();

  private:
    /**
     * @brief Private constructor for Singleton pattern.
     * @details Initializes all paths relative to current working directory
     *          and sets default configuration values.
     */
    Options();
};

inline Options::Options() {
    fs::path repoPath = fs::current_path();

    dataDir = repoPath / "my_data";
    corpusDir = dataDir / "nlp_corpus";
    textsDir = corpusDir / "texts";
    patternsFile = dataDir / "patterns.json";
    stopWordsFile = dataDir / "stop_words";
    tagsAndHubsFile = corpusDir / "tags_and_hubs";
    resDir = corpusDir / "results";
    corpusFile = corpusDir / "corpus";
    filteredCorpusFile = corpusDir / "filtered_corpus";
    sentencesFile = corpusDir / "sentences.json";
    embeddingModelFile = repoPath / "my_custom_fasttext_model_finetuned.bin";
    totalResultsPath = corpusDir / "total_results.json";
    termsCandidatesPath = corpusDir / "term_candidates.json";

    textToProcessCount = 0;
    tresholdTopicsCount = 7;
    cleanStopWords = true;
    validateBoundaries = true;
    topicsThreshold = 0.6;
    topicsHyponymThreshold = 0.98;
    freqTresholdCoeff = 0.12;
}

inline void Options::recomputeCorpusDependenciesPaths() {
    if (!fs::equivalent(corpusDir, dataDir / "nlp_corpus")) {
        textsDir = corpusDir / "texts";
        tagsAndHubsFile = corpusDir / "tags_and_hubs";
        resDir = corpusDir / "results";
        corpusFile = corpusDir / "corpus";
        filteredCorpusFile = corpusDir / "filtered_corpus";
        sentencesFile = corpusDir / "sentences.json";
        totalResultsPath = corpusDir / "total_results.json";
        termsCandidatesPath = corpusDir / "term_candidates.json";
    }
}

inline void Options::updateFileCount() {
    int fileCount = 0;
    try {
        for (const auto& entry : fs::directory_iterator(textsDir)) {
            if (entry.is_regular_file()) {
                ++fileCount;
            }
        }
        textToProcessCount = fileCount;
    } catch (const std::exception& ex) {
        Logger::log("Options", LogLevel::Error,
                    std::string("Failed to iterate over textsDir: ") + ex.what() + ". Exiting.");
        Logger::flushLogs();
        std::exit(EXIT_FAILURE);
    } catch (...) {
        Logger::log("Options", LogLevel::Error, "Unknown error while counting files in textsDir. Exiting.");
        Logger::flushLogs();
        std::exit(EXIT_FAILURE);
    }

    if (textToProcessCount == 0) {
        Logger::log("Options", LogLevel::Error, "No files to process. Exiting");
        Logger::flushLogs();
        std::exit(EXIT_FAILURE);
    }
}

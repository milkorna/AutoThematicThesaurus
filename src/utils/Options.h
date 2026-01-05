#pragma once

#include <filesystem>

namespace fs = std::filesystem;

/**
 * @struct Options
 * @brief Configuration options for phrase collection and text processing.
 * @details Implements Singleton pattern to manage global configuration settings
 *          including file paths, thresholds, and processing parameters.
 */
struct Options {
    int totalDocuments;
    int thresholdTopicsCount;
    bool cleanStopWords; ///< Indicates if stop words should be cleaned.
    bool validateBoundaries;
    bool mergeDocumentTitleAndText;
    double topicsThreshold;
    double topicsHyponymThreshold;
    double freqThresholdCoeff;

    fs::path dataDir;
    fs::path corpusDir;
    fs::path rawDataFile;
    fs::path patternsFile;
    fs::path stopWordsFile;
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
     * @brief Sets corpus directory and recomputes all corpus-dependent paths.
     * @details Updates corpusDir and all related paths in a single operation.
     * @param newCorpusDir The new corpus directory path
     */
    void setCorpusDir(const fs::path& newCorpusDir);

    /**
     * @brief Updates the count of documents from corpus metadata.
     * @details Reads "metadata.total_documents" from corpus JSON file.
     *          Exits application if file not found or parsing fails.
     */
    void updateDocumentCount();

  private:
    /**
     * @brief Private constructor for Singleton pattern.
     * @details Initializes all paths relative to current working directory
     *          and sets default configuration values.
     */
    Options();
};

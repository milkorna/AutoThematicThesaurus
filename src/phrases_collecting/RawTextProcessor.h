#pragma once

#include "Embedding.h"
#include "LSA.h"
#include "PhrasesCollectorUtils.h"
#include "ThreadController.h"

#include <nlohmann/json.hpp>

/**
 * @brief Processes raw text files extracts grammatical phrases from them
 */
class RawTextProcessor {
  public:
    /**
     * @brief Retrieves the singleton instance of RawTextProcessor.
     */
    static RawTextProcessor& GetProcessor() {
        static RawTextProcessor processor;
        return processor;
    }

    /**
     * @brief Processes all raw text files in the configured input directory.
     * @details This is the main entry point. It:
     *          1. Reads all input files from Options::textsDir
     *          2. Loads text content into corpus
     *          3. Processes each file to extract phrases
     *          4. Saves the final corpus to disk
     */
    void processRawData();

    /// Destructor
    ~RawTextProcessor() = default;

    // Delete copy constructor and assignment operator to enforce singleton
    RawTextProcessor(const RawTextProcessor&) = delete;
    RawTextProcessor& operator=(const RawTextProcessor&) = delete;

  private:
    /// Private constructor - enforces singleton pattern
    RawTextProcessor() = default;

    /**
     * @brief Processes a single text file and extracts all phrases.
     *
     * @param inputFile Path to input text file (.txt)
     * @param outputDir Path to directory where results will be saved
     *                  (result file will be named "res_<filename>.json")
     */
    void processFile(const fs::path& inputFile, const fs::path& outputDir);

    /**
     * @brief Analyzes a single sentence and extracts all phrases.
     *
     * CORPUS UPDATES:
     * - Updates word frequency for each lemma in sentence
     * - Accumulates lemmas in uniqueLemmasInDoc for document stats
     * - When document changes, updates document frequency for all accumulated lemmas
     *
     * PHRASE COLLECTION:
     * - SimplePhrasesCollector: Finds phrases matching grammar patterns
     * - ComplexPhrasesCollector: Extends simple phrases with additional patterns
     *
     * @param forms Morphological word forms from xmorphy analyzer.
     *              Result of Processor::analyze() after disambiguation
     * @param process Current processing context (docNum, sentNum, file info)
     */
    void collect(const std::vector<X::WordFormPtr>& forms, Process& process);

    /**
     * @brief Finalizes processing for the current document.
     * @details Called at the end of ProcessFile() to clean up the last document.
     *
     * OPERATIONS:
     * - Updates document frequencies for all lemmas in uniqueLemmasInDoc
     * - Clears the accumulated lemmas set
     */
    void finalizeDocumentProcessing();

    /// Tracks the last processed document ID to detect document boundaries.
    /// Initialized to -1 to trigger document boundary on first sentence.
    /// Updated in Collect() after processing a sentence.
    int lastDocumentId = -1;

    /// Accumulates unique lemmas within the current document.
    /// Used to track which lemmas appeared in a document for IDF calculations.
    /// Cleared after each document boundary (when docNum changes).
    std::unordered_set<std::string> uniqueLemmasInDoc;

    /// Reference to global options/configuration singleton
    Options& options = Options::getOptions();
};

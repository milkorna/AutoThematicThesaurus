#pragma once

#include "DocumentRecord.h"
#include "Options.h"
#include "Process.h"

#include "xmorphy/morph/WordForm.h"

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

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
     * @brief Processes all raw texts in the configured input directory.
     */
    void processRawData(const std::vector<DocumentRecord>& documents);

    /// Destructor
    ~RawTextProcessor() = default;

    // Delete copy constructor and assignment operator to enforce singleton
    RawTextProcessor(const RawTextProcessor&) = delete;
    RawTextProcessor& operator=(const RawTextProcessor&) = delete;

  private:
    /// Private constructor - enforces singleton pattern
    RawTextProcessor() = default;

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
     * @param process Current processing context (docId, sentNum, file info)
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
    std::string lastDocumentId = "";

    /// Accumulates unique lemmas within the current document.
    /// Used to track which lemmas appeared in a document for IDF calculations.
    /// Cleared after each document boundary (when docId changes).
    std::unordered_set<std::string> uniqueLemmasInDoc;

    /// Reference to global options/configuration singleton
    Options& options = Options::getOptions();
};

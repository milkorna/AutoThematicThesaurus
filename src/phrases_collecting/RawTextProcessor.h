#pragma once

#include "Document.h"
#include "Process.h"

#include "xmorphy/morph/WordForm.h"

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
     * @brief Processes all raw texts in the configured input directory.
     */
    void processRawData(std::vector<Document>& documents);

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
     * @param currentDoc Current document being processed
     */
    void collect(const std::vector<X::WordFormPtr>& forms, Process& process, Document& currentDoc);
};

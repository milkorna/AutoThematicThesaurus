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
     * @brief Collects phrases and builds normalized sentence
     * @details Updates word frequency, document lemmas, collects phrases,
     *          and builds lemmatized version of sentence
     *
     * @param forms Morphologically analyzed sentence
     * @param process Process context for phrase collection
     * @param currentDoc Document being processed
     * @param corpus Reference to vocabulary corpus
     * @return Normalized sentence with lemmatized words (space-separated)
     */
    std::string processSentence(const std::vector<X::WordFormPtr>& forms, Process& process, Document& currentDoc);
};

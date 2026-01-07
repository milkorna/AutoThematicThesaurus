#pragma once

#include "WordComplex.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>

/**
 * @brief Processing context for document phrase collection and JSON output
 * @details Manages document metadata, output file, and accumulated JSON results
 * from phrase extraction operations. Automatically loads existing data on construction
 * and saves accumulated results to file on destruction.
 */
class Process {

  public:
    /**
     * @brief Constructs Process context for document and sentence
     * @details Loads existing JSON file if present, otherwise creates new array.
     * Handles corrupted files by creating empty array with error logging.
     *
     * @param docId Unique document identifier
     * @param outputFile Path to output JSON file
     * @param sentNum Initial sentence number (default: 0)
     */
    explicit Process(const std::string& docId, const std::filesystem::path& outputFile, size_t sentNum = 0);

    /**
     * @brief Destructor: writes accumulated JSON to file
     * @details Saves jsonData array to file with 4-space indentation.
     * Logs success or failure of write operation.
     */
    ~Process();

    /**
     * @brief Sets the raw sentence text and initializes token positions
     * @details Must be called after tokenization but before removeSeparators()
     * to properly track character positions of non-separator tokens.
     *
     * @param sentence The raw sentence text from SentenceSplitter
     * @param tokens Vector of tokens with their types and positions
     */
    void setSentenceData(const std::string& sentence, const std::vector<std::pair<size_t, size_t>>& tokenSpans,
                         size_t sentenceOffset = 0);

    /**
     * @brief Outputs phrase extraction results to JSON
     * @details Processes word complexes and converts them to JSON objects.
     * For each phrase, creates entry with:
     * - Lemma key (space-separated lemmas)
     * - Text form and model name
     * - Document ID and sentence number
     * - Token indices (start, end)
     * - Character span calculated from tokenPositions
     * - Lemma list
     *
     * @param phrases Vector of extracted word complexes to serialize
     */
    void outputResults(const std::vector<WordComplexPtr>& phrases);

    /**
     * @brief Increments sentence number for next sentence processing
     * @details Called after processing all phrases in current sentence.
     * This advances the sentence counter for the next sentence in the document.
     */
    void nextSentence();

    // Accessors for document context
    [[nodiscard]] const std::string& getDocId() const;

    [[nodiscard]] size_t getSentNum() const;

    [[nodiscard]] const std::string& getRawSentenceText() const;

    [[nodiscard]] size_t getSentenceOffsetInDocument() const;

  private:
    /**
     * @brief Appends JSON object to results array
     * @details Adds object to jsonData array for later persistence
     *
     * @param newObj JSON object to append
     */
    void addJsonObject(const nlohmann::ordered_json& newObj);

  private:
    /// @brief Document identifier
    std::string docId;

    /// @brief Current sentence number (for sentence-level tracking)
    size_t sentNum;

    /// @brief Global offset of current sentence in document (for span calculation)
    /// @details Needed to convert sentence-local positions to document-global positions
    size_t sentenceOffsetInDocument = 0;

    /// @brief Raw sentence text (for span calculation and validation)
    std::string rawSentenceText;

    /// @brief Output file path for JSON results
    std::filesystem::path outputFile;

    /// @brief JSON array accumulating phrase extraction results
    nlohmann::ordered_json jsonData;

    /// @brief Token positions in raw text: pairs of (charStart, charEnd)
    /// @details Parallel to tokens in analyzed sentence (excluding separators)
    std::vector<std::pair<size_t, size_t>> tokenPositions;
};

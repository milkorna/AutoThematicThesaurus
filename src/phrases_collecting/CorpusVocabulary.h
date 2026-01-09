#pragma once

#include <Eigen/Dense>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <fasttext.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

using namespace Eigen;
using VectorPtr = std::shared_ptr<fasttext::Vector>;

/**
 * @class CorpusVocabulary
 * @brief Manages corpus-wide vocabulary statistics and global lemma frequencies.
 * @details Maintains global word frequency distribution and document frequency tracking
 * across all documents in a corpus. Handles serialization/deserialization to JSON format
 * and vocabulary filtering (stop words removal).
 */
class CorpusVocabulary {
  public:
    /**
     * @brief Default constructor.
     */
    CorpusVocabulary() = default;

    /**
     * @brief Provides singleton access to global corpus vocabulary instance.
     *
     * @return Reference to the static CorpusVocabulary instance
     */
    static CorpusVocabulary& GetCorpus() {
        static CorpusVocabulary corpus;
        return corpus;
    }

    // ════════════════════════════════════════════════════════════
    // Serialization and Storage
    // ════════════════════════════════════════════════════════════

    /**
     * @brief Saves global vocabulary statistics to JSON file
     * @details Serializes:
     * - corpus_metadata (total_documents, total_words)
     * - global_statistics (word_frequency, document_frequency)
     *
     * JSON Structure:
     * {
     *   "corpus_metadata": {
     *     "total_documents": 150,
     *     "total_words": 125000
     *   },
     *   "global_statistics": {
     *     "word_frequency": { "lemma": count, ... },
     *     "document_frequency": { "lemma": doc_count, ... }
     *   }
     * }
     *
     * @param filename Output file path
     * @throws std::runtime_error If file cannot be opened or written
     */
    void save(const std::string& filename);

    /**
     * @brief Loads global vocabulary statistics from JSON file
     * @details Deserializes word_frequency and document_frequency maps from JSON.
     * Clears existing vocabulary before loading to ensure clean state.
     *
     * @param filename Path to JSON file
     * @throws std::runtime_error If file cannot be opened or JSON is invalid
     */
    void load(const std::string& filename);

    // ════════════════════════════════════════════════════════════
    // Data Access
    // ════════════════════════════════════════════════════════════

    /**
     * @brief Provides read-only access to word frequency distribution.
     * @details Maps each lemma to its occurrence count in corpus.
     *
     * @return Const reference to word frequency map (lemma -> count)
     */
    const std::unordered_map<std::string, size_t>& getWordFrequencies() const;

    /**
     * @brief Provides read-only access to document frequency distribution.
     * @details Maps each lemma to number of documents containing it.
     *
     * @return Const reference to document frequency map (lemma -> doc_count)
     */
    const std::unordered_map<std::string, size_t>& getDocumentFrequencies() const;

    /**
     * @brief Retrieves frequency count for specified lemma.
     * @details Returns how many times given word appears in entire corpus.
     *
     * @param lemma The word to query
     * @return Frequency count; 0 if lemma not present
     */
    size_t getWordFrequency(const std::string& lemma) const;

    /**
     * @brief Retrieves document frequency for specified lemma.
     * @details Returns in how many documents given word appears.
     *
     * @param lemma The word to query
     * @return Number of documents containing lemma; 0 if not found
     */
    size_t getDocumentFrequency(const std::string& lemma) const;

    /**
     * @brief Returns unique document count in corpus.
     *
     * @return Count of unique documents
     */
    size_t getDocumentCount() const;

    /**
     * @brief Returns total word token count in corpus.
     * @details Cumulative count from all word frequency entries.
     *
     * @return Total word occurrences
     */
    size_t getWordCount() const;

    // ════════════════════════════════════════════════════════════
    // Data Update
    // ════════════════════════════════════════════════════════════

    /**
     * @brief Updates word frequency counter for specified lemma.
     * @details Increments both lemma frequency entry and total word counter.
     * Called for each lemma occurrence during document processing.
     *
     * @param lemma The word to increment frequency for
     */
    void updateWordFrequency(const std::string& lemma);

    /**
     * @brief Updates document frequency counter for specified lemma.
     * @details Increments document count for given word.
     * Called once per unique lemma per document during finalization.
     *
     * @param lemma The word to increment document frequency for
     */
    void updateDocumentFrequency(const std::string& lemma);

    /**
     * @brief Increments total document count.
     * @details Should be called when starting to process a new document.
     */
    void incrementDocumentCount();

    // ════════════════════════════════════════════════════════════
    // Vocabulary Filtering
    // ════════════════════════════════════════════════════════════

    /**
     * @brief Filters stop words from vocabulary
     * @details Removes stop words from word_frequency and document_frequency maps.
     * Recalculates word count after removal. Uses StringUtils for stop word detection.
     */
    void filter();

  private:
    /**
     * @brief Total word token occurrences in corpus.
     */
    size_t wordCount = 0;

    /**
     * @brief Total unique document count in corpus.
     */
    size_t documentCount = 0;

    /**
     * @brief Lemma frequency distribution.
     * @details Maps each word to its occurrence count in corpus.
     * Updated by updateWordFrequency() method.
     */
    std::unordered_map<std::string, size_t> wordFrequency;

    /**
     * @brief Document frequency distribution.
     * @details Maps each word to count of documents containing it.
     * Updated by updateDocumentFrequency() method.
     */
    std::unordered_map<std::string, size_t> documentFrequency;

    // ════════════════════════════════════════════════════════════
    // Serialization Internals
    // ════════════════════════════════════════════════════════════

    /**
     * @brief Clears all corpus data and resets counters to initial state.
     */
    void clear();

    /**
     * @brief Converts vocabulary to JSON representation
     * @details Serializes corpus_metadata and global_statistics.
     *
     * @return JSON object with corpus data
     */
    [[nodiscard]] nlohmann::ordered_json serialize();

    /**
     * @brief Populates vocabulary from JSON representation
     * @details Deserializes all corpus components without any filtering.
     * Clears existing corpus data before loading.
     *
     * @param j JSON object containing corpus data
     * @throws std::runtime_error If JSON structure is invalid or missing required fields
     */
    void deserialize(const nlohmann::ordered_json& j);
};

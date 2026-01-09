#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

/**
 * @brief Represents a document record parsed from JSON source
 * @details Contains document metadata (ID, title) and text content.
 * Provides utility method to combine fields for processing.
 */
class Document {
    friend class RawDataLoader;

  public:
    [[nodiscard]] std::string getDocId() const;
    [[nodiscard]] std::string getFilename() const;
    [[nodiscard]] std::string getProcessingTimestamp() const;
    [[nodiscard]] std::string getTitle() const;

    /**
     * @brief Combines title and text fields for processing
     * @details Returns merged text with configurable inclusion of title.
     * If both title and text are present, they are joined with newline separator.
     * If only one field is present, returns that field.
     * If both are empty, returns empty string.
     *
     * @param mergeWithTitle If true, includes title in output;
     *                       if false, returns text only (default: true)
     * @return Combined text ready for processing
     */
    [[nodiscard]] std::string getText(bool mergeWithTitle = true) const;

    [[nodiscard]] size_t getSentenceCount() const;
    [[nodiscard]] size_t getWordCount() const;
    [[nodiscard]] size_t getUniqueLemmasCount() const;
    [[nodiscard]] size_t getCharacterCount() const;
    [[nodiscard]] const std::unordered_map<std::string, size_t>& getWordFrequency() const;
    [[nodiscard]] const std::unordered_set<std::string>& getUniqueLemmas() const;

    void incrementSentenceCount(size_t count = 1);
    void incrementWordCount(size_t count = 1);
    void incrementWordFrequency(const std::string& lemma, size_t count = 1);
    void addUniqueLemma(const std::string& lemma);
    void addUniqueLemmasFromSentence(const std::unordered_set<std::string>& lemmas);
    void setCharacterCount(size_t count);

  private:
    /// @brief Unique document identifier
    std::string doc_id;

    std::string filename;

    std::string processing_timestamp;

    /// @brief Document title
    std::string title;

    /// @brief Document body text
    std::string text;

    struct DocumentStats {
        size_t sentence_count = 0;
        size_t word_count = 0;
        size_t unique_lemmas = 0;
        size_t character_count = 0;
    } stats;

    // Локальная статистика документа (используется при сохранении)
    std::unordered_map<std::string, size_t> word_frequency_local;
    std::unordered_set<std::string> document_lemmas; // для подсчета unique_lemmas
};

inline size_t countUTF8Characters(const std::string& str) {
    size_t count = 0;
    for (unsigned char c : str) {
        if ((c & 0xC0) != 0x80) {
            count++;
        }
    }
    return count;
}
#pragma once

#include <nlohmann/json.hpp>

#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

/**
 * @struct TokenizedSentence
 * @brief Represents a single sentence with original and normalized forms.
 * @details Stores document/sentence identifiers and both raw and processed text versions.
 */
struct TokenizedSentence {
    /// @brief Document identifier for sentence location.
    std::string docId;

    /// @brief Sentence index within document.
    size_t sentNum;

    /// @brief Original sentence text as extracted from source.
    std::string originalStr;

    /// @brief Normalized sentence with lemmatized words.
    std::string normalizedStr;
};

using SentenceMap = std::unordered_map<std::string, std::unordered_map<size_t, TokenizedSentence>>;

/**
 * @class SentenceCorpus
 * @brief Manages storage and serialization of sentence data.
 * @details Maintains corpus of sentences with both original and normalized forms.
 */
class SentenceCorpus {
  public:
    /**
     * @brief Default constructor.
     */
    SentenceCorpus() = default;

    /**
     * @brief Provides singleton access to global sentence corpus instance.
     */
    static SentenceCorpus& GetCorpus() {
        static SentenceCorpus corpus;
        return corpus;
    }

    /**
     * @brief Saves corpus to persistent storage.
     * @details Serializes all sentences and metadata to JSON file with formatted indentation.
     *
     * @param filename Path to output JSON file
     */
    void save(const std::string& filename);

    /**
     * @brief Loads corpus from persistent storage.
     * @details Reads JSON file and deserializes into corpus with automatic filtering
     *          of short sentences (normalized string length < 50 characters).
     *
     * @param filename Path to input JSON file
     */
    void load(const std::string& filename);

    /**
     * @brief Clears all corpus data and resets counters to initial state.
     */
    void clear();

    /**
     * @brief Retrieves sentence by document and sentence indices.
     *
     * @param docId Document identifier
     * @param sentNum Sentence index within document
     * @return Pointer to TokenizedSentence if found; nullptr otherwise
     *
     * @note Returns pointer to internal data; do not modify or store long-term
     */
    [[nodiscard]] const std::optional<TokenizedSentence> getSentence(const std::string& docId,
                                                                     const size_t sentNum) const;

    /**
     * @brief Returns total sentence count in corpus.
     *
     * @return Number of sentences currently stored
     */
    [[nodiscard]] size_t getSentencesCount() const;

    /**
     * @brief Returns read-only map of sentences.
     *
     * @return Map of sentences
     */
    [[nodiscard]] const SentenceMap& getSentenceMap() const {
        return sentenceMap;
    }

    /**
     * @brief Adds sentence to corpus under specified document.
     * @details Stores both original and normalized versions with document/sentence indices.
     *
     * @param docId Document identifier
     * @param sentNum Sentence index within document
     * @param originalStr Original sentence text
     * @param normalizedStr Lemmatized sentence text
     *
     * @note Increments total sentence counter
     * @note Package-private for use by build() method
     */
    void addSentence(const std::string& docId, const size_t sentNum, const std::string& data,
                     const std::string& normalizedData);

  private:
    /**
     * @brief Nested map structure for fast sentence retrieval.
     * @details Maps (docId -> (sentNum -> TokenizedSentence)).
     */
    SentenceMap sentenceMap;

    /**
     * @brief Total sentence count in corpus.
     */
    size_t sentencesCount = 0;

    /**
     * @brief Serializes corpus to JSON format.
     * @details Converts all sentences and metadata to JSON representation.
     *
     * @return JSON object containing corpus data
     */
    nlohmann::ordered_json serialize() const;

    /**
     * @brief Deserializes corpus from JSON format.
     * @details Loads sentences from JSON with automatic filtering of short entries
     *          (normalized string length < 50 characters).
     *
     * @param j JSON object with corpus data structure
     */
    void deserialize(const nlohmann::ordered_json& j);
};

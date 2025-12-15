#pragma once

#include "boost/algorithm/string.hpp"
#include "nlohmann/json.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <fasttext.h>
#include <string>
#include <unordered_map>

using json = nlohmann::json;
using namespace Eigen;
using VectorPtr = std::shared_ptr<fasttext::Vector>;

// Forward declarations
class TextCorpusLoader;
class TextCorpusFilter;

/**
 * @class TextCorpus
 * @brief Manages processing and storage of text corpus with document-based organization.
 * @details Handles text loading, word frequency tracking, and statistical metric calculation.
 *          Implements singleton pattern for global corpus access throughout application.
 */
class TextCorpus {
  public:
    /**
     * @brief Default constructor.
     */
    TextCorpus() = default;

    /**
     * @brief Provides singleton access to global corpus instance.
     *
     * @return Reference to the static TextCorpus instance
     */
    static TextCorpus& GetCorpus() {
        static TextCorpus corpus;
        return corpus;
    }

    /**
     * @brief Provides read-only access to word frequency distribution.
     * @details Maps each lemma to its occurrence count in corpus.
     *
     * @return Const reference to word frequency map (lemma -> count)
     */
    const std::unordered_map<std::string, int>& getWordFrequencies() const;

    /**
     * @brief Provides read-only access to document frequency distribution.
     * @details Maps each lemma to number of documents containing it.
     *
     * @return Const reference to document frequency map (lemma -> doc_count)
     */
    const std::unordered_map<std::string, int>& getDocumentFrequencies() const;

    /**
     * @brief Provides read-only access to texts storage.
     * @details Maps document titles to their associated text paragraphs.
     *
     * @return Const reference to texts map (document_title -> text_list)
     */
    const std::unordered_map<std::string, std::vector<std::string>>& getTexts() const;

    /**
     * @brief Retrieves frequency count for specified lemma.
     * @details Returns how many times given word appears in corpus.
     *
     * @param lemma The word to query

     * @return Frequency count; 0 if lemma not present
     */
    int getWordFrequency(const std::string& lemma) const;

    /**
     * @brief Retrieves document frequency for specified lemma.
     * @details Returns in how many documents given word appears.
     *
     * @param word The word to query
     * @return Number of documents containing word; 0 if not found
     *
     */
    int getDocumentFrequency(const std::string& word) const;

    /**
     * @brief Returns unique document count in corpus.
     * @details Each document corresponds to one source file.
     *
     * @return Count of unique documents
     */
    int getDocumentCount() const;

    /**
     * @brief Returns text paragraph count in corpus.
     * @details Represents cumulative paragraphs across all documents.
     *
     * @return Count of all text entries
     */
    int getTextCount() const;

    /**
     * @brief Returns word token count in corpus.
     * @details Cumulative count from all word frequency entries.
     *
     * @return Total word occurrences
     */
    int getWordCount() const;

    /**
     * @brief Adds text paragraph to corpus under specified document.
     * @details Extracts document title from filename, creates document entry if needed.
     *          Updates document count on first occurrence of new document.
     *
     * @param filename Document name
     * @param text Text paragraph content to add
     */
    void addText(const std::string& filename, const std::string& text);

    /**
     * @brief Extracts document title from corresponding file.
     * @details Implements naming convention: replaces "_text.txt" with "_title.txt"
     *          and reads first line of title file.
     *
     * @param filename Document name
     * @return Document title read from corresponding title file
     */
    std::string ExtractTitleFromFilename(const std::string& filename) const;

    /**
     * @brief Updates word frequency counter for specified lemma.
     * @details Increments both lemma frequency entry and total word counter.
     *
     * @param lemma The word to increment frequency for
     */
    void UpdateWordFrequency(const std::string& lemma);

    /**
     * @brief Updates document frequency counter for specified lemma.
     * @details Increments document count for given word.
     *
     * @param lemma The word to increment document frequency for
     */
    void UpdateDocumentFrequency(const std::string& lemma);

    /**
     * @brief Loads text paragraphs from file into corpus.
     * @details Reads file line-by-line; each line becomes separate text entry.
     *          Uses AddText internally to maintain consistency.
     *
     * @param filename Path to input text file
     */
    void LoadTextsFromFile(const std::string& filename);

    /**
     * @brief Calculates Term Frequency metric for specified lemma.
     * @details Quantifies word importance relative to corpus size.
     *          Formula: TF(word) = frequency(word) / total_words
     *
     * @param lemma The word to calculate TF for
     * @return TF value
     *
     * @note Represents local frequency within corpus context
     */
    double CalculateTF(const std::string& lemma) const;

    /**
     * @brief Calculates Inverse Document Frequency metric for specified lemma.
     * @details Quantifies word distinctiveness across documents.
     *          Formula: IDF(word) = log(total_documents / (1 + doc_frequency))
     *
     * @param lemma The word to calculate IDF for
     * @return IDF value; 0.0 if word absent
     *
     * @note Prevents common words from dominating TF-IDF calculation
     */
    double CalculateIDF(const std::string& lemma) const;

    /**
     * @brief Calculates combined TF-IDF metric for specified lemma.
     * @details Product of term frequency and inverse document frequency.
     *          Formula: TF-IDF(word) = TF(word) * IDF(word)
     *
     * @param lemma The word to calculate TF-IDF for
     * @return TF-IDF metric value; 0.0 if word absent
     *
     * @note Standard metric for information retrieval and text analysis
     */
    double CalculateTFIDF(const std::string& lemma) const;

  private:
    /**
     * @brief Total word token occurrences in corpus.
     */
    int wordCount = 0;

    /**
     * @brief Total text paragraph count in corpus.
     */
    int textCount = 0;

    /**
     * @brief Total unique document count in corpus.
     */
    int documentCount = 0;

    /**
     * @brief Storage for document texts.
     * @details Maps document title to vector of text paragraphs.
     *          Primary data structure for corpus content.
     */
    std::unordered_map<std::string, std::vector<std::string>> texts;

    /**
     * @brief Lemma frequency distribution.
     * @details Maps each word to its occurrence count in corpus.
     *          Updated by UpdateWordFrequency method.
     */
    std::unordered_map<std::string, int> wordFrequency;

    /**
     * @brief Document frequency distribution.
     * @details Maps each word to count of documents containing it.
     *          Updated by UpdateDocumentFrequency method.
     */
    std::unordered_map<std::string, int> documentFrequency;

    friend class TextCorpusLoader;
    friend class TextCorpusFilter;

    /**
     * @brief Provides modifiable access to texts container.
     */
    std::unordered_map<std::string, std::vector<std::string>>& getTextsForModification();

    /**
     * @brief Provides modifiable access to word frequency container.
     */
    std::unordered_map<std::string, int>& getWordFrequenciesForModification();

    /**
     * @brief Provides modifiable access to document frequency container.
     */
    std::unordered_map<std::string, int>& getDocumentFrequenciesForModification();

    /**
     * @brief Clears all corpus data and resets counters to initial state.
     */
    void clearAllData();

    /**
     * @brief Recalculates corpus statistics from current container contents.
     * @details Recomputes total word and text counts based on actual data.
     *          Should be called after any modification affecting corpus structure.
     */
    void recalculateStatistics();
};
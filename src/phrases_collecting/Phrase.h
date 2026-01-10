#pragma once

#include "xmorphy/morph/WordForm.h"

#include <deque>
#include <memory>
#include <string>

/**
 * @brief Represents the position of a word or phrase within a document
 */
struct Position {
    size_t start = 0;     ///< Start token index
    size_t end = 0;       ///< End token index
    size_t charStart = 0; ///< Start character position in sentence
    size_t charEnd = 0;   ///< End character position in sentence
    size_t sentNum = 0;   ///< Sentence number
    std::string docId;    ///< Document ID
};

class Process;
class Phrase;

using PhrasePtr = std::shared_ptr<Phrase>;

/**
 * @brief Represents a complex word or phrase with text form, position, and model information
 * @details Stores word forms, lemmas, text representation, and metadata about the phrase.
 * Provides methods for phrase construction and merging.
 */
class Phrase {
  public:
    /// @brief Deque of word forms (morphological data)
    std::deque<X::WordFormPtr> words;

    /// @brief Deque of lemmas corresponding to words
    std::deque<std::string> lemmas;

    /// @brief Text representation of the phrase
    std::string textForm;

    /// @brief Position metadata (start, end, sentence, document)
    Position pos;

    /// @brief Name of grammar model this phrase matches
    std::string modelName;

    /**
     * @brief Creates a new Phrase by copying from a base phrase
     * @details Shallow copy of words and lemmas, assigns new model name.
     * Useful when expanding a simple phrase with a grammar model.
     *
     * @param basePhrase Source phrase to copy from
     * @param modelName Model name to assign
     * @return Shared pointer to new Phrase instance
     */
    static PhrasePtr createFromPhrase(const PhrasePtr& basePhrase, const std::string& modelName);

    /**
     * @brief Creates a new Phrase from a single token
     * @details Initializes phrase with one word form and corresponding lemma.
     * Position and document metadata from Process.
     *
     * @param tokenIndex Token index in sentence
     * @param token Word form (morphological data)
     * @param modelName Model name to assign
     * @param process Process context (provides document ID, sentence number)
     * @return Shared pointer to new Phrase instance
     */
    static PhrasePtr createFromToken(const size_t tokenInd, const X::WordFormPtr token, const std::string modelName,
                                     const Process& process);

    /**
     * @brief Adds a word to the beginning of the phrase
     * @details Updates words, lemmas, position, and text form.
     *
     * @param form Word form to prepend
     */
    void addWordToLeft(const X::WordFormPtr& wordForm);

    /**
     * @brief Adds a word to the end of the phrase
     * @details Updates words, lemmas, position, and text form.
     *
     * @param form Word form to append
     */
    void addWordToRight(const X::WordFormPtr& wordForm);

    /**
     * @brief Merges another phrase to the left
     * @details Prepends all words and lemmas in reverse order
     * (to preserve word order). Text form NOT updated.
     *
     * @param otherPhrase Phrase to merge from left
     */
    void mergeLeft(const PhrasePtr& otherPhrase);

    /**
     * @brief Merges another phrase to the right
     * @details Appends all words and lemmas. Text form NOT updated.
     *
     * @param otherPhrase Phrase to merge from right
     */
    void mergeRight(const PhrasePtr& otherPhrase);
};

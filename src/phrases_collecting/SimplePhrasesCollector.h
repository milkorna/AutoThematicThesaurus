#pragma once

#include "GrammarPatternManager.h"
#include "ModelComponent.h"
#include "PhraseValidator.h"
#include "Process.h"

#include "xmorphy/morph/WordForm.h"

/**
 * @brief Collects simple phrases by matching grammar patterns against word sequences
 * @details Identifies phrases that match predefined grammar models by checking word form conditions
 * and component relationships. Simple phrases serve as building blocks for complex phrases.
 */
class SimplePhrasesCollector {
  public:
    /**
     * @brief Constructs a SimplePhrasesCollector with word forms from a sentence
     * @details Initializes the collector with morphological data for phrase pattern matching
     *
     * @param forms Vector of word forms with morphological information from the sentence
     */
    explicit SimplePhrasesCollector(const std::vector<WordFormPtr>& forms)
        : m_sentence(forms), m_collection{}, m_validator(forms) {
    }

    /**
     * @brief Retrieves the collection of identified phrases
     * @return Reference to the vector of collected PhrasePtr
     */
    [[nodiscard]] const std::vector<PhrasePtr>& getCollection() const;

    /**
     * @brief Collects simple phrases from the sentence
     * @details Processes each token, checking against grammar patterns and building phrases
     * by recursively extending left and right of matched heads. Outputs results to JSON.
     *
     * @param process Context object containing document ID, output file path, and processing state
     */
    void collect(Process& process);

    /// @brief Default destructor
    ~SimplePhrasesCollector() = default;

  private:
    /**
     * @brief Expands phrase in specified direction (left or right)
     * @details Recursively checks components and extends phrase.
     * Returns true if phrase was completely matched and added to m_collection.
     *
     * @param phrase Phrase being expanded (modified in place)
     * @param model Grammar model with expected component sequence
     * @param componentIndex Current component index
     * @param tokenIndex Current token index in sentence
     * @param matchedComponents Counter of matched components (incremented in place)
     * @param expandLeft Direction flag (true = leftward, false = rightward)
     * @return true if phrase completed and collected, false if match failed
     */
    [[nodiscard]] bool expandPhraseInDirection(const std::shared_ptr<Phrase>& wc, const std::shared_ptr<Model>& model,
                                               size_t compIndex, size_t tokenInd, size_t& correct, const bool isLeft);

    /**
     * @brief Matches and expands phrase for specific model
     * @param model Grammar model to match
     * @param tokenIndex Head token index
     * @param token Head word form
     * @param process Context for phrase creation
     * @return true if phrase was successfully collected
     */
    [[nodiscard]] bool tryExpandPhraseWithModel(Process& process, const std::shared_ptr<Model>& model,
                                                size_t tokenIndex, const X::WordFormPtr& token);

    /**
     * @brief Checks if token can be a valid phrase head
     * @param token Word form to check
     * @return true if token is valid and has head speech parts
     */
    [[nodiscard]] bool isValidPhraseHead(const X::WordFormPtr& token) const;

  private:
    /// @brief Collection of identified simple phrases
    std::vector<PhrasePtr> m_collection;

    /// @brief Word forms in the current sentence (reference, not owned)
    const std::vector<X::WordFormPtr>& m_sentence;

    /// @brief Morphological validator for tokens
    PhraseValidator m_validator;
};
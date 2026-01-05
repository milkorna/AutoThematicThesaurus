#pragma once

#include "xmorphy/morph/WordForm.h"

#include "GrammarPatternManager.h"
#include "ModelComponent.h"
#include "PhrasesCollectorUtils.h"

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
    explicit SimplePhrasesCollector(const std::vector<WordFormPtr>& forms) : m_sentence(forms), m_collection{} {
    }

    /**
     * @brief Retrieves the collection of identified word complexes
     * @details Returns the accumulated simple phrases found during collection
     *
     * @return Reference to the vector of collected WordComplexPtr
     */
    [[nodiscard]] std::vector<WordComplexPtr>& getCollection() {
        return m_collection;
    }

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
    /// @brief Collection of identified simple phrase complexes
    std::vector<WordComplexPtr> m_collection;

    /// @brief Word forms representing the current sentence
    std::vector<WordFormPtr> m_sentence;

    /**
     * @brief Recursively extends phrase in specified direction
     * @details Checks if word form at given position matches component requirements.
     * Applies filters (stop words, punctuation, morphology validation).
     * Recursively processes adjacent components until model boundary or no match.
     *
     * @param wc Word complex being built (extended in place)
     * @param model Grammar model defining expected component sequence
     * @param compIndex Current component index in the model
     * @param tokenInd Current word token index in the sentence
     * @param correct Reference counter for matched components (incremented in place)
     * @param isLeft Direction flag (true for extending left, false for extending right)
     * @return true if all remaining model components matched and phrase was added to collection,
     *         false if match failed or component is non-recursive
     */
    [[nodiscard]] bool checkAside(const std::shared_ptr<WordComplex>& wc, const std::shared_ptr<Model>& model,
                                  size_t compIndex, size_t tokenInd, size_t& correct, const bool isLeft);
};
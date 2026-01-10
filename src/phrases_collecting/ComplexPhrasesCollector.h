#pragma once

#include "GrammarPatternManager.h"
#include "ModelComponent.h"
#include "PhraseMatchStatus.h"
#include "PhraseValidator.h"
#include "Process.h"

/**
 * @brief Collects complex phrases by extending simple phrases with adjacent words
 * @details Uses GrammarPatternManager to identify and collect complex phrases based on grammar patterns.
 * Processes simple phrases by checking adjacent word components (left and right) against model conditions.
 */
class ComplexPhrasesCollector {
  public:
    /**
     * @brief Constructs a ComplexPhrasesCollector with simple phrases and word forms
     * @details Initializes the collector with pre-identified simple phrases and the current sentence context.
     * Creates validator and extender helper objects for morphological validation and phrase expansion.
     *
     * @param simplePhrases Vector of simple PhrasePtr to extend with adjacent words
     * @param forms Vector of word forms (morphological data) from the sentence being analyzed
     */
    explicit ComplexPhrasesCollector(const std::vector<PhrasePtr>& simplePhrases,
                                     const std::vector<X::WordFormPtr>& forms)
        : m_simplePhrases(simplePhrases), m_sentence(forms) {
    }

    /**
     * @brief Collects complex phrases from the sentence
     * @details Main entry point for complex phrase collection. For each simple phrase in the sentence,
     * iterates through all available grammar models and attempts to match them. When a model matches,
     * collects the resulting complex phrase and moves to the next simple phrase.
     * Finally validates phrase boundaries if the option is enabled to remove nested phrases.
     *
     * @param process Context object containing document ID, output file path, and processing state
     */
    void collect(Process& process);

    /// @brief Default destructor
    ~ComplexPhrasesCollector() = default;

  private:
    /// @brief Immutable vector of simple phrases to extend
    const std::vector<PhrasePtr>& m_simplePhrases;

    /// @brief Immutable reference to word forms in the current sentence
    const std::vector<X::WordFormPtr>& m_sentence;

    /// @brief Collection of identified complex phrases
    std::vector<PhrasePtr> m_collection;

    /**
     * @brief Validates that a simple phrase matches the current model component
     * @details Performs three-stage validation:
     * 1. Checks morphological compatibility using validateWordComponents()
     * 2. Validates that head is properly matched (if head was validated, it must match)
     * 3. Ensures all additional model conditions are satisfied (must be empty)
     *
     * @param simplePhrase Simple phrase to validate against the component
     * @param modelComp Model component to check compatibility with
     * @param status Phrase match status to update with morphological validation results
     * @return true if the simple phrase satisfies all component requirements, false otherwise
     */
    bool isSimplePhraseMatchesComponent(const PhrasePtr& simplePhrase, const std::shared_ptr<ModelComp>& modelComp,
                                        PhraseMatchStatus& status) const;

    /**
     * @brief Initializes a phrase and attempts to extend it in both directions around a component
     * @details Executes a two-step expansion process:
     * Step 1: Initialize Phrase from the simple phrase
     * Step 2: Attempt left extension (if component index > 0 and position allows)
     * Step 3: Attempt right extension (if component index < model size - 1 and position allows)
     *
     * Each extension uses the checkComponent() dispatcher to handle both WordComp and ModelComp types.
     * This allows seamless processing of mixed component sequences like (WordComp, ModelComp, WordComp).
     *
     * @param model Grammar model containing component definitions and structure
     * @param simplePhrase Simple phrase to initialize and expand from
     * @param simplePhraseIndex Index of the simple phrase in m_simplePhrases collection
     * @param componentIndex Position of the current component in the model (entry point for expansion)
     * @param status Phrase match status for tracking validation and matching progress
     * @return true if successful expansion finds complete pattern match, false otherwise
     */
    bool expandPhraseAroundComponent(const std::shared_ptr<Model>& model, const PhrasePtr& simplePhrase,
                                     size_t simplePhraseIndex, size_t componentIndex, PhraseMatchStatus& status);

    /**
     * @brief Orchestrates the complete matching process for a single model against a phrase
     * @details Implements a two-phase matching strategy:
     *
     * Phase 1 - Component Compatibility Check:
     *   - Finds the model component corresponding to the simple phrase
     *   - Validates morphological compatibility (isSimplePhraseMatchesComponent)
     *   - Returns false immediately if validation fails
     *
     * Phase 2 - Phrase Expansion:
     *   - Initializes Phrase from the simple phrase
     *   - Attempts bidirectional expansion around the component (expandPhraseAroundComponent)
     *   - Recursively processes adjacent components through the dispatcher
     *
     * @param model Grammar model to apply against the phrase
     * @param simplePhrase Simple phrase to process
     * @param simplePhraseIndex Index of the simple phrase in m_simplePhrases
     * @return true if model successfully matched and phrase was collected, false otherwise
     */
    bool processModelForPhrase(const std::shared_ptr<Model>& model, const PhrasePtr& simplePhrase,
                               size_t simplePhraseIndex);

    /**
     * @brief Removes nested phrases from the final collection
     * @details Post-processing step that filters out complex phrases completely contained within others.
     * A phrase is marked as nested if there exists another phrase with:
     *   - Same start position
     *   - End position greater than or equal to the current phrase's end position
     *
     * Only called when Options::validateBoundaries is enabled. Ensures only maximal
     * non-overlapping phrases are included in the final result.
     */
    void validateBoundaries();
};
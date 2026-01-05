#pragma once

#include "GrammarPatternManager.h"
#include "ModelComponent.h"

#include "Process.h"

/**
 * @brief Status tracking for phrase components during grammar pattern matching
 * @details Maintains state information for the current phrase being processed,
 * including matched component count and various validation flags.
 */
struct CurrentPhraseStatus {
    /// @brief Number of successfully matched phrase components
    size_t correct = 0;

    /// @brief Flag indicating phrase head has been matched against pattern
    bool headIsMatched = false;

    /// @brief Flag indicating phrase head validation has been performed
    bool headIsChecked = false;

    /// @brief Flag indicating a lexical item was located during processing
    bool foundLex = false;
};

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
     *
     * @param simplePhrases Vector of simple WordComplexPtr to extend with adjacent words
     * @param forms Vector of word forms (morphological data) from the sentence being analyzed
     */
    explicit ComplexPhrasesCollector(const std::vector<WordComplexPtr>& simplePhrases,
                                     const std::vector<X::WordFormPtr>& forms)
        : m_simplePhrases(simplePhrases), m_sentence(forms), m_collection{} {
    }

    /**
     * @brief Collects complex phrases from the sentence
     * @details Processes simple phrases using grammar patterns and outputs results.
     * Validates phrase boundaries if option is enabled.
     *
     * @param process Context object containing document ID, output file path, and processing state
     */
    void collect(Process& process);

    /**
     * @brief Removes nested phrases from the collection
     * @details Filters out complex phrases that are completely contained within other phrases.
     * A phrase is considered nested if its start position matches another phrase's start
     * and its end position is within or equal to that phrase's end position.
     */
    void validateBoundaries();

    /// @brief Default destructor
    ~ComplexPhrasesCollector() = default;

  private:
    /// @brief Immutable vector of simple phrases to extend
    const std::vector<WordComplexPtr> m_simplePhrases;

    /// @brief Collection of identified complex phrases
    std::vector<WordComplexPtr> m_collection;

    /// @brief Word forms representing the current sentence
    std::vector<WordFormPtr> m_sentence;

    /**
     * @brief Validates morphological tags against pattern conditions
     * @details Checks if any morphological form matches the condition requirements.
     *
     * @param morphForms Set of morphological information from a word form
     * @param cond Condition to check against morphological data
     * @param curPhrStatus Current phrase validation status (updated in place)
     * @return true if a matching morphological form is found, false otherwise
     */
    [[nodiscard]] bool checkMorphologicalTags(const std::unordered_set<X::MorphInfo>& morphForms, const Condition& cond,
                                              CurrentPhraseStatus& curPhrStatus);

    /**
     * @brief Validates word components of a model component
     * @details Checks if components within a model component match morphological conditions.
     *
     * @param curSimplePhr Current simple phrase being processed
     * @param curModelComp Model component containing word components to validate
     * @param curPhrStatus Current phrase validation status (updated in place)
     * @return true if validation succeeds, false otherwise
     */
    [[nodiscard]] bool checkWordComponents(const WordComplexPtr& curSimplePhr,
                                           const std::shared_ptr<ModelComp>& curModelComp,
                                           CurrentPhraseStatus& curPhrStatus);

    /**
     * @brief Checks if a simple phrase satisfies current model component requirements
     * @details Validates morphological tags and additional conditions for the simple phrase.
     *
     * @param curSimplePhr Simple phrase to validate
     * @param curModelComp Model component with conditions to check
     * @param curPhrStatus Current phrase validation status (updated in place)
     * @return true if phrase passes validation, false otherwise
     */
    [[nodiscard]] bool checkCurrentSimplePhrase(const WordComplexPtr& curSimplePhr,
                                                const std::shared_ptr<ModelComp>& curModelComp,
                                                CurrentPhraseStatus& curPhrStatus);

    /**
     * @brief Determines if a simple phrase should be skipped during processing
     * @details Checks boundary conditions, position validity, and model compatibility.
     *
     * @param smpPhrOffset Index of the simple phrase to check
     * @param curSimplePhrInd Index of the current simple phrase being processed
     * @param isLeft Direction flag (true for left/preceding, false for right/following)
     * @param wc Current word complex being built
     * @param modelComp Model component being processed
     * @return true if phrase should be skipped, false otherwise
     */
    [[nodiscard]] bool shouldSkip(size_t smpPhrOffset, size_t curSimplePhrInd, bool isLeft, const WordComplexPtr& wc,
                                  std::shared_ptr<ModelComp> modelComp);

    /**
     * @brief Recursively extends complex phrase with adjacent word forms
     * @details Checks components in specified direction (left/right) and accumulates matching words.
     * Handles both WordComp and ModelComp component types. Recursively processes nested components
     * until no more matches are found or boundaries are reached.
     *
     * @param curSPhPosCmp Index of current component in the model
     * @param wc Current word complex being extended (modified in place)
     * @param model Grammar model defining expected component structure
     * @param compIndex Current component index in the model
     * @param formIndex Current word form index in the sentence
     * @param isLeft Direction flag (true for left/preceding words, false for right/following)
     * @param curPhrStatus Current phrase validation status (updated in place)
     * @param curSimplePhrInd Index of the current simple phrase
     * @return true if complete pattern match is found, false otherwise
     */
    [[nodiscard]] bool checkAside(size_t curSPhPosCmp, const WordComplexPtr& wc, const std::shared_ptr<Model>& model,
                                  size_t compIndex, size_t formIndex, const bool isLeft,
                                  CurrentPhraseStatus& curPhrStatus, size_t curSimplePhrInd);

    /**
     * @brief Processes a model component for a simple phrase
     * @details Initializes word complex from simple phrase and recursively checks adjacent components.
     * Handles both left and right directions if component index allows.
     *
     * @param model Grammar model containing the component structure
     * @param curSimplePhr Current simple phrase to process
     * @param curSimplePhrInd Index of the current simple phrase
     * @param curPhrStatus Current phrase validation status (updated in place)
     * @param wc Output word complex to initialize (modified in place)
     * @return true if complete pattern match is found, false otherwise
     */
    [[nodiscard]] bool processModelComponent(const std::shared_ptr<Model>& model, const WordComplexPtr& curSimplePhr,
                                             const size_t curSimplePhrInd, CurrentPhraseStatus& curPhrStatus,
                                             WordComplexPtr& wc);

    /**
     * @brief Updates phrase boundaries and text form with adjacent phrase
     * @details Extends the word complex text representation and adjusts position boundaries
     * based on the adjacent phrase (left or right).
     *
     * @param wc Current word complex being updated (modified in place)
     * @param asidePhrase Adjacent phrase to merge
     * @param curPhrStatus Current phrase validation status (updated in place)
     * @param isLeft Direction flag (true for extending left, false for right)
     */
    void updatePhraseStatus(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase,
                            CurrentPhraseStatus& curPhrStatus, bool isLeft);
};
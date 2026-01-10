#pragma once

#include "ModelComponent.h"
#include "Phrase.h"
#include "PhraseMatchStatus.h"
#include "PhraseValidator.h"

/**
 * @brief Manages recursive phrase boundary extension with dispatcher pattern
 * @details Handles both WordComp and ModelComp component types through a dispatcher.
 * Stores expansion context (model, collection, status, phraseIndex) set during construction
 * to enable clean recursive checking without excessive parameter passing.
 *
 * Context is initialized once at construction and remains valid for the lifetime of this object.
 * Designed to be created as a temporary/local variable for a single expansion operation.
 */
class PhraseExtender {
  public:
    /**
     * @brief Constructs PhraseExtender with all necessary context for recursive expansion
     * @details Initializes both static references (simplePhrases, sentence, validator)
     * and dynamic context (model, collection, status, phraseIndex) needed for component checking.
     * This is the primary constructor used to set up the extender for an expansion operation.
     *
     * @param model Grammar model defining component structure
     * @param collection Output collection where matched phrases are accumulated
     * @param status Phrase match status object tracking validation progress
     * @param simplePhraseIndex Index of the current simple phrase being expanded
     * @param simplePhrases Vector of all simple phrases in the sentence
     * @param sentence Vector of word forms (tokens) in the sentence
     * @param validator Validator for morphological compatibility checks
     */
    explicit PhraseExtender(const std::shared_ptr<Model>& model, std::vector<PhrasePtr>& collection,
                            PhraseMatchStatus& status, size_t simplePhraseIndex,
                            const std::vector<PhrasePtr>& simplePhrases, const std::vector<X::WordFormPtr>& sentence)
        : m_currentModel(model), m_currentCollection(&collection), m_currentStatus(&status),
          m_currentSimplePhraseIndex(simplePhraseIndex), m_simplePhrases(simplePhrases), m_sentence(sentence) {
    }

    /**
     * @brief Dispatches component checking based on component type (WordComp vs ModelComp)
     * @details Main recursive entry point that selects the appropriate handler for current component.
     * Performs boundary validation before dispatching and routes to either checkWordComponentImpl()
     * or checkModelComponentImpl() based on component type.
     *
     * Boundary checks ensure:
     *   - m_currentModel is valid and initialized
     *   - componentIndex < model->size() (component exists)
     *   - formIndex < sentence->size() (token exists)
     *
     * @param componentIndex Current component index in the grammar model
     * @param formIndex Current token index in the sentence
     * @param isLeft Direction flag (true for leftward/backward expansion, false for rightward/forward)
     * @param wc Word complex being expanded (modified in place during recursion)
     * @return true if matching continues successfully, false if boundary exceeded or no match
     */
    bool checkComponent(size_t componentIndex, size_t formIndex, bool isLeft, const PhrasePtr& wc);

    /**
     * @brief Validates if an adjacent phrase should be included during expansion
     * @details Performs four-stage filtering to determine validity of adjacent phrase:
     * 1. Index bounds check (phraseIndex must be in m_simplePhrases)
     * 2. Self-exclusion check (not the current phrase being processed)
     * 3. Positional check (correct direction: left phrase must be left, right must be right)
     * 4. Form matching check (adjacent phrase's modelName must match expected form)
     *
     * @param phraseIndex Index of phrase candidate in m_simplePhrases
     * @param currentIndex Index of phrase currently being expanded
     * @param isLeft Direction flag (true = looking for left/preceding phrases)
     * @param currentPhrase The word complex currently being expanded
     * @param modelComp Model component defining expected form name
     * @return true if phrase should be skipped, false if it's a valid adjacent phrase
     */
    bool shouldSkipAdjacentPhrase(size_t phraseIndex, size_t currentIndex, bool isLeft, const PhrasePtr& currentPhrase,
                                  const std::shared_ptr<ModelComp>& modelComp) const;

    /**
     * @brief Merges an adjacent phrase into the current word complex
     * @details Updates position boundaries and text content of target complex to include adjacent phrase.
     * Increments m_currentStatus->matchedComponents to track expansion progress.
     *
     * For left attachment (preceding phrase):
     *   - Updates target->pos.start to adjacent->pos.start
     *   - Prepends adjacent text: target.text = "adjacent text " + target.text
     *
     * For right attachment (following phrase):
     *   - Updates target->pos.end to adjacent->pos.end
     *   - Appends adjacent text: target.text = target.text + " adjacent text"
     *
     * @param target Word complex to expand (modified in place)
     * @param adjacent Adjacent phrase to merge in
     * @param status Match status to update (increments matchedComponents)
     * @param isLeft Direction of attachment (true for left, false for right)
     */
    void attachAdjacentPhrase(const PhrasePtr& target, const PhrasePtr& adjacent, PhraseMatchStatus& status,
                              bool isLeft);

  private:
    /// @brief Reference to simple phrases collection (immutable, set at construction)
    const std::vector<PhrasePtr>& m_simplePhrases;

    /// @brief Reference to word forms/tokens in current sentence (immutable, set at construction)
    const std::vector<X::WordFormPtr>& m_sentence;

    /// @brief Current grammar model being processed (set at construction, valid for lifetime)
    std::shared_ptr<Model> m_currentModel;

    /// @brief Pointer to output collection for matched phrases (set at construction, valid for lifetime)
    std::vector<PhrasePtr>* m_currentCollection;

    /// @brief Pointer to phrase match status (set at construction, valid for lifetime)
    PhraseMatchStatus* m_currentStatus;

    /// @brief Index of current simple phrase in m_simplePhrases (set at construction)
    size_t m_currentSimplePhraseIndex;

    /**
     * @brief Processes a single word token (WordComp) in recursive expansion
     * @details Core recursive handler for word component matching:
     *
     * Matching flow:
     * 1. Validate token existence and morphological compatibility
     * 2. Update head validation status (if applicable)
     * 3. Append/prepend token to word complex
     * 4. Increment matched components counter
     * 5. Recurse to next component in direction (if not at boundary)
     * 6. Save completed pattern to m_currentCollection (if reached end)
     * 7. Handle repetition flag isRec() (if component allows repeating)
     *
     * Recursive behavior:
     *   - If not at model boundary: recurse to next component with next token
     *   - If at end: save pattern and optionally allow component repetition
     *
     * @param componentIndex Current component index in m_currentModel
     * @param formIndex Current token index in m_sentence
     * @param isLeft Direction (true = moving leftward/backward, false = rightward/forward)
     * @param wc Word complex being built (modified in place)
     * @return true if complete pattern found and saved, false if matching failed
     */
    bool checkWordComponentImpl(size_t componentIndex, size_t formIndex, bool isLeft, const PhrasePtr& wc);

    /**
     * @brief Processes a nested model component (ModelComp) in recursive expansion
     * @details Complex handler for model component matching with multi-phrase integration:
     *
     * Processing flow:
     * 1. Iterate through all simple phrases to find adjacent ones
     * 2. For each valid adjacent phrase (via shouldSkipAdjacentPhrase):
     *    a. Validate head element (if component has head and not yet validated)
     *    b. Validate lexical conditions (check word forms match expectations)
     *    c. Attach adjacent phrase to current complex (merge positions and text)
     *    d. Recurse to next model component in direction
     * 3. When all components validated: save complete pattern to m_currentCollection
     *
     * Validation flow:
     * - Head check: verifies head word token at specified position matches head conditions
     * - Lex check: verifies all words in adjacent phrase match lexical model conditions
     * - Attachment: updates current complex boundaries and text to include adjacent phrase
     * - Recursion: if more components exist, moves to next component at new position
     *
     * @param componentIndex Current component index in m_currentModel
     * @param formIndex Current token index in m_sentence (head word position)
     * @param isLeft Direction (true = left/preceding components, false = right/following)
     * @param wc Word complex being built (modified in place by attachAdjacentPhrase)
     * @return false always (internal collection is populated, not return value)
     */
    bool checkModelComponentImpl(size_t componentIndex, size_t formIndex, bool isLeft, const PhrasePtr& wc);
};

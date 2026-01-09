#pragma once

#include <xmorphy/morph/WordForm.h>
#include <xmorphy/tag/UniMorphTag.h>
#include <xmorphy/tag/UniSPTag.h>

using namespace X;

/**
 * @brief Describes syntactic roles of components within sentences
 * @details Determines the grammatical function of a phrase component
 */
enum class SyntaxRole {
    Head,       // The central word of a phrase.
    Dependent,  // Dependent on the head.
    Independent // Neither dependent nor a head.
};

/**
 * @brief Defines all grammatical conditions for matching a component
 * @details Combines syntax role, morphological tags, exact lexeme matching,
 * and recursion flags into a single validation rule.
 *
 * Validation checks three aspects:
 *   1. Syntax role (head, dependent, or independent)
 *   2. Morphological tag attributes (case, number, tense, etc.)
 *   3. Exact lexeme matching (if exLex is specified) and recursion flag
 */
class Condition {
  private:
    /// @brief Syntactic role of this component
    SyntaxRole m_role;

    /// @brief Morphological tag constraints for matching
    UniMorphTag m_tag;

    /// @brief Flag indicating if component can be recursively repeated
    bool m_isRecursive = false;

    /// @brief Exact lexeme to match (empty string = accept any lexeme)
    std::string m_exLex = "";

  public:
    Condition(SyntaxRole role = SyntaxRole::Independent, UniMorphTag morphTag = UniMorphTag::UNKN,
              bool isRecursive = false, const std::string& exLex = "");

    ~Condition() = default;

    /**
     * @brief Validates morphological tag attributes against this condition
     * @details Checks all morphological attributes (case, number, tense, etc.)
     * defined in this condition against the provided morphological form.
     * Only attributes explicitly set in condition are validated.
     *
     * @param morphForm Morphological information to validate
     * @return true if all set attributes match, false if any mismatch
     */
    [[nodiscard]] bool morphTagCheck(const MorphInfo& morphForm) const;

    /**
     * @brief Validates a word form against this complete condition
     * @details Three-step validation with immediate return on failure:
     * 1. Find morphological form with matching speech part tag
     * 2. Validate morphological tag attributes
     * 3. Validate exact lexeme match (if specified)
     *
     * Returns immediately on first validation failure.
     *
     * @param spTag Speech part tag to match
     * @param form Word form containing morphological information
     * @return true if form satisfies all conditions, false otherwise
     */
    [[nodiscard]] bool check(const X::UniSPTag spTag, const X::WordFormPtr& form) const;

    // ─────────────────────────────────────────────────────────────────────
    // Getters
    // ─────────────────────────────────────────────────────────────────────

    /// @brief Returns the morphological tag constraints
    [[nodiscard]] const UniMorphTag& getMorphTag() const;

    /// @brief Returns the syntactic role
    [[nodiscard]] SyntaxRole getSyntaxRole() const;

    /// @brief Returns whether component can be recursively repeated
    [[nodiscard]] bool isRecursive() const;

    /// @brief Returns the exact lexeme constraint (empty if none)
    [[nodiscard]] const std::string& getExactLexeme() const;

    // ─────────────────────────────────────────────────────────────────────
    // Validation helpers
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Checks if this condition has any constraints (non-default state)
     * @details A condition is empty if:
     *   - Morphological tag is UNKN (no tag constraint)
     *   - No exact lexeme specified
     *
     * @return true if condition is in default/unconstrained state
     */
    [[nodiscard]] bool isDefault() const;

    /**
     * @brief Checks if exact lexeme matching is required
     * @return true if exact lexeme is specified (non-empty string)
     */
    [[nodiscard]] bool hasExactLexeme() const;

    /**
     * @brief Validates morphological form against exact lexeme constraint
     * @details Compares normalized form of morphForm with exact lexeme.
     * If exact lexeme is not specified, always returns true (any lexeme accepted).
     *
     * Exact lexeme matching is case-insensitive, based on normalized word form.
     *
     * @param morphForm Morphological information to check
     * @return true if form matches exact lexeme or no constraint set
     */
    [[nodiscard]] bool matchesExactLexeme(const X::MorphInfo& morphForm) const;

  private:
    /**
     * @brief Finds morphological form by speech part tag
     * @details Searches through all morphological forms in the word form
     * and returns the first one matching the specified speech part tag.
     *
     * @param form Word form to search within
     * @param spTag Speech part tag to find
     * @return Pointer to matching morphological form, nullptr if not found
     */
    const X::MorphInfo* findMorphFormBySpeechPart(const X::WordFormPtr& form, X::UniSPTag spTag) const;
};

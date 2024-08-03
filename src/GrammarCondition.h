#ifndef GRAMMAR_CONDITION_H
#define GRAMMAR_CONDITION_H

#include <xmorphy/morph/WordForm.h>
#include <xmorphy/tag/UniMorphTag.h>
#include <xmorphy/tag/UniSPTag.h>

#include <Logger.h>

#include <memory>
#include <optional>

using namespace X;

// Enum to describe syntactic roles of components within a sentence.
enum class SyntaxRole {
    Head,       // The central word of a phrase.
    Dependent,  // Dependent on the head.
    Independent // Neither dependent nor a head.
};

// Struct to manage additional grammatical conditions.
struct Additional {
    bool m_rec = false;                     // Flag for recursion, if needed.
    std::string m_exLex = "";               // Example lexicon (specific words to match).
    std::vector<std::string> m_themes = {}; // Themes associated with this condition.

    // Checks if the Additional instance has no data.
    bool empty() const;

    // Checks if a specific word form matches the example lexicon.
    bool exLexCheck(const X::MorphInfo& morphForm) const;

    // Placeholder for logic to compare themes.
    bool themesCheck() const;

    bool check(const X::MorphInfo& morphForm) const;
};

// Class to define conditions for matching grammatical components.
class Condition {
private:
    SyntaxRole m_role;
    UniMorphTag m_tag;
    Additional m_addcond;

public:
    Condition(SyntaxRole role = SyntaxRole::Independent, UniMorphTag morphTag = UniMorphTag::UNKN,
              Additional cond = Additional());
    ~Condition() = default;

    bool morphTagCheck(const MorphInfo& morphForm) const;

    const UniMorphTag getMorphTag() const;
    const Additional getAdditional() const;
    const SyntaxRole getSyntaxRole() const;

    // Checks if the Condition instance contains default or empty values.
    bool empty() const;

    bool check(const X::UniSPTag spTag, const X::WordFormPtr& form) const;
};

#endif // GRAMMAR_CONDITION_H

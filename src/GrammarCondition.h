#ifndef GRAMMAR_CONDITION_H
#define GRAMMAR_CONDITION_H

#include <Logger.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <xmorphy/morph/WordForm.h>
#include <xmorphy/tag/UniMorphTag.h>
#include <xmorphy/tag/UniSPTag.h>

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
    bool empty() const
    {
        return m_themes.empty() && m_exLex.empty();
    }

    // Checks if a specific word form matches the example lexicon.
    bool exLexCheck(const X::MorphInfo& morphForm) const
    {
        return m_exLex.empty() || (m_exLex == morphForm.normalForm.toLowerCase().getRawString());
    }

    // Placeholder for logic to compare themes.
    bool themesCheck() const
    {
        if (const auto& theme = m_themes; !theme.empty()) {
            // TODO: Add logic to compare themes
        }
        return true;
    }

    bool check(const X::MorphInfo& morphForm) const
    {
        if (!empty()) {
            Logger::log("Check", LogLevel::Debug, "Checking additional conditions.");
            if (!exLexCheck(morphForm)) {
                Logger::log("Check", LogLevel::Debug, "exLexCheck failed.");
                return false;
            }
            if (!themesCheck()) {
                Logger::log("Check", LogLevel::Debug, "themesCheck failed.");
                return false;
            }
        }
        return true;
    }
};

// Class to define conditions for matching grammatical components.
class Condition {
private:
    SyntaxRole m_role;
    UniMorphTag m_tag;
    Additional m_addcond;

public:
    Condition(SyntaxRole role = SyntaxRole::Independent, UniMorphTag morphTag = UniMorphTag::UNKN,
              Additional cond = Additional())
        : m_role(role), m_tag(morphTag), m_addcond(cond) {};

    const UniMorphTag getMorphTag() const
    {
        return m_tag;
    };
    const Additional getAdditional() const
    {
        return m_addcond;
    };
    const SyntaxRole getSyntaxRole() const
    {
        return m_role;
    };

    // Checks if the Condition instance contains default or empty values.
    bool empty() const
    {
        return m_tag == UniMorphTag::UNKN && m_addcond.empty();
    }

    // Method to determine if a given morphological form matches the condition.
    bool morphTagCheck(const X::MorphInfo& morphForm) const;

    bool check(const X::UniSPTag spBaseTag, const X::WordFormPtr& form) const
    {
        for (const auto& morphForm : form->getMorphInfo()) {
            Logger::log("check", LogLevel::Debug,
                        "Checking morphForm against spBaseTag.\n\tmorphForm: " + morphForm.normalForm.getRawString() +
                            ", " + morphForm.sp.toString() + "\t\tspBaseHeadTag: " + spBaseTag.toString());
            if (morphForm.sp == spBaseTag) {
                if (!morphTagCheck(morphForm)) {
                    Logger::log("check", LogLevel::Debug, "morphTagCheck failed.");
                    return false;
                }

                if (!m_addcond.check(morphForm))
                    return false;
            } else {
                Logger::log("check", LogLevel::Debug, "spBaseHeadTag does not match.");
                return false;
            }
        }
        Logger::log("check", LogLevel::Debug, "Exiting function, the return value is TRUE.");
        return true;
    }
};

#endif

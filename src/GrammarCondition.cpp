#include <GrammarCondition.h>

bool Additional::empty() const
{
    return m_themes.empty() && m_exLex.empty();
}

// Checks if a specific word form matches the example lexicon.
bool Additional::exLexCheck(const X::MorphInfo& morphForm) const
{
    return m_exLex.empty() || (m_exLex == morphForm.normalForm.toLowerCase().getRawString());
}

// Placeholder for logic to compare themes.
bool Additional::themesCheck() const
{
    if (const auto& theme = m_themes; !theme.empty()) {
        // TODO: Add logic to compare themes
    }
    return true;
}

bool Additional::check(const X::MorphInfo& morphForm) const
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

Condition::Condition(SyntaxRole role, UniMorphTag morphTag, Additional cond)
    : m_role(role), m_tag(morphTag), m_addcond(cond) {};

const UniMorphTag Condition::getMorphTag() const
{
    return m_tag;
};
const Additional Condition::getAdditional() const
{
    return m_addcond;
};
const SyntaxRole Condition::getSyntaxRole() const
{
    return m_role;
};

template <typename AttrType>
static bool checkAttribute(bool (X::UniMorphTag::*hasAttribute)() const,
                           AttrType (X::UniMorphTag::*getAttribute)() const, const X::UniMorphTag& tag,
                           const X::UniMorphTag& formTag)
{
    if ((tag.*hasAttribute)()) {
        bool result = (formTag.*hasAttribute)() && (tag.*getAttribute)() == (formTag.*getAttribute)();
        Logger::log("checkAttribute", LogLevel::Debug, "Attribute check: " + std::to_string(result));
        return result;
    }
    // Logger::log("checkAttribute", LogLevel::Debug, "No attribute to check, returning true.");
    return true;
}

bool Condition::morphTagCheck(const MorphInfo& morphForm) const
{
    const auto& compMorphTag = this->getMorphTag();
    return checkAttribute(&X::UniMorphTag::hasCase, &X::UniMorphTag::getCase, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasAnimacy, &X::UniMorphTag::getAnimacy, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasNumber, &X::UniMorphTag::getNumber, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasTense, &X::UniMorphTag::getTense, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasCmp, &X::UniMorphTag::getCmp, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasVerbForm, &X::UniMorphTag::getVerbForm, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasMood, &X::UniMorphTag::getMood, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasPerson, &X::UniMorphTag::getPerson, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasVariance, &X::UniMorphTag::getVariance, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasVoice, &X::UniMorphTag::getVoice, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasAspect, &X::UniMorphTag::getAspect, compMorphTag, morphForm.tag);
}

// Checks if the Condition instance contains default or empty values.
bool Condition::empty() const
{
    return m_tag == UniMorphTag::UNKN && m_addcond.empty();
}

bool Condition::check(const X::UniSPTag spTag, const X::WordFormPtr& form) const
{
    for (const auto& morphForm : form->getMorphInfo()) {
        Logger::log("check", LogLevel::Debug,
                    "Checking morphForm against spTag.\n\tmorphForm: " + morphForm.normalForm.getRawString() + ", " +
                        morphForm.sp.toString() + "\t\tspHeadTag: " + spTag.toString());
        if (morphForm.sp == spTag) {
            if (!morphTagCheck(morphForm)) {
                Logger::log("check", LogLevel::Debug, "morphTagCheck failed.");
                return false;
            }
            if (!m_addcond.check(morphForm))
                return false;
        } else {
            Logger::log("check", LogLevel::Debug, "spTagHeadTag does not match.");
            return false;
        }
    }
    Logger::log("check", LogLevel::Debug, "Exiting function, the return value is TRUE.");
    return true;
}
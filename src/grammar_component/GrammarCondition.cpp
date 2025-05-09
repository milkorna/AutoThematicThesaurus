#include <GrammarCondition.h>

bool Additional::empty() const
{
    return m_exLex.empty();
}

// Checks if a specific word form matches the example lexicon.
bool Additional::exLexCheck(const X::MorphInfo& morphForm) const
{
    return m_exLex.empty() || (m_exLex == morphForm.normalForm.toLowerCase().getRawString());
}

bool Additional::check(const X::MorphInfo& morphForm) const
{
    if (!empty()) {
        if (!exLexCheck(morphForm)) {
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
        return result;
    }
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
        if (morphForm.sp == spTag) {
            if (!morphTagCheck(morphForm)) {
                return false;
            }
            if (!m_addcond.check(morphForm))
                return false;
        } else {
            return false;
        }
    }
    return true;
}
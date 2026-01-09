#include <GrammarCondition.h>

Condition::Condition(SyntaxRole role, UniMorphTag morphTag, bool isRecursive, const std::string& exLex)
    : m_role(role), m_tag(morphTag), m_isRecursive(isRecursive), m_exLex(exLex) {};

namespace {
/// @brief Helper: validates a single morphological attribute
/// @details Uses member function pointers for generic attribute checking.
/// Returns true if attribute is not constrained or matches the expected value.
template <typename AttrType>
static bool validateAttribute(bool (X::UniMorphTag::*hasAttribute)() const,
                              AttrType (X::UniMorphTag::*getAttribute)() const, const X::UniMorphTag& tag,
                              const X::UniMorphTag& formTag) {
    if ((tag.*hasAttribute)()) {
        bool result = (formTag.*hasAttribute)() && (tag.*getAttribute)() == (formTag.*getAttribute)();
        return result;
    }
    return true;
}
} // namespace

bool Condition::morphTagCheck(const MorphInfo& morphForm) const {
    const auto& compMorphTag = this->getMorphTag();
    return validateAttribute(&X::UniMorphTag::hasCase, &X::UniMorphTag::getCase, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasAnimacy, &X::UniMorphTag::getAnimacy, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasNumber, &X::UniMorphTag::getNumber, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasTense, &X::UniMorphTag::getTense, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasCmp, &X::UniMorphTag::getCmp, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasVerbForm, &X::UniMorphTag::getVerbForm, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasMood, &X::UniMorphTag::getMood, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasPerson, &X::UniMorphTag::getPerson, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasVariance, &X::UniMorphTag::getVariance, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasVoice, &X::UniMorphTag::getVoice, compMorphTag, morphForm.tag) &&
           validateAttribute(&X::UniMorphTag::hasAspect, &X::UniMorphTag::getAspect, compMorphTag, morphForm.tag);
}

bool Condition::check(const X::UniSPTag spTag, const X::WordFormPtr& form) const {
    for (const auto& morphForm : form->getMorphInfo()) {
        if (morphForm.sp == spTag) {
            if (!morphTagCheck(morphForm)) {
                return false;
            }
            if (!matchesExactLexeme(morphForm)) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

bool Condition::isRecursive() const {
    return m_isRecursive;
}

const std::string& Condition::getExactLexeme() const {
    return m_exLex;
}

const UniMorphTag& Condition::getMorphTag() const {
    return m_tag;
};

SyntaxRole Condition::getSyntaxRole() const {
    return m_role;
};

bool Condition::isDefault() const {
    return m_tag == UniMorphTag::UNKN && m_exLex.empty();
}

bool Condition::hasExactLexeme() const {
    return !m_exLex.empty();
}

bool Condition::matchesExactLexeme(const X::MorphInfo& morphForm) const {
    // If no exact lexeme specified, any form matches
    if (m_exLex.empty()) {
        return true;
    }
    // Match against normalized form (case-insensitive)
    return m_exLex == morphForm.normalForm.toLowerCase().getRawString();
}

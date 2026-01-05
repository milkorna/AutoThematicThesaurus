
#include "MorphAnalyzer.h"
#include "xmorphy/morph/WordForm.h"

X::MorphInfo MorphAnalyzer::getMostProbableMorphInfo(const std::unordered_set<X::MorphInfo>& morphSet) const {
    auto maxElement = *morphSet.begin();
    for (const auto& elem : morphSet) {
        if (elem.probability > maxElement.probability) {
            maxElement = elem;
        }
    }
    return maxElement;
}

bool MorphAnalyzer::isMorphAnalysisError(const X::WordFormPtr& token) const {
    auto isDesiredPOS = [](const X::UniSPTag& tag) -> bool {
        static const std::unordered_set<std::string> desiredPOS = {"ADJ", "NOUN", "PROPN", "VERB"};
        return desiredPOS.find(tag.toString()) != desiredPOS.end();
    };

    return token->getWordForm().length() == 1 && token->getMorphInfo().size() == 1 &&
           isDesiredPOS(token->getMorphInfo().begin()->sp);
}

const std::string MorphAnalyzer::getLemma(const X::WordFormPtr& form) const {
    return getMostProbableMorphInfo(form->getMorphInfo()).normalForm.toLowerCase().getRawString();
}

const std::string MorphAnalyzer::getNormalForm(const X::WordFormPtr& form) const {
    return getMostProbableMorphInfo(form->getMorphInfo()).normalForm.getRawString();
}
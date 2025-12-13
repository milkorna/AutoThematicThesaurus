
#include "MorphAnalyzer.h"
#include "PhrasesCollectorUtils.h"
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

const std::string MorphAnalyzer::getLemma(const X::WordFormPtr& form) const {
    return getMostProbableMorphInfo(form->getMorphInfo()).normalForm.toLowerCase().getRawString();
}

const std::string MorphAnalyzer::getNormalForm(const X::WordFormPtr& form) const {
    return getMostProbableMorphInfo(form->getMorphInfo()).normalForm.getRawString();
}
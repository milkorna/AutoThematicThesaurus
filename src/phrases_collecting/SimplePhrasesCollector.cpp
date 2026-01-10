#include "SimplePhrasesCollector.h"
#include "GrammarPatternManager.h"
#include "Process.h"

const std::vector<PhrasePtr>& SimplePhrasesCollector::getCollection() const {
    return m_collection;
}

void SimplePhrasesCollector::collect(Process& process) {
    const auto& simplePatterns = GrammarPatternManager::GetManager().getSimplePatterns();

    for (size_t tokenIndex = 0; tokenIndex < m_sentence.size(); tokenIndex++) {
        const auto& token = m_sentence[tokenIndex];

        // Skip invalid tokens and non-head candidates
        if (!isValidPhraseHead(token)) {
            continue;
        }

        // Try to match token as head in each grammar model
        for (const auto& [name, model] : simplePatterns) {

            // Check if token matches model's head component
            if (!model->getHead()->isValidCondition(token)) {
                continue;
            }

            // Attempt to expand phrase with this model
            if (tryExpandPhraseWithModel(process, model, tokenIndex, token)) {
                break;
            }
        }
    }

    // Output all collected phrases
    process.outputResults(m_collection);
}

bool SimplePhrasesCollector::expandPhraseInDirection(const PhrasePtr& phrase, const std::shared_ptr<Model>& model,
                                                     size_t compIndex, size_t tokenInd, size_t& correct,
                                                     const bool isLeft) {

    const auto& comp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[compIndex]);
    const auto& token = m_sentence[tokenInd];
    if (!m_validator.isTokenValid(token)) {
        return false;
    }

    if (!comp->isValidCondition(token)) {
        return false;
    }

    if (isLeft) {
        phrase->addWordToLeft(token);
    } else {
        phrase->addWordToRight(token);
    }

    ++correct;
    const size_t nextCompIndex = isLeft ? compIndex - 1 : compIndex + 1;
    const size_t nextTokenInd = isLeft ? tokenInd - 1 : tokenInd + 1;

    if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->size() - 1)) {
        return expandPhraseInDirection(phrase, model, nextCompIndex, nextTokenInd, correct, isLeft);
    } else {
        m_collection.push_back(std::make_shared<Phrase>(*phrase));
        if (comp->isRec() && ((isLeft && tokenInd > 0) || (!isLeft && tokenInd < m_sentence.size() - 1))) {
            return expandPhraseInDirection(phrase, model, compIndex, nextTokenInd, correct, isLeft);
        }
    }

    return false;
}

bool SimplePhrasesCollector::tryExpandPhraseWithModel(Process& process, const std::shared_ptr<Model>& model,
                                                      size_t tokenIndex, const X::WordFormPtr& token) {
    if (!model || !token) {
        return false;
    }

    const auto headPosition = model->getHeadPos();
    if (!headPosition.has_value()) {
        return false;
    }

    auto phrase = Phrase::createFromToken(tokenIndex, token, model->getForm(), process);
    size_t matchedComponents = 1;

    if (headPosition.value() > 0 && tokenIndex > 0) {
        if (expandPhraseInDirection(phrase, model, headPosition.value() - 1, tokenIndex - 1, matchedComponents, true)) {
            return true;
        }
    }

    if (headPosition.value() != model->size() - 1 && tokenIndex + 1 < m_sentence.size()) {
        return expandPhraseInDirection(phrase, model, headPosition.value() + 1, tokenIndex + 1, matchedComponents,
                                       false);
    }

    return false;
}

bool SimplePhrasesCollector::isValidPhraseHead(const X::WordFormPtr& token) const {
    if (!token) {
        return false;
    }

    // Check if token is valid
    if (!m_validator.isTokenValid(token)) {
        return false;
    }

    // Check if token has morphology
    if (token->getMorphInfo().empty()) {
        return false;
    }

    // Check if any morph form matches a head speech part
    const auto& usedHeadSpeechParts = GrammarPatternManager::GetManager().getUsedHeadSp();

    for (const auto& morphForm : token->getMorphInfo()) {
        if (usedHeadSpeechParts.contains(morphForm.sp.toString())) {
            return true; // Found valid head speech part
        }
    }

    return false;
}
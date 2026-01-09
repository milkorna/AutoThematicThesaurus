#include "PhraseValidator.h"
#include "MorphAnalyzer.h"
#include "Options.h"
#include "StopWordsManager.h"

// ════════════════════════════════════════════════════════════════════════════
// PhraseValidator Implementation
// ════════════════════════════════════════════════════════════════════════════

bool PhraseValidator::isTokenValid(const X::WordFormPtr& token) {
    auto& options = Options::getOptions();
    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    // Проверка 1: Стоп-слова
    if (options.cleanStopWords) {
        if (StopWordsManager::isStopWord(token->getWordForm().toLowerCase().getRawString())) {
            return false;
        }
        const auto lemma = morphAnalyzer.getLemma(token);
        if (StopWordsManager::isStopWord(lemma)) {
            return false;
        }
    }

    // Проверка 2: Пунктуация и числа
    if (token->getTokenType() == X::TokenTypeTag::PNCT || token->getTokenType() == X::TokenTypeTag::NUMB) {
        return false;
    }

    // Проверка 3: Ошибки морфанализа
    if (morphAnalyzer.isMorphAnalysisError(token)) {
        return false;
    }

    return true;
}

bool PhraseValidator::validateMorphology(const X::WordFormPtr& token, const Condition& condition,
                                         PhraseMatchStatus& status) {
    const auto& morphForms = token->getMorphInfo();

    for (const auto& morphForm : morphForms) {
        if (!condition.morphTagCheck(morphForm)) {
            continue;
        }

        status.headValidated = true;
        status.headMatched = true;

        if (condition.matchesExactLexeme(morphForm)) {
            status.lexFound = true;
        }
        return true;
    }
    return false;
}

bool PhraseValidator::validateWordComponents(const std::vector<X::WordFormPtr>& sentence, const WordComplexPtr& phrase,
                                             const std::shared_ptr<ModelComp>& modelComp, PhraseMatchStatus& status) {
    size_t componentIndex = 0;

    for (const auto& component : modelComp->getComponents()) {
        if (const auto& wordComp = std::dynamic_pointer_cast<WordComp>(component)) {
            const auto& morphForms = sentence[phrase->pos.start + componentIndex++]->getMorphInfo();

            if (validateMorphology(sentence[phrase->pos.start + componentIndex - 1], modelComp->getHead()->condition(),
                                   status)) {
                if (wordComp->isHead()) {
                    status.headValidated = true;
                    status.headMatched = true;
                }
                return true;
            }
        }
    }
    return false;
}

bool PhraseValidator::isPhraseComplete(const PhraseMatchStatus& status) {
    return status.headValidated && status.headMatched && status.lexFound;
}
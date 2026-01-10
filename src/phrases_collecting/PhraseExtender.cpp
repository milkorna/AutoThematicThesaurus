#include "PhraseExtender.h"

bool PhraseExtender::checkComponent(size_t componentIndex, size_t formIndex, bool isLeft, const PhrasePtr& wc) {

    // Проверяем что контекст установлен и индексы валидны
    if (!m_currentModel || componentIndex >= m_currentModel->size() || formIndex >= m_sentence.size()) {
        return false;
    }

    auto comp = m_currentModel->getComponents()[componentIndex];

    // Диспетчинг: какой тип компонента?
    if (std::dynamic_pointer_cast<WordComp>(comp)) {
        return checkWordComponentImpl(componentIndex, formIndex, isLeft, wc);
    } else if (std::dynamic_pointer_cast<ModelComp>(comp)) {
        return checkModelComponentImpl(componentIndex, formIndex, isLeft, wc);
    }

    return false;
}

bool PhraseExtender::checkWordComponentImpl(size_t componentIndex, size_t formIndex, bool isLeft, const PhrasePtr& wc) {

    auto comp = m_currentModel->getComponents()[componentIndex];
    auto wordComp = std::dynamic_pointer_cast<WordComp>(comp);

    if (!wordComp) {
        return false;
    }

    if (formIndex >= m_sentence.size()) {
        return false;
    }

    const auto token = m_sentence[formIndex];

    if (!PhraseValidator::isTokenValid(token)) {
        return false;
    }

    // Проверка морфологии
    if (!wordComp->condition().check(wordComp->getSPTag(), token)) {
        return false;
    }

    if (!m_currentStatus->headValidated) {
        m_currentStatus->headValidated = true;
        m_currentStatus->headMatched = true;
    }

    // Обновление фразы
    std::string formFromText = token->getWordForm().getRawString();
    if (isLeft) {
        wc->addWordToLeft(token);
    } else {
        wc->addWordToRight(token);
    }

    m_currentStatus->matchedComponents++;

    size_t nextCompIndex = isLeft ? componentIndex - 1 : componentIndex + 1;
    size_t nextFormIndex = isLeft ? formIndex - 1 : formIndex + 1;

    if (isLeft && formIndex == 0) {
        return false;
    }

    // Рекурсия к следующему компоненту (используем диспетчер)
    if ((isLeft && componentIndex > 0) || (!isLeft && componentIndex < m_currentModel->size() - 1)) {
        if (!checkComponent(nextCompIndex, nextFormIndex, isLeft, wc)) {
            return false;
        }
    } else {
        // Паттерн завершен
        if (m_currentCollection->empty() || wc->textForm != m_currentCollection->back()->textForm) {
            m_currentCollection->push_back(std::make_shared<Phrase>(*wc));
        }

        if (wordComp->isRec() && ((isLeft && formIndex > 0) || (!isLeft && formIndex < m_sentence.size() - 1))) {
            if (checkComponent(componentIndex, nextFormIndex, isLeft, wc)) {
                return true;
            }
            return false;
        }
    }

    return false;
}

bool PhraseExtender::checkModelComponentImpl(size_t componentIndex, size_t formIndex, bool isLeft,
                                             const PhrasePtr& wc) {

    auto comp = m_currentModel->getComponents()[componentIndex];
    auto modelComp = std::dynamic_pointer_cast<ModelComp>(comp);

    if (!modelComp) {
        return false;
    }

    if (formIndex >= m_sentence.size() || m_currentSimplePhraseIndex >= m_simplePhrases.size()) {
        return false;
    }

    for (size_t smpPhrOffset = 0; smpPhrOffset < m_simplePhrases.size(); smpPhrOffset++) {

        const auto& asidePhrase = m_simplePhrases[smpPhrOffset];

        if (shouldSkipAdjacentPhrase(smpPhrOffset, m_currentSimplePhraseIndex, isLeft, wc, modelComp)) {
            continue;
        }

        // Проверка head
        if (!m_currentStatus->headValidated && modelComp->isHead()) {
            if (formIndex + *modelComp->getHeadPos() >= m_sentence.size()) {
                continue;
            }

            if (!modelComp->getHead()->condition().check(modelComp->getHead()->getSPTag(),
                                                         m_sentence[formIndex + *modelComp->getHeadPos()])) {
                return false;
            }

            m_currentStatus->headValidated = true;
            m_currentStatus->headMatched = true;
        }

        // Проверка лексики
        if (!m_currentStatus->lexFound) {
            bool lexMatched = true;
            const auto curSimplePhr = m_simplePhrases[m_currentSimplePhraseIndex];

            for (size_t offset = 0; offset < curSimplePhr->words.size(); offset++) {
                if (formIndex + offset >= m_sentence.size()) {
                    lexMatched = false;
                    break;
                }

                bool wordLexMatched = false;
                for (const auto& morphForm : m_sentence[formIndex + offset]->getMorphInfo()) {
                    if (modelComp->getCondition().matchesExactLexeme(morphForm)) {
                        wordLexMatched = true;
                        break;
                    }
                }

                if (!wordLexMatched) {
                    lexMatched = false;
                    break;
                }
            }

            if (lexMatched) {
                m_currentStatus->lexFound = true;
            } else {
                continue;
            }
        }

        size_t nextFormIndex = isLeft ? formIndex - 1 : formIndex + 1;

        // Присоединяем соседнюю фразу
        // todo
        if (isLeft && asidePhrase->pos.end == formIndex) {
            wc->mergeLeft(asidePhrase);
            attachAdjacentPhrase(wc, asidePhrase, *m_currentStatus, true);
        } else if (!isLeft && asidePhrase->pos.start == formIndex) {
            wc->mergeRight(asidePhrase);
            attachAdjacentPhrase(wc, asidePhrase, *m_currentStatus, false);
        }

        // Рекурсия к следующему компоненту (используем диспетчер)
        if (componentIndex > 0 && wc->pos.start > 0) {
            const auto curSimplePhr = m_simplePhrases[m_currentSimplePhraseIndex];
            if (curSimplePhr->pos.start - 1 == nextFormIndex) {
                if (checkComponent(componentIndex - 1, curSimplePhr->pos.start - 1, isLeft, wc)) {
                    break;
                }
            }
        }

        if (componentIndex < m_currentModel->size() - 1) {
            const auto curSimplePhr = m_simplePhrases[m_currentSimplePhraseIndex];
            if (curSimplePhr->pos.end + 1 == nextFormIndex) {
                if (checkComponent(componentIndex + 1, curSimplePhr->pos.end + 1, isLeft, wc)) {
                    break;
                }
            }
        }

        // Проверка готовности
        if (m_currentStatus->isValid() && componentIndex == m_currentModel->size() - 1 &&
            m_currentStatus->matchedComponents >= m_currentModel->size()) {
            if (m_currentCollection->empty() || wc->textForm != m_currentCollection->back()->textForm) {
                m_currentCollection->push_back(std::make_shared<Phrase>(*wc));
            }
        }
    }

    return false;
}

bool PhraseExtender::shouldSkipAdjacentPhrase(size_t phraseIndex, size_t currentIndex, bool isLeft,
                                              const PhrasePtr& currentPhrase,
                                              const std::shared_ptr<ModelComp>& modelComp) const {
    if (phraseIndex >= m_simplePhrases.size()) {
        return true;
    }

    if (phraseIndex == currentIndex) {
        return true;
    }

    const auto& adjacent = m_simplePhrases[phraseIndex];

    if (isLeft && adjacent->pos.start >= currentPhrase->pos.end) {
        return true;
    }
    if (!isLeft && adjacent->pos.start <= currentPhrase->pos.end) {
        return true;
    }

    if (adjacent->modelName != modelComp->getForm()) {
        return true;
    }

    return false;
}

void PhraseExtender::attachAdjacentPhrase(const PhrasePtr& target, const PhrasePtr& adjacent, PhraseMatchStatus& status,
                                          bool isLeft) {
    status.matchedComponents++;

    if (isLeft) {
        target->pos.start = adjacent->pos.start;
        target->textForm.insert(0, adjacent->textForm + " ");
    } else {
        target->pos.end = adjacent->pos.end;
        target->textForm.append(" " + adjacent->textForm);
    }
}

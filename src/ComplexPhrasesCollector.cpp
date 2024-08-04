#include <ComplexPhrasesCollector.h>
#include <PatternPhrasesStorage.h>

using namespace PhrasesCollectorUtils;

bool ComplexPhrasesCollector::CheckMorphologicalTags(const std::unordered_set<MorphInfo>& morphForms,
                                                     const Condition& cond, CurrentPhraseStatus& curPhrStatus)
{
    for (const auto& morphForm : morphForms) {
        if (!cond.morphTagCheck(morphForm)) {
            Logger::log("CheckCurrentSimplePhrase", LogLevel::Debug, "morphTagCheck failed.");
            continue;
        } else {
            curPhrStatus.headIsChecked = true;
            curPhrStatus.headIsMatched = true;
            if (cond.getAdditional().exLexCheck(morphForm)) {
                curPhrStatus.foundLex = true;
            }
            return true;
        }
    }
    return false;
}

bool ComplexPhrasesCollector::CheckWordComponents(const WordComplexPtr& curSimplePhr,
                                                  const std::shared_ptr<ModelComp>& curModelComp,
                                                  CurrentPhraseStatus& curPhrStatus)
{
    size_t wcInd = 0;
    for (const auto& wordComp : curModelComp->getComponents()) {
        if (const auto& wc = std::dynamic_pointer_cast<WordComp>(wordComp)) {
            const auto& morphForms = m_sentence[curSimplePhr->pos.start + wcInd++]->getMorphInfo();
            if (CheckMorphologicalTags(morphForms, curModelComp->getHead()->getCondition(), curPhrStatus)) {
                if (wc->isHead()) {
                    curPhrStatus.headIsChecked = true;
                    curPhrStatus.headIsMatched = true;
                }
                return true;
            }
        }
    }
    return false;
}

bool ComplexPhrasesCollector::CheckCurrentSimplePhrase(const WordComplexPtr& curSimplePhr,
                                                       const std::shared_ptr<ModelComp>& curModelComp,
                                                       CurrentPhraseStatus& curPhrStatus)
{
    const auto& addCond = curModelComp->getCondition().getAdditional();
    bool simplePhrAddCond = addCond.empty();
    bool simplePhrMorph = CheckWordComponents(curSimplePhr, curModelComp, curPhrStatus);

    if (curPhrStatus.headIsChecked && !curPhrStatus.headIsMatched) {
        return false;
    }

    if (!simplePhrAddCond) {
        return false;
    }

    return simplePhrMorph;
}

bool ComplexPhrasesCollector::ShouldSkip(size_t smpPhrOffset, size_t curSimplePhrInd, bool isLeft,
                                         const WordComplexPtr& wc, std::shared_ptr<ModelComp> modelComp)
{
    if (smpPhrOffset >= m_simplePhrases.size() || smpPhrOffset < 0) {
        return true;
    }

    if (smpPhrOffset == curSimplePhrInd) {
        return true;
    }

    if (isLeft && m_simplePhrases[smpPhrOffset]->pos.start >= wc->pos.end) {
        return true;
    }

    if (!isLeft && m_simplePhrases[smpPhrOffset]->pos.start <= wc->pos.end) {
        return true;
    }

    if (m_simplePhrases[smpPhrOffset]->modelName != modelComp->getForm()) {
        return true;
    }

    return false;
}

bool ComplexPhrasesCollector::CheckAside(size_t curSPhPosCmp, const WordComplexPtr& wc,
                                         const std::shared_ptr<Model>& model, size_t compIndex, size_t formIndex,
                                         const bool isLeft, CurrentPhraseStatus& curPhrStatus, size_t curSimplePhrInd)
{
    auto comp = model->getComponents()[compIndex];

    // Check if the component is a WordComp
    if (auto wordComp = std::dynamic_pointer_cast<WordComp>(comp)) {

        if (MorphAnanlysisError(m_sentence[formIndex]) || !HaveSp(m_sentence[formIndex]->getMorphInfo()))
            return false;

        std::string formFromText = m_sentence[formIndex]->getWordForm().getRawString();
        Logger::log("CheckAside", LogLevel::Debug, "FormFromText: " + formFromText);

        if (!wordComp->getCondition().check(wordComp->getSPTag(), m_sentence[formIndex])) {
            Logger::log("CheckAside", LogLevel::Debug, "check failed.");
            return false;
        } else {
            if (!curPhrStatus.headIsChecked) {
                curPhrStatus.headIsChecked = true;
                curPhrStatus.headIsMatched = true;
            }
        }

        UpdateWordComplex(wc, m_sentence[formIndex], formFromText, isLeft);

        curPhrStatus.correct++;
        size_t nextCompIndex = isLeft ? compIndex - 1 : compIndex + 1;
        size_t nextFormIndex = isLeft ? formIndex - 1 : formIndex + 1;

        if (isLeft && formIndex == 0)
            return false;

        if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->size() - 1)) {
            CheckAside(curSPhPosCmp, wc, model, nextCompIndex, nextFormIndex, isLeft, curPhrStatus, curSimplePhrInd);
        } else {
            if (m_collection.empty() || wc->textForm != m_collection.back()->textForm) {
                m_collection.push_back(std::make_shared<WordComplex>(*wc));
            }

            if (wordComp->isRec() && ((isLeft && formIndex > 0) || (!isLeft && formIndex < m_sentence.size() - 1))) {
                if (CheckAside(curSPhPosCmp, wc, model, compIndex, nextFormIndex, isLeft, curPhrStatus,
                               curSimplePhrInd)) {
                    return true;
                } else {
                    Logger::log("Recursive checkAside", LogLevel::Debug, "Failed, stop recursive.");
                    return false;
                }
            }
        }
    }
    // If the component is a ModelComp
    else if (auto modelComp = std::dynamic_pointer_cast<ModelComp>(comp)) {
        if (curSimplePhrInd > m_simplePhrases.size())
            return false;

        for (size_t smpPhrOffset = 0; smpPhrOffset < m_simplePhrases.size(); smpPhrOffset++) {

            const auto asidePhrase = m_simplePhrases[smpPhrOffset];

            if (ShouldSkip(smpPhrOffset, curSimplePhrInd, isLeft, wc, modelComp)) {
                continue;
            }

            if (!curPhrStatus.headIsChecked) {
                if (modelComp->isHead()) {
                    if (modelComp->getHead()->getCondition().check(modelComp->getHead()->getSPTag(),
                                                                   m_sentence[formIndex + *modelComp->getHeadPos()])) {
                        curPhrStatus.headIsChecked = true;
                        curPhrStatus.headIsMatched = true;
                    } else {
                        Logger::log("CheckAside", LogLevel::Debug, "check failed.");
                        return false;
                    }
                }
            }

            const auto curSimplePhr = m_simplePhrases[curSimplePhrInd];

            if (!curPhrStatus.foundLex) {
                for (size_t offset = 0; offset < curSimplePhr->words.size(); offset++) {
                    for (const auto& morphForm : m_sentence[formIndex + offset]->getMorphInfo()) {
                        if (!modelComp->getCondition().getAdditional().check(morphForm)) {
                            return false;
                        } else {
                            curPhrStatus.foundLex = true;
                        }
                    }
                }
            }

            size_t nextFormIndex = isLeft ? formIndex - 1 : formIndex + 1;

            if (isLeft && asidePhrase->pos.end == formIndex) {
                AddWordsToFront(wc, asidePhrase);
                UpdatePhraseStatus(wc, asidePhrase, curPhrStatus, true);
            } else if (asidePhrase->pos.start == formIndex) {
                AddWordsToBack(wc, asidePhrase);
                UpdatePhraseStatus(wc, asidePhrase, curPhrStatus, false);
            }

            if (curSPhPosCmp != 0 && wc->pos.start != 0 && curSimplePhr->pos.start - 1 == nextFormIndex) {
                if (CheckAside(curSPhPosCmp, wc, model, curSPhPosCmp - 1, curSimplePhr->pos.start - 1, isLeft,
                               curPhrStatus, smpPhrOffset))
                    break;
            }
            if (curSPhPosCmp != model->size() - 1 && curSimplePhr->pos.end + 1 == nextFormIndex) {
                if (CheckAside(curSPhPosCmp, wc, model, curSPhPosCmp + 1, curSimplePhr->pos.end + 1, isLeft,
                               curPhrStatus, smpPhrOffset))
                    break;
            }

            if (curPhrStatus.foundLex && curPhrStatus.headIsChecked && curPhrStatus.headIsMatched &&
                compIndex == model->size() - 1 && curPhrStatus.correct >= model->size()) {
                if (m_collection.empty() || wc->textForm != m_collection.back()->textForm) {
                    m_collection.push_back(std::make_shared<WordComplex>(*wc));
                }
            }
        }
    }
    return false;
}

bool ComplexPhrasesCollector::ProcessModelComponent(const std::shared_ptr<Model>& model,
                                                    const WordComplexPtr& curSimplePhr, const size_t curSimplePhrInd,
                                                    CurrentPhraseStatus& curPhrStatus, WordComplexPtr& wc)
{
    auto curSPhPosCmp = model->getModelCompIndByForm(curSimplePhr->modelName);
    if (!curSPhPosCmp)
        return false;

    if (!CheckCurrentSimplePhrase(curSimplePhr, model->getModelComponent(*curSPhPosCmp), curPhrStatus))
        return false;

    wc = InicializeWordComplex(curSimplePhr, model->getForm());
    curPhrStatus.correct++;

    if (*curSPhPosCmp != 0 && wc->pos.start != 0) {
        if (CheckAside(*curSPhPosCmp, wc, model, *curSPhPosCmp - 1, curSimplePhr->pos.start - 1, true, curPhrStatus,
                       curSimplePhrInd))
            return true;
    }
    if (*curSPhPosCmp != model->size() - 1) {
        if (CheckAside(*curSPhPosCmp, wc, model, *curSPhPosCmp + 1, curSimplePhr->pos.end + 1, false, curPhrStatus,
                       curSimplePhrInd))
            return true;
    }
    return false;
}

void ComplexPhrasesCollector::Collect(Process& process)
{
    for (size_t curSimplePhrInd = 0; curSimplePhrInd < m_simplePhrases.size(); curSimplePhrInd++) {
        const auto curSimplePhr = m_simplePhrases[curSimplePhrInd];

        LogCurrentSimplePhrase(curSimplePhr);

        for (const auto& [name, model] : manager.getComplexPatterns()) {
            LogCurrentComplexModel(name);

            CurrentPhraseStatus curPhrStatus;

            WordComplexPtr wc;
            if (ProcessModelComponent(model, curSimplePhr, curSimplePhrInd, curPhrStatus, wc))
                break;
        }
    }

    // PatternPhrasesStorage::GetStorage().threadController.reachCheckpoint();
    PatternPhrasesStorage::GetStorage().AddWordComplexes(m_collection);
    // PatternPhrasesStorage::GetStorage().threadController.waitForCheckpoint();
    OutputResults(m_collection, process);
}

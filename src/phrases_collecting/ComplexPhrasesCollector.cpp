#include <ComplexPhrasesCollector.h>
#include <PatternPhrasesStorage.h>

using namespace PhrasesCollectorUtils;

bool ComplexPhrasesCollector::CheckMorphologicalTags(const std::unordered_set<MorphInfo>& morphForms,
                                                     const Condition& cond, CurrentPhraseStatus& curPhrStatus)
{
    for (const auto& morphForm : morphForms) {
        if (!cond.morphTagCheck(morphForm)) {
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
    auto& options = PhrasesCollectorUtils::Options::getOptions();
    auto comp = model->getComponents()[compIndex];

    // Check if the component is a WordComp
    if (auto wordComp = std::dynamic_pointer_cast<WordComp>(comp)) {
        const auto token = m_sentence[formIndex];
        const auto& stopWords = GetStopWords();

        if (options.cleanStopWords) {
            if (stopWords.find(token->getWordForm().toLowerCase().getRawString()) != stopWords.end())
                return false;

            const auto normalForm = GetLemma(token);
            if (stopWords.find(normalForm) != stopWords.end())
                return false;
        }

        if (MorphAnanlysisError(token) || !HaveSp(token->getMorphInfo()))
            return false;

        std::string formFromText = token->getWordForm().getRawString();

        if (!wordComp->getCondition().check(wordComp->getSPTag(), token)) {
            return false;
        } else {
            if (!curPhrStatus.headIsChecked) {
                curPhrStatus.headIsChecked = true;
                curPhrStatus.headIsMatched = true;
            }
        }

        UpdateWordComplex(wc, token, formFromText, isLeft);

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

bool isMatchingPattern(const std::string& str)
{
    // Define a regular expression pattern that matches strings of the form (*+*) + * + (*+*)
    std::regex pattern(R"(\(.*\)\s*\+\s*.*\s*\+\s*\(.*\))");
    return std::regex_match(str, pattern);
}

void ComplexPhrasesCollector::ValidateBoundares()
{
    if (m_collection.empty()) {
        return;
    }

    // A new container to store only the valid elements after validation
    std::vector<PHUtils::WordComplexPtr> validatedCollection;

    for (const auto& it : m_collection) {
        if (it == nullptr) {
            continue;
        }

        bool isNested = false;

        const auto s = it->pos.start;
        const auto e = it->pos.end;

        for (const auto& innerIt : m_collection) {
            if (innerIt == it) {
                continue;
            }

            const auto& innerWC = innerIt;

            // Mark the current element as nested if its start position is the same as the inner element's start, and
            // its end position is less than or equal to the inner element's end position (indicating that the current
            // element is within the inner one)
            if (innerWC->pos.start == s && innerWC->pos.end >= e) {
                isNested = true;
                break;
            }
        }

        // If the current element is not nested within another element, add it to the validated collection
        if (!isNested) {
            validatedCollection.push_back(it);
        }
    }

    // Replace the old collection with the new collection containing only valid elements
    m_collection = std::move(validatedCollection);
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

        for (const auto& [name, model] : manager.getComplexPatterns()) {
            CurrentPhraseStatus curPhrStatus;
            WordComplexPtr wc;
            if (ProcessModelComponent(model, curSimplePhr, curSimplePhrInd, curPhrStatus, wc))
                break;
        }
    }

    auto& options = PhrasesCollectorUtils::Options::getOptions();
    if (options.validateBoundaries) {
        ValidateBoundares();
    }

    OutputResults(m_collection, process);
}

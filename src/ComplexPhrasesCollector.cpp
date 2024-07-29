#include <ComplexPhrasesCollector.h>

bool ComplexPhrasesCollector::CheckCurrentSimplePhrase(const WordComplexPtr& curSimplePhr,
                                                       const std::shared_ptr<ModelComp>& curModelComp,
                                                       CurrentPhraseStatus& curPhrStatus)
{
    bool simplePhrMorph = false;
    bool simplePhrAddCond = false;

    const auto& baseAddCond = curModelComp->getCondition().getAdditional();
    if (baseAddCond.empty()) {
        simplePhrAddCond = true;
    } // Index to track word components within the base model component
    size_t wcInd = 0;
    for (const auto& wordComp : curModelComp->getComponents()) {
        // Try to cast each component to a WordComp and check conditions
        if (const auto& wc = std::dynamic_pointer_cast<WordComp>(wordComp)) {
            // Check morphological tags for each form associated with the head
            for (const auto& morphForm : m_sentence[curSimplePhr->pos.start + wcInd++]->getMorphInfo()) {
                const auto& baseCond = curModelComp->getHead()->getCondition();
                if (!baseCond.morphTagCheck(morphForm)) {
                    Logger::log("check", LogLevel::Debug, "morphTagCheck failed.");
                    continue;
                } else {
                    simplePhrMorph = true;
                    // Check if the word component is the head of the model
                    if (wc->isHead()) {
                        curPhrStatus.headIsChecked = true;
                        curPhrStatus.headIsMatched = simplePhrMorph;
                    }

                    if (baseAddCond.exLexCheck(morphForm)) {
                        curPhrStatus.foundLex = true;
                    }

                    if (baseAddCond.themesCheck()) {
                        curPhrStatus.foundTheme = true;
                    }

                    if (simplePhrMorph && curPhrStatus.foundLex && curPhrStatus.foundTheme) {
                        break;
                    }
                }
            }
            if (!simplePhrMorph)
                continue;
            else
                break;
        }
    }

    if (curPhrStatus.headIsChecked && !curPhrStatus.headIsMatched) {
        return false;
    }

    if (!simplePhrAddCond)
        return false;

    return true;
}

static void updateWordComplex(const std::shared_ptr<WordComplex>& wc, const WordFormPtr& form,
                              const std::string& formFromText, bool isLeft)
{
    if (isLeft) {
        wc->words.push_front(form);
        wc->pos.start--;
        wc->textForm.insert(0, formFromText + " ");
    } else {
        wc->words.push_back(form);
        wc->pos.end++;
        wc->textForm.append(" " + formFromText);
    }
}

bool ComplexPhrasesCollector::shouldSkip(size_t smpPhrOffset, size_t curSimplePhrInd, bool isLeft,
                                         const std::shared_ptr<WordComplex>& wc, std::shared_ptr<ModelComp> modelComp)
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

bool ComplexPhrasesCollector::CheckAside(size_t curSPhPosCmp, const std::shared_ptr<WordComplex>& wc,
                                         const std::shared_ptr<Model>& model, size_t compIndex, size_t formIndex,
                                         const bool isLeft, CurrentPhraseStatus& curPhrStatus, size_t curSimplePhrInd)
{
    auto comp = model->getComponents()[compIndex];

    // Check if the component is a WordComp
    if (auto wordComp = std::dynamic_pointer_cast<WordComp>(comp)) {
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

        updateWordComplex(wc, m_sentence[formIndex], formFromText, isLeft);

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
            Logger::log("CURRENT m_simplePhrases[smpPhrOffset]", LogLevel::Debug,
                        std::to_string(smpPhrOffset) + " " + m_simplePhrases[smpPhrOffset]->textForm + " " +
                            std::to_string(m_simplePhrases[smpPhrOffset]->pos.start) + "-" +
                            std::to_string(m_simplePhrases[smpPhrOffset]->pos.end));
            Logger::log("CURRENT m_simplePhrases[curSimplePhrInd]", LogLevel::Debug,
                        std::to_string(curSimplePhrInd) + " " + m_simplePhrases[curSimplePhrInd]->textForm + " " +
                            std::to_string(m_simplePhrases[curSimplePhrInd]->pos.start) + "-" +
                            std::to_string(m_simplePhrases[curSimplePhrInd]->pos.end));

            if (shouldSkip(smpPhrOffset, curSimplePhrInd, isLeft, wc, modelComp)) {
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

            if (!curPhrStatus.foundLex || !curPhrStatus.foundTheme) {
                for (size_t offset = 0; offset < m_simplePhrases[curSimplePhrInd]->words.size(); offset++) {
                    for (const auto& morphForm : m_sentence[formIndex + offset]->getMorphInfo()) {
                        if (!modelComp->getCondition().getAdditional().check(morphForm)) {
                            return false;
                        } else {
                            curPhrStatus.foundLex = true;
                            curPhrStatus.foundTheme = true;
                        }
                    }
                }
            }

            size_t nextFormIndex = isLeft ? formIndex - 1 : formIndex + 1;
            Logger::log("CURRENT WC", LogLevel::Debug,
                        wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));

            if (isLeft && m_simplePhrases[smpPhrOffset]->pos.end == formIndex) {
                for (auto rit = m_simplePhrases[smpPhrOffset]->words.rbegin();
                     rit != m_simplePhrases[smpPhrOffset]->words.rend(); ++rit) {
                    wc->words.push_front(std::move(*rit));
                }
                curPhrStatus.correct++;
                wc->pos.start = m_simplePhrases[smpPhrOffset]->pos.start;
                wc->textForm.insert(0, m_simplePhrases[smpPhrOffset]->textForm + " ");
                Logger::log("CURRENT WC", LogLevel::Debug,
                            wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
            } else if (m_simplePhrases[smpPhrOffset]->pos.start == formIndex) {
                for (auto& elem : m_simplePhrases[smpPhrOffset]->words) {
                    wc->words.push_back(std::move(elem));
                }
                curPhrStatus.correct++;
                wc->pos.end = m_simplePhrases[smpPhrOffset]->pos.end;
                wc->textForm.append(" " + m_simplePhrases[smpPhrOffset]->textForm);
                Logger::log("CURRENT WC", LogLevel::Debug,
                            wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
            }

            if (curSPhPosCmp != 0 && wc->pos.start != 0 &&
                m_simplePhrases[curSimplePhrInd]->pos.start - 1 == nextFormIndex) {
                if (CheckAside(curSPhPosCmp, wc, model, curSPhPosCmp - 1,
                               m_simplePhrases[curSimplePhrInd]->pos.start - 1, isLeft, curPhrStatus, smpPhrOffset))
                    break;
            }
            if (curSPhPosCmp != model->size() - 1 && m_simplePhrases[curSimplePhrInd]->pos.end + 1 == nextFormIndex) {
                if (CheckAside(curSPhPosCmp, wc, model, curSPhPosCmp + 1, m_simplePhrases[curSimplePhrInd]->pos.end + 1,
                               isLeft, curPhrStatus, smpPhrOffset))
                    break;
            }

            if (curPhrStatus.foundLex && curPhrStatus.foundTheme && curPhrStatus.headIsChecked &&
                curPhrStatus.headIsMatched && compIndex == model->size() - 1 && curPhrStatus.correct >= model->size()) {
                if (m_collection.empty() || wc->textForm != m_collection.back()->textForm) {
                    m_collection.push_back(std::make_shared<WordComplex>(*wc));
                }
            }
        }
    }
    return false;
}

static WordComplexPtr InicializeWordComplex(const WordComplexPtr& curSimplePhr, const std::string& modelName)
{
    WordComplexPtr wc = std::make_shared<WordComplex>();
    wc->words = curSimplePhr->words;
    wc->textForm = curSimplePhr->textForm;
    wc->pos = curSimplePhr->pos;
    wc->modelName = modelName;
    return wc;
}

void ComplexPhrasesCollector::Collect(const std::vector<WordFormPtr>& forms, Process& process)
{
    m_sentence = forms;

    // Iterate over each simple phrase (as word complex component) provided in sentence
    for (size_t curSimplePhrInd = 0; curSimplePhrInd < m_simplePhrases.size(); curSimplePhrInd++) {
        const auto curSimplePhr = m_simplePhrases[curSimplePhrInd];

        Logger::log("CURRENT SIMPLE PHRASE", LogLevel::Info, curSimplePhr->textForm + " || " + curSimplePhr->modelName);

        // Iterate over each assembly in the Grammar Pattern Manager
        for (const auto& [name, model] : manager.getComplexPatterns()) {
            Logger::log("CURRENT COMPLEX MODEL", LogLevel::Info, name);

            // Get the simple phrase position as model component index (match with the word complex name)
            auto curSPhPosCmp = model->getModelCompIndByForm(curSimplePhr->modelName);
            if (!curSPhPosCmp)
                continue;

            CurrentPhraseStatus curPhrStatus; // TODO! if there are many themes (???)
            Logger::log("CURRENT COMPONENT", LogLevel::Info, model->getComponent(*curSPhPosCmp)->getForm());

            if (!CheckCurrentSimplePhrase(curSimplePhr, model->getModelComponent(*curSPhPosCmp), curPhrStatus))
                continue;

            WordComplexPtr wc = InicializeWordComplex(curSimplePhr, model->getForm());
            curPhrStatus.correct++;

            if (*curSPhPosCmp != 0 && wc->pos.start != 0) {
                if (CheckAside(*curSPhPosCmp, wc, model, *curSPhPosCmp - 1, curSimplePhr->pos.start - 1, true,
                               curPhrStatus, curSimplePhrInd))
                    break;
            }
            if (*curSPhPosCmp != model->size() - 1) {
                if (CheckAside(*curSPhPosCmp, wc, model, *curSPhPosCmp + 1, curSimplePhr->pos.end + 1, false,
                               curPhrStatus, curSimplePhrInd))
                    break;
            }
        }
    }

    for (const auto& wc : m_collection) {
        process.m_output << process.m_docNum << " " << process.m_sentNum << " start_ind = " << wc->pos.start
                         << " end_ind = " << wc->pos.end << "\t||\t" << wc->textForm << "\t||\t" << wc->modelName
                         << std::endl;
    }
}

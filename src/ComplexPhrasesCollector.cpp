#include <ComplexPhrasesCollector.h>

bool ComplexPhrasesCollector::CheckBase(const WordComplexPtr &base, const std::shared_ptr<ModelComp> &baseModelComp, bool &headIsMatched, bool &headIsChecked, bool &foundLex, bool &foundTheme)
{
    bool simplePhrMorph = false;
    bool simplePhrAddCond = false;

    // Check BASE
    const auto &baseAddCond = baseModelComp->getCondition().getAdditional();
    if (baseAddCond.empty())
    {
        simplePhrAddCond = true;
    } // Index to track word components within the base model component
    size_t wcInd = 0;
    for (const auto &wordCompFromBMC : baseModelComp->getComponents())
    {
        // Try to cast each component to a WordComp and check conditions
        if (const auto &wc = std::dynamic_pointer_cast<WordComp>(wordCompFromBMC))
        {
            // Check morphological tags for each form associated with the head
            for (const auto &morphForm : m_sentence[base->pos.start + wcInd++]->getMorphInfo())
            {
                const auto &baseCond = baseModelComp->getHead()->getCondition();
                if (!baseCond.morphTagCheck(morphForm))
                {
                    Logger::log("check", LogLevel::Debug, "morphTagCheck failed.");
                    continue;
                }
                else
                {
                    simplePhrMorph = true;
                    // Check if the word component is the head of the model
                    if (wc->isHead())
                    {
                        headIsChecked = true;
                        headIsMatched = simplePhrMorph;
                    }

                    if (baseAddCond.exLexCheck(morphForm))
                    {
                        foundLex = true;
                    }

                    if (baseAddCond.themesCheck())
                    {
                        foundTheme = true;
                    }

                    if (simplePhrMorph && foundLex && foundTheme)
                    {
                        break;
                    }
                }
            }
            if (!simplePhrMorph)
                continue;
            else
                break;
        }

    } // END OF Check BASE

    if (headIsChecked && !headIsMatched)
    {
        return false;
    }

    if (!simplePhrAddCond)
        return false;

    return true;
}

static void updateWordComplex(const std::shared_ptr<WordComplex> &wc, const WordFormPtr &form, const std::string &formFromText, bool isLeft)
{
    if (isLeft)
    {
        wc->words.push_front(form);
        wc->pos.start--;
        wc->textForm.insert(0, formFromText + " ");
    }
    else
    {
        wc->words.push_back(form);
        wc->pos.end++;
        wc->textForm.append(" " + formFromText);
    }
}

bool ComplexPhrasesCollector::CheckAside(size_t curSPhPosCmp, const std::shared_ptr<WordComplex> &wc,
                                         const std::shared_ptr<Model> &model, size_t compIndex, size_t formIndex,
                                         size_t &correct, const bool isLeft, bool &headIsMatched, bool &headIsChecked, bool &foundLex, bool &foundTheme, size_t curSimplePhrInd)
{
    auto comp = model->getComponents()[compIndex];

    // Check if the component is a WordComp
    if (auto wordComp = std::dynamic_pointer_cast<WordComp>(comp))
    {
        std::string formFromText = m_sentence[formIndex]->getWordForm().getRawString();
        Logger::log("CheckAside", LogLevel::Debug, "FormFromText: " + formFromText);

        if (!wordComp->getCondition().check(wordComp->getSPTag(), m_sentence[formIndex]))
        {
            Logger::log("CheckAside", LogLevel::Debug, "check failed.");
            return false;
        }
        else
        {
            if (!headIsChecked)
            {
                headIsChecked = true;
                headIsMatched = true;
            }
        }

        updateWordComplex(wc, m_sentence[formIndex], formFromText, isLeft);

        ++correct;
        size_t nextCompIndex = isLeft ? compIndex - 1 : compIndex + 1;
        size_t nextFormIndex = isLeft ? formIndex - 1 : formIndex + 1;

        if (isLeft && formIndex == 0)
            return false;

        if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->size() - 1))
        {
            CheckAside(curSPhPosCmp, wc, model, nextCompIndex, nextFormIndex,
                       correct, isLeft, headIsMatched, headIsChecked, foundLex, foundTheme, curSimplePhrInd);
        }
        else
        {
            if (m_collection.empty() || wc->textForm != m_collection.back()->textForm)
            {
                m_collection.push_back(std::make_shared<WordComplex>(*wc));
            }

            if (wordComp->isRec() && ((isLeft && formIndex > 0) || (!isLeft && formIndex < m_sentence.size() - 1)))
            {
                if (CheckAside(curSPhPosCmp, wc, model, compIndex, nextFormIndex,
                               correct, isLeft, headIsMatched, headIsChecked, foundLex, foundTheme, curSimplePhrInd))
                {
                    return true;
                }
                else
                {
                    Logger::log("Recursive checkAside", LogLevel::Debug, "Failed, stop recursive.");
                    return false;
                }
            }
        }
    }
    // If the component is a ModelComp
    else if (auto modelComp = std::dynamic_pointer_cast<ModelComp>(comp))
    {
        if (curSimplePhrInd > m_simplePhrasesCollection.size())
            return false;

        for (size_t baseWCOffset = 0; baseWCOffset < m_simplePhrasesCollection.size(); baseWCOffset++)
        {
            Logger::log("CURRENT basesWC[baseWCOffset]", LogLevel::Debug, std::to_string(baseWCOffset) + " " + m_simplePhrasesCollection[baseWCOffset]->textForm + " " + std::to_string(m_simplePhrasesCollection[baseWCOffset]->pos.start) + "-" + std::to_string(m_simplePhrasesCollection[baseWCOffset]->pos.end));
            Logger::log("CURRENT basesWC[curSimplePhrInd]", LogLevel::Debug, std::to_string(curSimplePhrInd) + " " + m_simplePhrasesCollection[curSimplePhrInd]->textForm + " " + std::to_string(m_simplePhrasesCollection[curSimplePhrInd]->pos.start) + "-" + std::to_string(m_simplePhrasesCollection[curSimplePhrInd]->pos.end));

            if (baseWCOffset > m_simplePhrasesCollection.size() || baseWCOffset < 0)
                return false;
            if (baseWCOffset == curSimplePhrInd)
                continue;
            if (isLeft && m_simplePhrasesCollection[baseWCOffset]->pos.start >= wc->pos.end) // TODO: check with time
                continue;
            if (!isLeft && m_simplePhrasesCollection[baseWCOffset]->pos.start <= wc->pos.end)
                continue;
            if (m_simplePhrasesCollection[baseWCOffset]->baseName != modelComp->getForm())
                continue;

            if (!headIsChecked)
            {
                if (modelComp->isHead())
                {
                    if (modelComp->getHead()->getCondition().check(modelComp->getHead()->getSPTag(), m_sentence[formIndex + *modelComp->getHeadPos()]))
                    {
                        headIsChecked = true;
                        headIsMatched = true;
                    }
                    else
                    {
                        Logger::log("CheckAside", LogLevel::Debug, "check failed.");
                        return false;
                    }
                }
            }

            if (!foundLex || !foundTheme)
            {
                for (size_t offset = 0; offset < m_simplePhrasesCollection[curSimplePhrInd]->words.size(); offset++)
                {
                    for (const auto &morphForm : m_sentence[formIndex + offset]->getMorphInfo())
                    {
                        if (!modelComp->getCondition().getAdditional().check(morphForm))
                        {
                            return false;
                        }
                        else
                        {
                            foundLex = true;
                            foundTheme = true;
                        }
                    }
                }
            }

            size_t nextCompIndex = isLeft ? compIndex - 1 : compIndex + 1;
            size_t nextFormIndex = isLeft ? formIndex - 1 : formIndex + 1;
            size_t nextBaseForm = isLeft ? curSimplePhrInd - 1 : curSimplePhrInd + 1;
            Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));

            if (isLeft && m_simplePhrasesCollection[baseWCOffset]->pos.end == formIndex)
            {
                for (auto rit = m_simplePhrasesCollection[baseWCOffset]->words.rbegin(); rit != m_simplePhrasesCollection[baseWCOffset]->words.rend(); ++rit)
                {
                    wc->words.push_front(std::move(*rit));
                }
                correct++;
                wc->pos.start = m_simplePhrasesCollection[baseWCOffset]->pos.start;
                wc->textForm.insert(0, m_simplePhrasesCollection[baseWCOffset]->textForm + " ");
                Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
            }
            else if (m_simplePhrasesCollection[baseWCOffset]->pos.start == formIndex)
            {
                for (auto &elem : m_simplePhrasesCollection[baseWCOffset]->words)
                {
                    wc->words.push_back(std::move(elem));
                }
                correct++;
                wc->pos.end = m_simplePhrasesCollection[baseWCOffset]->pos.end;
                wc->textForm.append(" " + m_simplePhrasesCollection[baseWCOffset]->textForm);
                Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
            }

            if (curSPhPosCmp != 0 && wc->pos.start != 0 && m_simplePhrasesCollection[curSimplePhrInd]->pos.start - 1 == nextFormIndex)
            {
                if (CheckAside(curSPhPosCmp, wc, model, curSPhPosCmp - 1, m_simplePhrasesCollection[curSimplePhrInd]->pos.start - 1, correct,
                               isLeft, headIsMatched, headIsChecked, foundLex, foundTheme, baseWCOffset))
                    break;
            }
            if (curSPhPosCmp != model->size() - 1 && m_simplePhrasesCollection[curSimplePhrInd]->pos.end + 1 == nextFormIndex)
            {
                if (CheckAside(curSPhPosCmp, wc, model, curSPhPosCmp + 1, m_simplePhrasesCollection[curSimplePhrInd]->pos.end + 1, correct,
                               isLeft, headIsMatched, headIsChecked, foundLex, foundTheme, baseWCOffset))
                    break;
            }

            if (foundLex && foundTheme && headIsChecked && headIsMatched && compIndex == model->size() - 1 && correct >= model->size())
            {
                if (m_collection.empty() || wc->textForm != m_collection.back()->textForm)
                {
                    m_collection.push_back(std::make_shared<WordComplex>(*wc));
                }
            }
        }
    }
    return false;
}

void ComplexPhrasesCollector::Collect(const std::vector<WordFormPtr> &forms, Process &process)
{
    m_sentence = forms;

    m_simplePhrasesCollection = simplePhrasesCollector.GetCollector().GetCollection();
    // const autom_simplePhrasesCollection =m_simplePhrasesCollectionCollector::GetCollector().GetCollection();

    // Iterate over each simple phrase (as word complex component) provided in sentence
    for (size_t curSimplePhrInd = 0; curSimplePhrInd < m_simplePhrasesCollection.size(); curSimplePhrInd++)
    {
        const auto curSimplePhr = m_simplePhrasesCollection[curSimplePhrInd];

        Logger::log("CURRENT SIMPLE PHRASE", LogLevel::Info, curSimplePhr->textForm + " || " + curSimplePhr->baseName);

        // Iterate over each assembly in the Grammar Pattern Manager
        for (const auto &[name, model] : manager.getComplexPatterns())
        {
            Logger::log("CURRENT COMPLEX MODEL", LogLevel::Info, name);

            // Get the simple phrase position as model component index (match with the word complex name)
            auto curSPhPosCmp = model->getModelCompIndByForm(curSimplePhr->baseName);
            if (!curSPhPosCmp)
                continue;

            size_t correct = 0;
            bool simplePhrMorph = false;
            bool simplePhrAddCond = false;
            bool headIsMatched = false;
            bool headIsChecked = false;

            // Access the component corresponding to the base index
            auto comp = model->getComponents()[*curSPhPosCmp];
            auto baseModelComp = std::dynamic_pointer_cast<ModelComp>(comp);

            Logger::log("CURRENT COMPONENT", LogLevel::Info, comp->getForm());

            bool foundLex = false;
            bool foundTheme = false; // TODO! if there are many themes (???)

            WordComplexPtr wc = std::make_shared<WordComplex>();

            if (CheckBase(curSimplePhr, baseModelComp, headIsMatched, headIsChecked, foundLex, foundTheme))
            {
                ++correct;
                wc->words = curSimplePhr->words;
                wc->textForm = curSimplePhr->textForm;
                wc->pos = curSimplePhr->pos;
                wc->baseName = model->getForm();
                Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
            }
            else
            {
                continue;
            }

            if (*curSPhPosCmp != 0 && wc->pos.start != 0)
            {
                if (CheckAside(*curSPhPosCmp, wc, model, *curSPhPosCmp - 1, curSimplePhr->pos.start - 1, correct,
                               true, headIsMatched, headIsChecked, foundLex, foundTheme, curSimplePhrInd))
                    break;
            }
            if (*curSPhPosCmp != model->size() - 1)
            {
                if (CheckAside(*curSPhPosCmp, wc, model, *curSPhPosCmp + 1, curSimplePhr->pos.end + 1, correct,
                               false, headIsMatched, headIsChecked, foundLex, foundTheme, curSimplePhrInd))
                    break;
            }
        }
    }

    for (const auto &wc : m_collection)
    {
        process.m_output << process.m_docNum << " " << process.m_sentNum << " start_ind = " << wc->pos.start << " end_ind = " << wc->pos.end << "\t||\t" << wc->textForm << "\t||\t" << wc->baseName << std::endl;
    }
}

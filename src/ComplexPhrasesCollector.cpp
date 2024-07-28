#include <ComplexPhrasesCollector.h>

static bool CheckBase(const std::vector<WordFormPtr> &forms, const WordComplexPtr &base, const std::shared_ptr<ModelComp> &baseModelComp, bool &headIsMatched, bool &headIsChecked, bool &foundLex, bool &foundTheme)
{
    bool baseMorphIsMatched = false;
    bool baseAddCondIsMatched = false;

    // Check BASE
    const auto &baseAddCond = baseModelComp->getCondition().getAdditional();
    if (baseAddCond.isEmpty())
    {
        baseAddCondIsMatched = true;
    } // Index to track word components within the base model component
    size_t wcInd = 0;
    for (const auto &wordCompFromBMC : baseModelComp->getComponents())
    {
        // Try to cast each component to a WordComp and check conditions
        if (const auto &wc = std::dynamic_pointer_cast<WordComp>(wordCompFromBMC))
        {
            // Check morphological tags for each form associated with the head
            for (const auto &morphForm : forms[base->pos.start + wcInd++]->getMorphInfo())
            {
                const auto &baseCond = baseModelComp->getHead()->getCondition();
                if (!baseCond.morphTagCheck(morphForm))
                {
                    Logger::log("ConditionsCheck", LogLevel::Debug, "morphTagCheck failed.");
                    continue;
                }
                else
                {
                    baseMorphIsMatched = true;
                    // Check if the word component is the head of the model
                    if (wc->isHead())
                    {
                        headIsChecked = true;
                        headIsMatched = baseMorphIsMatched;
                    }

                    if (baseAddCond.exLexCheck(morphForm))
                    {
                        foundLex = true;
                    }

                    if (baseAddCond.themesCheck())
                    {
                        foundTheme = true;
                    }

                    if (baseMorphIsMatched && foundLex && foundTheme)
                    {
                        break;
                    }
                }
            }
            if (!baseMorphIsMatched)
                continue;
            else
                break;
        }

    } // END OF Check BASE

    if (headIsChecked && !headIsMatched)
    {
        return false;
    }

    if (!baseAddCondIsMatched)
        return false;

    return true;
}

static bool AdditionalConditionCheck(const Condition &baseCond, const X::MorphInfo &morphForm)
{
    if (const auto &additCond = baseCond.getAdditional(); !additCond.isEmpty())
    {
        Logger::log("AdditionalConditionCheck", LogLevel::Debug, "Checking additional conditions.");
        if (!additCond.exLexCheck(morphForm))
        {
            Logger::log("AdditionalConditionCheck", LogLevel::Debug, "exLexCheck failed.");
            return false;
        }
        if (!additCond.themesCheck())
        {
            Logger::log("AdditionalConditionCheck", LogLevel::Debug, "themesCheck failed.");
            return false;
        }
    }
    return true;
}

static bool ConditionsCheck(const std::shared_ptr<WordComp> &base, const X::WordFormPtr &form)
{
    const auto &spBaseTag = base->getSPTag();
    for (const auto &morphForm : form->getMorphInfo())
    {
        Logger::log("ConditionsCheck", LogLevel::Debug, "Checking morphForm against spBaseTag.\n\tmorphForm: " + morphForm.normalForm.getRawString() + ", " + morphForm.sp.toString() + "\t\tspBaseHeadTag: " + spBaseTag.toString());
        if (morphForm.sp == spBaseTag)
        {
            const auto &baseCond = base->getCondition();
            if (!baseCond.morphTagCheck(morphForm))
            {
                Logger::log("ConditionsCheck", LogLevel::Debug, "morphTagCheck failed.");
                return false;
            }

            if (!AdditionalConditionCheck(baseCond, morphForm))
                return false;
        }
        else
        {
            Logger::log("ConditionsCheck", LogLevel::Debug, "spBaseHeadTag does not match.");
            return false;
        }
    }
    Logger::log("ConditionsCheck", LogLevel::Debug, "Exiting function, the return value is TRUE.");
    return true;
}

bool ComplexPhrasesCollector::checkAsideWithAssemDraft(const std::vector<WordComplexPtr> &basesWC, size_t basePos, const std::shared_ptr<WordComplex> &wc,
                                                       const std::shared_ptr<Model> &model, size_t compIndex,
                                                       const std::vector<WordFormPtr> &forms, size_t formIndex,
                                                       size_t &correct, const bool isLeft, bool &headIsMatched, bool &headIsChecked, bool &foundLex, bool &foundTheme, size_t baseNumFromBasesWC)
{
    auto comp = model->getComponents()[compIndex];

    // Check if the component is a WordComp
    if (auto wordComp = std::dynamic_pointer_cast<WordComp>(comp))
    {
        std::string formFromText = forms[formIndex]->getWordForm().getRawString();
        Logger::log("checkAsideWithAssemDraft", LogLevel::Debug, "FormFromText: " + formFromText);

        if (!ConditionsCheck(wordComp, forms[formIndex]))
        {
            Logger::log("checkAsideWithAssemDraft", LogLevel::Debug, "ConditionsCheck failed.");
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

        if (isLeft)
        {
            wc->words.push_front(forms[formIndex]);
            wc->pos.start--;
            wc->textForm.insert(0, formFromText + " ");
            Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
        }
        else
        {
            wc->words.push_back(forms[formIndex]);
            wc->pos.end++;
            wc->textForm.append(" " + formFromText);
            Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
        }

        ++correct;

        size_t offset = 1;
        size_t nextCompIndex = isLeft ? compIndex - offset : compIndex + offset;
        if (isLeft && formIndex == 0)
            return false;
        size_t nextFormIndex = isLeft ? formIndex - offset : formIndex + offset;

        if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->getSize() - 1))
        {
            checkAsideWithAssemDraft(basesWC, basePos, wc, model, nextCompIndex, forms, nextFormIndex,
                                     correct, isLeft, headIsMatched, headIsChecked, foundLex, foundTheme, baseNumFromBasesWC);
        }
        else
        {
            if (m_collection.empty() || wc->textForm != m_collection.back()->textForm)
            {
                m_collection.push_back(std::make_shared<WordComplex>(*wc));
            }

            if (wordComp->isRec() && ((isLeft && formIndex > 0) || (!isLeft && formIndex < forms.size() - 1)))
            {
                if (checkAsideWithAssemDraft(basesWC, basePos, wc, model, compIndex, forms, nextFormIndex,
                                             correct, isLeft, headIsMatched, headIsChecked, foundLex, foundTheme, baseNumFromBasesWC))
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
        if (baseNumFromBasesWC > basesWC.size())
            return false;

        for (size_t baseWCOffset = 0; baseWCOffset < basesWC.size(); baseWCOffset++)
        {
            Logger::log("CURRENT basesWC[baseWCOffset]", LogLevel::Debug, std::to_string(baseWCOffset) + " " + basesWC[baseWCOffset]->textForm + " " + std::to_string(basesWC[baseWCOffset]->pos.start) + "-" + std::to_string(basesWC[baseWCOffset]->pos.end));
            Logger::log("CURRENT basesWC[baseNumFromBasesWC]", LogLevel::Debug, std::to_string(baseNumFromBasesWC) + " " + basesWC[baseNumFromBasesWC]->textForm + " " + std::to_string(basesWC[baseNumFromBasesWC]->pos.start) + "-" + std::to_string(basesWC[baseNumFromBasesWC]->pos.end));

            if (baseWCOffset > basesWC.size() || baseWCOffset < 0)
                return false;
            if (baseWCOffset == baseNumFromBasesWC)
                continue;

            if (isLeft && basesWC[baseWCOffset]->pos.start >= wc->pos.end) // TODO: check with time
                continue;
            if (!isLeft && basesWC[baseWCOffset]->pos.start <= wc->pos.end)
                continue;

            Logger::log("checkAsideWithAssemDraft", LogLevel::Debug,
                        "\n\tbaseNumFromBasesWC = " + std::to_string(baseNumFromBasesWC) +
                            "\n\tbaseWCOffset = " + std::to_string(baseWCOffset) +
                            "\n\tbasesWC[baseWCOffset]->baseName = " + basesWC[baseWCOffset]->baseName +
                            "\n\tmodelComp->getForm() = " + modelComp->getForm());

            if (basesWC[baseWCOffset]->baseName != modelComp->getForm())
                continue;

            Logger::log("checkAsideWithAssemDraft", LogLevel::Debug,
                        "\n\tformIndex = " + std::to_string(formIndex) +
                            "\n\tbasesWC[baseWCOffset]->pos.start = " + std::to_string(basesWC[baseWCOffset]->pos.start) +
                            "\n\tbasesWC[baseWCOffset]->pos.end = " + std::to_string(basesWC[baseWCOffset]->pos.end));

            if (!headIsChecked)
            {
                if (modelComp->isHead())
                {
                    if (ConditionsCheck(modelComp->getHead(), forms[formIndex + *modelComp->getHeadPos()]))
                    {
                        headIsChecked = true;
                        headIsMatched = true;
                    }
                    else
                    {
                        Logger::log("checkAsideWithAssemDraft", LogLevel::Debug, "ConditionsCheck failed.");
                        return false;
                    }
                }
            }
            // chrck head and if ok then ...

            if (!foundLex || !foundTheme)
            {
                for (size_t offset = 0; offset < basesWC[baseNumFromBasesWC]->words.size(); offset++)
                {
                    for (const auto &morphForm : forms[formIndex + offset]->getMorphInfo())
                    {
                        if (!AdditionalConditionCheck(modelComp->getCondition(), morphForm))
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
            // const std::vector<WordComplexPtr> &basesWC, size_t basePos,

            size_t offset = 1;
            size_t nextCompIndex = isLeft ? compIndex - offset : compIndex + offset;
            size_t nextFormIndex = isLeft ? formIndex - offset : formIndex + offset;
            size_t nextBaseForm = isLeft ? baseNumFromBasesWC - 1 : baseNumFromBasesWC + 1;
            Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));

            if (isLeft && basesWC[baseWCOffset]->pos.end == formIndex)
            {
                for (auto rit = basesWC[baseWCOffset]->words.rbegin(); rit != basesWC[baseWCOffset]->words.rend(); ++rit)
                {
                    wc->words.push_front(std::move(*rit));
                }
                correct++;
                wc->pos.start = basesWC[baseWCOffset]->pos.start;
                wc->textForm.insert(0, basesWC[baseWCOffset]->textForm + " ");
                Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
            }
            else if (basesWC[baseWCOffset]->pos.start == formIndex)
            {
                for (auto &elem : basesWC[baseWCOffset]->words)
                {
                    wc->words.push_back(std::move(elem));
                }
                correct++;
                wc->pos.end = basesWC[baseWCOffset]->pos.end;
                wc->textForm.append(" " + basesWC[baseWCOffset]->textForm);
                Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
            }

            if (basePos != 0 && wc->pos.start != 0 && basesWC[baseNumFromBasesWC]->pos.start - 1 == nextFormIndex)
            {
                if (checkAsideWithAssemDraft(basesWC, basePos, wc, model, basePos - 1, forms, basesWC[baseNumFromBasesWC]->pos.start - 1, correct,
                                             isLeft, headIsMatched, headIsChecked, foundLex, foundTheme, baseWCOffset))
                    break;
            }
            if (basePos != model->getSize() - 1 && basesWC[baseNumFromBasesWC]->pos.end + 1 == nextFormIndex)
            {
                if (checkAsideWithAssemDraft(basesWC, basePos, wc, model, basePos + 1, forms, basesWC[baseNumFromBasesWC]->pos.end + 1, correct,
                                             isLeft, headIsMatched, headIsChecked, foundLex, foundTheme, baseWCOffset))
                    break;
            }

            if (foundLex && foundTheme && headIsChecked && headIsMatched && compIndex == model->getSize() - 1 && correct >= model->getSize())
            {
                if (m_collection.empty() || wc->textForm != m_collection.back()->textForm)
                {
                    m_collection.push_back(std::make_shared<WordComplex>(*wc));
                }
            }
        }

        // auto headWordComp = modelComp->getHead();
    }

    // Logger::log("checkAside", LogLevel::Debug, "Exiting function, the return value is TRUE.");
    return false;
}

void ComplexPhrasesCollector::Collect(const std::vector<WordFormPtr> &forms, Process &process)
{
    Logger::log("collectAssemblies", LogLevel::Debug, "Starting assembly collection process.");

    const auto &basesWC = SimplePhrasesCollector::GetCollector().GetCollection();

    Logger::log("", LogLevel::Debug, "Coolected bases:");
    int counter = 0;
    for (const auto b : basesWC)
    {
        Logger::log("", LogLevel::Debug, std::to_string(counter++) + " " + b->textForm);
    }

    // Iterate over each base word complex provided in sentence
    for (size_t baseNumFromBasesWC = 0; baseNumFromBasesWC < basesWC.size(); baseNumFromBasesWC++)
    {
        Logger::log("CURRENT BASE", LogLevel::Info, basesWC[baseNumFromBasesWC]->textForm + " || " + basesWC[baseNumFromBasesWC]->baseName);

        // Iterate over each assembly in the Grammar Pattern Manager
        for (const auto &asem : GrammarPatternManager::GetManager()->getAssemblies())
        {
            Logger::log("CURRENT ASSEMBLY", LogLevel::Info, asem.first);

            // Get the model component index that matches the base word complex name
            auto basePos = asem.second->getModelCompIndByForm(basesWC[baseNumFromBasesWC]->baseName); // todo save logic
            if (basePos)
            {
                Logger::log("Collect", LogLevel::Info, "ModelCompInd which coincides with base: " + std::to_string(*basePos));
            }
            else
            {
                Logger::log("Collect", LogLevel::Info, "No ModelComp which coincides with base.");
                continue;
            }

            size_t correct = 0;
            bool baseMorphIsMatched = false;
            bool baseAddCondIsMatched = false;
            bool headIsMatched = false;
            bool headIsChecked = false;

            // Access the component corresponding to the base index
            auto comp = asem.second->getComponents()[*basePos];

            if (asem.second->getForm() == "(Прил + С) + Предл + (Прил + С)")
            {
                std::cout << "gocha" << std::endl;
            }

            Logger::log("CURRENT COMP", LogLevel::Info, comp->getForm());

            std::cout << comp->getForm();
            // Cast the component to a ModelComp pointer
            auto baseModelComp = std::dynamic_pointer_cast<ModelComp>(comp);

            Logger::log("CURRENT baseModelComp", LogLevel::Debug, baseModelComp->getForm());

            bool foundLex = false;
            bool foundTheme = false; // TODO! if there are many themes (???)

            WordComplexPtr wc = std::make_shared<WordComplex>();

            if (CheckBase(forms, basesWC[baseNumFromBasesWC], baseModelComp, headIsMatched, headIsChecked, foundLex, foundTheme))
            {
                ++correct;
                wc->words = basesWC[baseNumFromBasesWC]->words;
                wc->textForm = basesWC[baseNumFromBasesWC]->textForm;
                wc->pos = basesWC[baseNumFromBasesWC]->pos;
                wc->baseName = asem.second->getForm();
                Logger::log("CURRENT WC", LogLevel::Debug, wc->textForm + " " + std::to_string(wc->pos.start) + "-" + std::to_string(wc->pos.end));
            }
            else
            {
                Logger::log("CheckBase failed", LogLevel::Debug, "");

                continue;
            }

            if (*basePos != 0 && wc->pos.start != 0)
            {
                if (checkAsideWithAssemDraft(basesWC, *basePos, wc, asem.second, *basePos - 1, forms, basesWC[baseNumFromBasesWC]->pos.start - 1, correct,
                                             true, headIsMatched, headIsChecked, foundLex, foundTheme, baseNumFromBasesWC))
                    break;
            }
            if (*basePos != asem.second->getSize() - 1)
            {
                if (checkAsideWithAssemDraft(basesWC, *basePos, wc, asem.second, *basePos + 1, forms, basesWC[baseNumFromBasesWC]->pos.end + 1, correct,
                                             false, headIsMatched, headIsChecked, foundLex, foundTheme, baseNumFromBasesWC))
                    break;
            }
        }
    }
    Logger::log("Collect", LogLevel::Debug, "Added WordComplexes to matched collection.");
    for (const auto &wc : m_collection)
    {
        process.m_output << process.m_docNum << " " << process.m_sentNum << " start_ind = " << wc->pos.start << " end_ind = " << wc->pos.end << "\t||\t" << wc->textForm << "\t||\t" << wc->baseName << std::endl;
    }
}

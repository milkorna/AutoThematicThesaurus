#include <SimplePhrasesCollector.h>

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

static bool HeadCheck(const std::shared_ptr<Model> &baseModel, const X::WordFormPtr &form)
{
    if (!ConditionsCheck(baseModel->getHead(), form))
    {
        Logger::log("HeadCheck", LogLevel::Debug, "ConditionsCheck returned false.");
        return false;
    }
    Logger::log("HeadCheck", LogLevel::Debug, "Exiting function, the return value is TRUE.");
    return true;
}

static bool HaveSpHead(const std::unordered_set<X::MorphInfo> &currFormMorphInfo)
{
    for (const auto &morphForm : currFormMorphInfo)
    {
        Logger::log("HaveSpHead", LogLevel::Debug, "MorphForm: " + morphForm.normalForm.getRawString());
        const auto &spSet = GrammarPatternManager::GetManager()->getUsedHeadSp();
        if (spSet.find(morphForm.sp.toString()) == spSet.end())
        {
            Logger::log("HaveSpHead", LogLevel::Debug, "No head with " + morphForm.sp.toString() + " speach of word");
        }
        else
        {
            Logger::log("HaveSpHead", LogLevel::Debug, "Found head with " + morphForm.sp.toString() + " speach of word");
            return true;
        }
    }
    return false;
}

static bool CheckForMisclassifications(const X::WordFormPtr &form)
{
    std::unordered_set<char> punctuation = {'!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~'};
    const auto str = form->getWordForm().getRawString();

    for (char c : str)
    {
        if (!std::isdigit(c) && punctuation.find(c) == punctuation.end())
        {
            return false;
        }
    }
    return true;
}

bool SimplePhrasesCollector::CheckAside(const std::shared_ptr<WordComplex> &wc, const std::shared_ptr<Model> &model, size_t compIndex, const std::vector<WordFormPtr> &forms, size_t formIndex, size_t &correct, const bool isLeft)
{
    const auto &comp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[compIndex]);
    std::string formFromText = forms[formIndex]->getWordForm().getRawString();
    Logger::log("CheckAside", LogLevel::Debug, "FormFromText: " + formFromText);

    if (CheckForMisclassifications(forms[formIndex]))
        return false;

    if (!ConditionsCheck(comp, forms[formIndex]))
    {
        Logger::log("CheckAside", LogLevel::Debug, "ConditionsCheck failed.");
        return false;
    }
    // also need to update found themes and lex

    // add exception check like 2 = adj

    if (isLeft)
    {
        wc->words.push_front(forms[formIndex]);
        wc->pos.start--;
        wc->textForm.insert(0, formFromText + " ");
    }
    else
    {
        wc->words.push_back(forms[formIndex]);
        wc->pos.end++;
        wc->textForm.append(" " + formFromText);
    }

    ++correct;
    size_t offset = 1;
    size_t nextCompIndex = isLeft ? compIndex - offset : compIndex + offset;
    size_t nextFormIndex = isLeft ? formIndex - offset : formIndex + offset;

    if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->getSize() - 1))
    {
        CheckAside(wc, model, nextCompIndex, forms, nextFormIndex, correct, isLeft);
    }
    else
    {
        m_collection.push_back(std::make_shared<WordComplex>(*wc));
        if (comp->isRec() && ((isLeft && formIndex > 0) || (!isLeft && formIndex < forms.size() - 1)))
        {
            if (CheckAside(wc, model, compIndex, forms, nextFormIndex, correct, isLeft))
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

    // Logger::log("checkAside", LogLevel::Debug, "Exiting function, the return value is TRUE.");
    return false;
}

void SimplePhrasesCollector::Collect(const std::vector<WordFormPtr> &forms, Process &process)
{
    Logger::log("Collect", LogLevel::Debug, "Starting base collection process.");
    const auto &manager = GrammarPatternManager::GetManager();
    const auto &collection = SimplePhrasesCollector::GetCollector();
    const auto &bases = manager->getBases();

    WordComplexAgregate wcAgregate;

    // std::string agregateForm;

    // WordComplexCollection collectedBases;
    for (size_t currFormInd = 0; currFormInd < forms.size(); currFormInd++)
    {
        Logger::log("Collect", LogLevel::Debug, "Processing form index: " + std::to_string(currFormInd));
        if (!HaveSpHead(forms[currFormInd]->getMorphInfo()))
        {
            continue;
        }

        for (const auto &base : bases)
        {
            Logger::log("Collect", LogLevel::Debug, "Current base: " + base.second->getForm());

            size_t correct = 0;
            bool headIsMatched = HeadCheck(base.second, forms[currFormInd]);
            if (!headIsMatched)
                continue;
            size_t headPos = *base.second->getHeadPos(); // TODO: make save logic
            ++correct;
            WordComplexPtr wc = std::make_shared<WordComplex>();
            wc->words.push_back(forms[currFormInd]);
            wc->textForm = forms[currFormInd]->getWordForm().getRawString();
            wc->pos = {currFormInd,
                       currFormInd,
                       process.m_docNum,
                       process.m_sentNum};
            wc->baseName = base.second->getForm();

            if (headPos != 0 && currFormInd != 0)
            {
                if (CheckAside(wc, base.second, headPos - 1, forms, currFormInd - 1, correct, true))

                    break;
            }
            if (headPos != base.second->getSize() - 1)
            {
                if (CheckAside(wc, base.second, headPos + 1, forms, currFormInd + 1, correct, false))

                    break;
            }
            // add to collection with base.second.form key
        }
    }
    Logger::log("Collect", LogLevel::Debug, "Added WordComplexes to matched collection.");
    for (const auto &wc : m_collection)
    {
        process.m_output << process.m_docNum << " " << process.m_sentNum << " start_ind = " << wc->pos.start << " end_ind = " << wc->pos.end << "\t||\t" << wc->textForm << "\t||\t" << wc->baseName << std::endl;
    }
}

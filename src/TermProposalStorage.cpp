// TermDictionary.cpp
#include <TermProposalStorage.h>
#include <GrammarPatternManager.h>
#include <Logger.h>
#include <memory>
#include <deque>

WCModelCollection *WCModelCollection::instance = nullptr;

WCModelCollection *WCModelCollection::getInstance()
{
    if (!instance)
    {
        instance = new WCModelCollection();
    }
    return instance;
}

// void WCModelCollection::addModel(const std::string &key)
// {
//     dictionary[key] = {};
// }

void WCModelCollection::addWordComplex(const std::string &key, const WordComplex &wc)
{
    // Check if the key exists in the dictionary
    if (dictionary.find(key) == dictionary.end())
    {
        // If the key does not exist, initialize it with an empty WordComplexCollection
        dictionary[key] = WordComplexCollection();
    }

    // Check if there are any aggregates already, and if not, create one
    if (dictionary[key].empty())
    {
        WordComplexAgregate newAggregate;
        newAggregate.wordComplexes.push_back(wc);
        // newAggregate.amount = 1;
        newAggregate.size = wc.words.size();
        // newAggregate.normalizedForm = wc.textForm; // some logic to normalize form from text !
        //  Assume other initializations as needed

        //        dictionary[key].push_back(newAggregate); !!
    }
    else
    {
        // If aggregates already exist, add to the first one or based on some logic

        // dictionary[key][0].wordComplexes.push_back(wc); !!

        // dictionary[key][0].amount += 1; // TODO: add logic to find needed word complex
    }
}

static bool ConditionsCheck(const std::shared_ptr<WordComp> &base, const X::WordFormPtr &form)
{
    const auto &spBaseTag = base->getSPTag();
    for (const auto &morphForm : form->getMorphInfo())
    {
        Logger::log("ConditionsCheck", LogLevel::Debug, "Checking morphForm against spBaseTag.\n\tmorphForm: " + morphForm.normalForm.getRawString() + ", " + morphForm.sp.toString() + "\t\tspBaseHeadTag: " + spBaseTag.toString());
        if (morphForm.sp == spBaseTag)
        {
            // dont need to check if a word comp bs collectedBases works only with wordcomps
            const auto &baseCond = base->getCondition();

            if (!baseCond.morphTagCheck(morphForm))
            {
                Logger::log("ConditionsCheck", LogLevel::Debug, "morphTagCheck failed.");
                return false;
            }

            if (const auto &additCond = baseCond.getAdditional(); !additCond.isEmpty())
            {
                Logger::log("ConditionsCheck", LogLevel::Debug, "Checking additional conditions.");
                if (!additCond.exLexCheck(morphForm))
                {
                    Logger::log("ConditionsCheck", LogLevel::Debug, "exLexCheck failed.");
                    return false;
                }
                if (!additCond.themesCheck())
                {
                    Logger::log("ConditionsCheck", LogLevel::Debug, "themesCheck failed.");
                    return false;
                }
            }
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

static bool haveSpHead(const std::unordered_set<X::MorphInfo> &currFormMorphInfo)
{
    for (const auto &morphForm : currFormMorphInfo)
    {
        Logger::log("haveSpHead", LogLevel::Debug, "MorphForm: " + morphForm.normalForm.getRawString());
        const auto &spSet = GrammarPatternManager::getInstance()->getUsedHeadSp();
        if (spSet.find(morphForm.sp.toString()) == spSet.end())
        {
            Logger::log("haveSpHead", LogLevel::Debug, "No head with " + morphForm.sp.toString() + " speach of word");
        }
        else
        {
            Logger::log("haveSpHead", LogLevel::Debug, "Found head with " + morphForm.sp.toString() + " speach of word");
            return true;
        }
    }
    return false;
}

static bool haveSp(const std::unordered_set<X::MorphInfo> &currFormMorphInfo)
{
    for (const auto &morphForm : currFormMorphInfo)
    {
        Logger::log("haveSp", LogLevel::Debug, "MorphForm: " + morphForm.normalForm.getRawString());
        const auto &spSet = GrammarPatternManager::getInstance()->getUsedSp();
        if (spSet.find(morphForm.sp.toString()) == spSet.end())
        {
            Logger::log("haveSp", LogLevel::Debug, "No word component with " + morphForm.sp.toString() + " speach of word");
        }
        else
        {
            Logger::log("haveSp", LogLevel::Debug, "Found word component with " + morphForm.sp.toString() + " speach of word");
            return true;
        }
    }
    return false;
}

static bool checkAside(std::vector<WordComplexPtr> &matchedWordComplexes, const std::shared_ptr<WordComplex> &wc, const std::shared_ptr<Model> &model, size_t compIndex, const std::vector<WordFormPtr> &forms, size_t formIndex, size_t &correct, bool isLeft)
{
    const auto &comp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[compIndex]);
    std::string formFromText = forms[formIndex]->getWordForm().getRawString();
    Logger::log("checkAside", LogLevel::Debug, "FormFromText: " + formFromText);

    if (!ConditionsCheck(comp, forms[formIndex]))
    {
        Logger::log("checkAside", LogLevel::Debug, "ConditionsCheck failed.");
        return false;
    }

    if (isLeft)
    {
        wc->words.push_front(forms[formIndex]);
        wc->textForm.insert(0, forms[formIndex]->getWordForm().getRawString() + " ");
    }
    else
    {
        wc->words.push_back(forms[formIndex]);
        wc->textForm.append(" " + forms[formIndex]->getWordForm().getRawString());
    }

    ++correct;
    size_t offset = 1;
    size_t nextCompIndex = isLeft ? compIndex - offset : compIndex + offset;
    size_t nextFormIndex = isLeft ? formIndex - offset : formIndex + offset;

    if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->getSize() - 1))
    {
        checkAside(matchedWordComplexes, wc, model, nextCompIndex, forms, nextFormIndex, correct, isLeft);
    }
    else
    {
        matchedWordComplexes.push_back(std::make_shared<WordComplex>(*wc));
        if (comp->isRec() && ((isLeft && formIndex > 0) || (!isLeft && formIndex < forms.size() - 1)))
        {
            if (checkAside(matchedWordComplexes, wc, model, compIndex, forms, nextFormIndex, correct, isLeft))
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

    Logger::log("checkAside", LogLevel::Debug, "Exiting function, the return value is TRUE.");
    return false;
}

std::vector<WordComplexPtr> WCModelCollection::collectBases(const std::vector<WordFormPtr> &forms, std::ostream &output)
{
    Logger::log("collectBases", LogLevel::Debug, "Starting base collection process.");
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &wcCollection = WCModelCollection::getInstance();
    const auto &bases = manager->getBases();

    WordComplexAgregate wcAgregate;

    std::vector<WordComplexPtr> matchedWordComplexes;
    // std::string agregateForm;

    // WordComplexCollection collectedBases;
    for (size_t currFormInd = 0; currFormInd < forms.size(); currFormInd++)
    {
        Logger::log("collectBases", LogLevel::Debug, "Processing form index: " + std::to_string(currFormInd));
        if (!haveSpHead(forms[currFormInd]->getMorphInfo()))
        {
            continue;
        }

        for (const auto &base : bases)
        {
            Logger::log("collectBases", LogLevel::Debug, "Current base: " + base.second->getForm());

            size_t correct = 0;
            bool headIsMatched = HeadCheck(base.second, forms[currFormInd]);
            if (headIsMatched)
            {
                size_t headPos = base.second->getHeadPos();
                ++correct;
                WordComplexPtr wc = std::make_shared<WordComplex>();
                wc->words.push_back(forms[currFormInd]);
                wc->textForm = forms[currFormInd]->getWordForm().getRawString();
                wc->pos.start = currFormInd; // TODO: deal with pos
                wc->pos.end = currFormInd;

                if (headPos != 0 && currFormInd != 0)
                {
                    size_t offset = 1;
                    if (checkAside(matchedWordComplexes, wc, base.second, headPos - offset, forms, currFormInd - offset, correct, true))
                        break;
                }
                if (headPos != base.second->getSize() - 1)
                {
                    size_t offset = 1;
                    if (checkAside(matchedWordComplexes, wc, base.second, headPos + offset, forms, currFormInd + offset, correct, false))
                        break;
                }
                // add to wcCollection with base.second.form key
            }
            else
            {
                continue;
            }
        }
    }
    Logger::log("collectBases", LogLevel::Debug, "Added WordComplexes to matched collection.");
    for (const auto &wc : matchedWordComplexes)
    {
        output << wc->textForm << std::endl;
    }
    return matchedWordComplexes;
}

void WCModelCollection::collectAssemblies(const std::vector<WordFormPtr> &forms, const std::vector<WordComplexPtr> &baseInfos)
{
    Logger::log("collectAssemblies", LogLevel::Debug, "Starting assembly collection process.");
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &asems = manager->getAssemblies(); // instead of asems a set of asems where are bases

    for (size_t currFormInd = 0; currFormInd < forms.size(); currFormInd++)
    {
        for (const auto &asem : asems)
        {
            for (size_t baseInd = 0; baseInd < baseInfos.size(); baseInd++)
            {
            }
        }
    }
}

void WCModelCollection::collect(const std::vector<WordFormPtr> &forms, std::ostream &output)
{
    Logger::log("collect", LogLevel::Debug, "Starting collection process.");
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &baseInfos = this->collectBases(forms, output);
    Logger::log("collect", LogLevel::Debug, "Completed collection process.");

    this->collectAssemblies(forms, baseInfos);
}
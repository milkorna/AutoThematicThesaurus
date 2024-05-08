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

static bool ConditionsCheck(const std::shared_ptr<WordComp> &baseHead, const X::WordFormPtr &form)
{
    const auto &spBaseHeadTag = baseHead->getSPTag();
    for (const auto &morphForm : form->getMorphInfo())
    {
        Logger::log("ConditionsCheck", LogLevel::Debug, "Checking morphForm against spBaseHeadTag.\n\tmorphForm: " + morphForm.normalForm.getRawString() + ", " + morphForm.sp.toString() + "\t\tspBaseHeadTag: " + spBaseHeadTag.toString());
        if (morphForm.sp == spBaseHeadTag)
        {
            // dont need to check if a word comp bs collectedBases works only with wordcomps
            const auto &baseHeadCond = baseHead->getCondition();

            if (!baseHeadCond.morphTagCheck(morphForm))
            {
                Logger::log("ConditionsCheck", LogLevel::Debug, "morphTagCheck failed.");
                return false;
            }

            if (const auto &additCond = baseHeadCond.getAdditional(); !additCond.isEmpty())
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

static bool LeftCheck(const std::shared_ptr<WordComplex> &wc, const std::shared_ptr<Model> &model, const size_t currCompInd, const std::vector<WordFormPtr> &forms, const size_t currFormInd, size_t &correct)
{
    Logger::log("LeftCheck", LogLevel::Debug, "Starting LeftCheck for component index: " + std::to_string(currFormInd));
    const auto &leftComp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[currCompInd]);

    std::string formFromText = forms[currFormInd]->getWordForm().getRawString();
    Logger::log("LeftCheck", LogLevel::Debug, "FormFromText: " + formFromText);

    if (!ConditionsCheck(leftComp, forms[currFormInd]))
    {
        Logger::log("LeftCheck", LogLevel::Debug, "ConditionsCheck failed for leftComp.");
        return false;
    }

    if (leftComp->isRec())
    {
        if (LeftCheck(wc, model, currCompInd, forms, currFormInd - 1, correct))
        {
            wc->words.push_front(forms[currFormInd - 1]);
            //++correct;
            Logger::log("Recursive LeftCheck", LogLevel::Debug, "Succeeded, added form to front.");
        }
        else
        {
            Logger::log("Recursive LeftCheck", LogLevel::Debug, "Failed, stop recursive.");
            // return false;  TODO: fix
        }
    }
    Logger::log("LeftCheck", LogLevel::Debug, "Exiting function, the return value is TRUE.");

    return true;
}

static bool RightCheck(const std::shared_ptr<WordComplex> &wc, const std::shared_ptr<Model> &model, const size_t currCompInd, const std::vector<WordFormPtr> &forms, const size_t currFormInd, size_t &correct)
{
    Logger::log("RightCheck", LogLevel::Debug, "Starting RightCheck for component index: " + std::to_string(currFormInd));
    const auto &rightComp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[currCompInd]);

    std::string formFromText = forms[currFormInd]->getWordForm().getRawString();
    Logger::log("RightCheck", LogLevel::Debug, "FormFromText: " + formFromText);

    if (!ConditionsCheck(rightComp, forms[currFormInd]))
    {
        Logger::log("RightCheck", LogLevel::Debug, "ConditionsCheck failed for rightComp.");
        return false;
    }
    if (rightComp->isRec())
    {
        if (RightCheck(wc, model, currCompInd, forms, currFormInd + 1))
        {
            wc->words.push_back(forms[currFormInd + 1]);
            Logger::log("Recursive RightCheck", LogLevel::Debug, "Succeeded, added form to back.");
        }
        else
        {
            Logger::log("Recursive RightCheck", LogLevel::Debug, "Failed, stop recursive.");
            // return false; TODO: fix
        }
    }
    Logger::log("RightCheck", LogLevel::Debug, "Exiting function, the return value is TRUE.");

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

std::vector<WordComplexPtr> WCModelCollection::collectBases(const std::vector<WordFormPtr> &forms)
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
                if (headPos != 0)
                {
                    if (!haveSp(forms[currFormInd - 1]->getMorphInfo()))
                    {
                        break;
                    }
                }
                else
                {
                    if (headPos != base.second->getSize() - 1)
                    {
                        if (!haveSp(forms[currFormInd + 1]->getMorphInfo()))
                        {
                            break;
                        }
                    }
                }
                ++correct;
                WordComplexPtr wc = std::make_shared<WordComplex>();
                wc->words.push_back(forms[currFormInd]);
                std::string formFromText = forms[currFormInd]->getWordForm().getRawString();
                // Logger::log("collectBases", LogLevel::Debug, "FormFromText: " + formFromText);
                // Logger::log("collectBases", LogLevel::Debug, "HeadPos: " + std::to_string(headPos));
                if (headPos != 0)
                {
                    size_t offset = 1;
                    while (LeftCheck(wc, base.second, headPos - offset, forms, currFormInd - offset, correct))
                    {
                        wc->words.push_front(forms[currFormInd - offset]);
                        formFromText.insert(0, forms[currFormInd - offset]->getWordForm().getRawString());
                        wc->pos.start = currFormInd - offset;
                        ++correct;
                        ++offset;
                    }
                }
                if (headPos != base.second->getSize() - 1)
                {
                    size_t offset = 1;
                    while (RightCheck(wc, base.second, headPos + offset, forms, currFormInd + offset, correct))
                    {
                        wc->words.push_back(forms[currFormInd + offset]);
                        formFromText.append(forms[currFormInd + offset]->getWordForm().getRawString());
                        wc->pos.end = currFormInd + offset;
                        ++correct;
                        ++offset;
                    }
                }

                // add to wcCollection with base.second.form key
                if (correct > base.second->getSize())
                {
                    matchedWordComplexes.push_back(wc);
                }
            }
            else
            {
                continue;
            }

            // if (base.second->checkComponentsMatch(forms[currFormInd]))
            // {
            //     // collectedBases.push_back(base);
            // }
        }
    }
    Logger::log("collectBases", LogLevel::Debug, "Added WordComplex to matched collection.");
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

void WCModelCollection::collect(const std::vector<WordFormPtr> &forms)
{
    Logger::log("collect", LogLevel::Debug, "Starting collection process.");
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &baseInfos = this->collectBases(forms);
    Logger::log("collect", LogLevel::Debug, "Completed collection process.");

    this->collectAssemblies(forms, baseInfos);
}
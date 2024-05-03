// TermDictionary.cpp
#include <TermProposalStorage.h>
#include <GrammarPatternManager.h>
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
        if (morphForm.sp == spBaseHeadTag)
        {
            // dont need to check if a word comp bs collectedBases works only with wordcomps
            const auto &baseHeadCond = baseHead->getCondition();

            if (!baseHeadCond.morphTagCheck(morphForm))
            {
                return false;
            }

            if (const auto &additCond = baseHeadCond.getAdditional(); !additCond.isEmpty())
            {
                if (!additCond.exLexCheck(morphForm))
                {
                    return false;
                }
                if (!additCond.themesCheck())
                {
                    return false;
                }
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}

static bool HeadCheck(const std::shared_ptr<Model> &baseModel, const X::WordFormPtr &form)
{
    if (!ConditionsCheck(baseModel->getHead(), form))
    {
        return false;
    }
    return true;
}

static bool LeftCheck(const std::shared_ptr<WordComplex> &wc, const std::shared_ptr<Model> &model, const size_t currCompInd, const std::vector<WordFormPtr> &forms, const size_t currFormInd)
{
    const auto &leftComp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[currCompInd]);

    if (!ConditionsCheck(leftComp, forms[currFormInd]))
    {
        return false;
    }

    if (leftComp->isRec())
    {
        if (LeftCheck(wc, model, currCompInd, forms, currFormInd - 1))
        {
            wc->words.push_front(forms[currFormInd - 1]);
        }
    }
    return true;
}

static bool RightCheck(const std::shared_ptr<WordComplex> &wc, const std::shared_ptr<Model> &model, const size_t currCompInd, const std::vector<WordFormPtr> &forms, const size_t currFormInd)
{
    const auto &rightComp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[currCompInd]);

    if (!ConditionsCheck(rightComp, forms[currFormInd]))
    {
        return false;
    }
    if (rightComp->isRec())
    {
        if (RightCheck(wc, model, currCompInd, forms, currFormInd + 1))
        {
            wc->words.push_back(forms[currFormInd + 1]);
        }
    }
    return true;
}

std::vector<WordComplexPtr> WCModelCollection::collectBases(const std::vector<WordFormPtr> &forms)
{
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &wcCollection = WCModelCollection::getInstance();
    const auto &bases = manager->getBases();

    WordComplexAgregate wcAgregate;

    std::vector<WordComplexPtr> matchedWordComplexes;
    // std::string agregateForm;

    // WordComplexCollection collectedBases;
    for (size_t currFormInd = 0; currFormInd < forms.size(); currFormInd++)
    {
        for (const auto &base : bases)
        {
            size_t correct = 0;
            bool headIsMatched = HeadCheck(base.second, forms[currFormInd]);
            if (headIsMatched)
            {
                ++correct;
                WordComplexPtr wc = std::make_shared<WordComplex>();
                wc->words.push_back(forms[currFormInd]);
                std::string formFromText = forms[currFormInd]->getWordForm().getRawString();
                size_t headPos = base.second->getHeadPos();
                if (headPos != 0)
                {
                    if (LeftCheck(wc, base.second, headPos - 1, forms, currFormInd - 1))
                    {
                        wc->words.push_front(forms[currFormInd - 1]);
                        formFromText.insert(0, forms[currFormInd - 1]->getWordForm().getRawString());
                        wc->pos.start = currFormInd - 1;
                        ++correct;
                    }
                }
                if (headPos != base.second->getSize() - 1)
                {
                    if (RightCheck(wc, base.second, headPos + 1, forms, currFormInd + 1))
                    {
                        wc->words.push_back(forms[currFormInd + 1]);
                        formFromText.append(forms[currFormInd + 1]->getWordForm().getRawString());
                        wc->pos.end = currFormInd + 1;
                        ++correct;
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

    return matchedWordComplexes;
}

void WCModelCollection::collectAssemblies(const std::vector<WordFormPtr> &forms, const std::vector<WordComplexPtr> &baseInfos)
{
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
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &baseInfos = this->collectBases(forms);

    this->collectAssemblies(forms, baseInfos);
}
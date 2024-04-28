// TermDictionary.cpp
#include <TermProposalStorage.h>
#include <GrammarPatternManager.h>
#include <memory>

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

        dictionary[key].push_back(newAggregate);
    }
    else
    {
        // If aggregates already exist, add to the first one or based on some logic
        dictionary[key][0].wordComplexes.push_back(wc);
        // dictionary[key][0].amount += 1; // TODO: add logic to find needed word complex
    }
}

static bool CheckHead(const std::shared_ptr<Model> &baseModel, X::WordFormPtr form)
{
    const auto &baseHead = baseModel->getHead();
    const auto &spBaseHeadTag = baseHead->getSPTag();
    for (const auto &morphForm : form->getMorphInfo())
    {
        if (morphForm.sp == spBaseHeadTag)
        {

            // dont need to check if a word comp bs collectedBases works only with wordcomps
            const auto &baseHeadCond = baseHead->getCondition();
            const auto &baseHeadMorphTag = baseHeadCond.getMorphTag();
            if (baseHeadMorphTag.hasAnimacy())
            {
                if (morphForm.tag.hasAnimacy())
                {
                    if (baseHeadMorphTag.getAnimacy() == morphForm.tag.getAnimacy())
                    {
                        // animacy matched
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            // make it shorter
            // if (baseHeadMorphTag.hasNumber())
            //{
            // }
            // if (baseHeadMorphTag.hasCase())
            // {
            // }
            // if (baseHeadMorphTag.hasTense())
            // {
            // }
            // if (baseHeadMorphTag.hasCmp())
            // {
            // }
            // if (baseHeadMorphTag.hasVerbForm())
            // {
            // }
            // if (baseHeadMorphTag.hasMood())
            // {
            // }
            // if (baseHeadMorphTag.hasPerson())
            // {
            // }
            // if (baseHeadMorphTag.hasVariance())
            // {
            // }
            // if (baseHeadMorphTag.hasVoice())
            // {
            // }
            // if (baseHeadMorphTag.hasAspect())
            // {
            // }
            if (const auto &additCond = baseHeadCond.getAdditional(); !additCond.isEmpty())
            {
                if (const X::UniString exLex(additCond.m_exLex); !exLex.isEmpty())
                {
                    if (exLex == morphForm.normalForm)
                    {
                        // exLex matched
                    }
                    else
                    {
                        return false;
                    }
                }
                if (const auto &themes = additCond.m_themes; !themes.empty())
                {
                    for (const auto &theme : themes)
                    {
                        // TODO: Add logic to compare themes
                    }
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

static void CheckLeft(const std::shared_ptr<Model> &baseModel, X::WordFormPtr form)
{
}

static void CheckRight(const std::shared_ptr<Model> &baseModel, X::WordFormPtr form)
{
}

WordComplexCollection WCModelCollection::collectBases(const std::vector<WordFormPtr> &forms)
{
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &bases = manager->getBases();

    WordComplexAgregate wcAgregate;
    std::vector<WordComplex> matchedWordComplexes;
    std::string agregateForm;

    WordComplexCollection collectedBases;
    for (size_t currFormInd = 0; currFormInd < forms.size(); currFormInd++)
    {
        for (const auto &base : bases)
        {
            bool headIsMatched = CheckHead(base.second, forms[currFormInd]);
            if (headIsMatched)
            {

                size_t headPos = base.second->getHeadPos();
                if (headPos != 0)
                {
                    CheckLeft(base.second, forms[currFormInd - 1]);
                }
                if (headPos != base.second->getSize() - 1)
                {
                    CheckRight(base.second, forms[currFormInd + 1]);
                }
            }
            else
            {
                // break;
            }

            // if (base.second->checkComponentsMatch(forms[currFormInd]))
            // {
            //     // collectedBases.push_back(base);
            // }
        }
    }

    return collectedBases;
}

void WCModelCollection::collectAssemblies(const std::vector<WordFormPtr> &forms)
{
}

void WCModelCollection::collect(const std::vector<WordFormPtr> &forms)
{
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &baseInfos = this->collectBases(forms);

    const auto &asems = manager->getAssemblies();
    this->collectAssemblies(forms);
}
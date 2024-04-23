// TermDictionary.cpp
#include <TermProposalStorage.h>
#include <GrammarPatternManager.h>

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
        newAggregate.amount = 1;
        newAggregate.size = wc.words.size();
        newAggregate.normalizedForm = wc.textForm; // some logic to normalize form from text !
        // Assume other initializations as needed

        dictionary[key].push_back(newAggregate);
    }
    else
    {
        // If aggregates already exist, add to the first one or based on some logic
        dictionary[key][0].wordComplexes.push_back(wc);
        dictionary[key][0].amount += 1; // TODO: add logic to find needed word complex
    }
}

WordComplexCollection WCModelCollection::collectBases(const std::vector<WordFormPtr> &forms)
{
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &bases = manager->getBases();

    WordComplexCollection collectedBases;

    // auto matchingForms = manager->findMatchingForms(forms);

    // for (const auto &form : matchingForms)
    // {
    //     for (const auto &base : bases)
    //     {
    //         if (base.second->checkComponentsMatch(form))
    //         {
    //             collectedBases.push_back(base);
    //         }
    //     }
    // }

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
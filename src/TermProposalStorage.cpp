// TermDictionary.cpp
#include <TermProposalStorage.h>
#include <GrammarPatternManager.h>

TermPropCollector *TermPropCollector::instance = nullptr;

TermPropInfo::TermPropInfo() : count(0), documentCount(0) {}

void TermPropInfo::addOccurrence(const std::string &documentName)
{
    ++count;
    if (documents.insert(documentName).second)
    { // Returns true if the document was not already present
        ++documentCount;
    }
}

// TermDictionary::TermDictionary() {}

// TermDictionary& TermDictionary::getInstance() {
//     if (!instance) {
//         instance.reset(new TermDictionary());
//     }
//     return *instance;
// }

// void TermDictionary::addTerm(const std::string& term, const std::string& documentName) {
//     dictionary[term].addOccurrence(documentName);
// }

// const TermPropInfo* TermDictionary::getTermInfo(const std::string& term) const {
//     auto it = dictionary.find(term);
//     if (it != dictionary.end()) {
//         return &it->second;
//     }
//     return nullptr;
// }

// Implementations for TermProposal methods go here
TermProposal::TermProposal()
{
    // Constructor implementation
}

const std::string TermProposal::NormalizePhrase() const
{
    // Method implementation
    return std::string(); // Placeholder return
}

const X::WordFormPtr TermProposal::GetHead() const
{
    // Method implementation
    return nullptr; // Placeholder return
}

///////////////////////////////

TermPropCollector *TermPropCollector::getInstance()
{
    if (!instance)
    {
        instance = new TermPropCollector();
    }
    return instance;
}

void TermPropCollector::addTermProp(const std::string &term, const std::string &documentName)
{
    // dictionary[term].addOccurrence(documentName);
}

BaseInfos TermPropCollector::collectBases(const std::vector<WordFormPtr> &forms)
{
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &bases = manager->getBases();

    BaseInfos collectedBases;

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

void TermPropCollector::collectAssemblies(const std::vector<WordFormPtr> &forms)
{
}

void TermPropCollector::collect(const std::vector<WordFormPtr> &forms)
{
    const auto &manager = GrammarPatternManager::getInstance();
    const auto &baseInfos = this->collectBases(forms);

    const auto &asems = manager->getAssemblies();
    this->collectAssemblies(forms);
}
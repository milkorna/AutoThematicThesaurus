#ifndef TERMPROPOSALSTORAGE_H
#define TERMPROPOSALSTORAGE_H

#pragma once

#include <string>
#include <unordered_map>
#include <set>
#include <memory>

#include <unordered_map>
#include <xmorphy/morph/WordForm.h>

#include <GrammarComponent.h>

struct TermProposal
{
    size_t size;
    std::shared_ptr<Model> model;
    Condition m_cond;
    std::string normalizedPhrase;
    size_t m_start;
    size_t m_end;
    double m_weight;

    TermProposal();

    // Normalizes the phrase associated with the term proposal
    const std::string NormalizePhrase() const;

    // Returns the head word form of the term proposal
    const X::WordFormPtr GetHead() const; // Assuming WordFormPtr is a defined type
};

struct BaseInfo
{
    std::shared_ptr<TermProposal> termPropPtr;
    size_t sentenceStartPos;
    size_t sentenceEndPos;
};

using BaseInfos = std::vector<std::shared_ptr<BaseInfo>>;

// struct TermPropID{
//     std::string
// }

using TermProposals = std::vector<std::shared_ptr<TermProposal>>;

struct TermPropInfo
{
    size_t count;                    // Total number of occurrences across all documents
    size_t documentCount;            // Number of documents where the term appears
    std::set<std::string> documents; // Names of the documents

    TermPropInfo();

    // Records an occurrence of the term in a specific document
    void addOccurrence(const std::string &documentName);
};

class TermPropCollector
{
private:
    std::unordered_map<std::string, TermProposal> collection; //????????
    static TermPropCollector *instance;

    TermPropCollector(){};

public:
    TermPropCollector(const TermPropCollector &) = delete;
    TermPropCollector &operator=(const TermPropCollector &) = delete;

    // Singleton access method
    static TermPropCollector *getInstance();

    // Adds a term proposal along with the document name where it appeared
    void addTermProp(const std::string &term, const std::string &documentName);

    void collect(const std::vector<WordFormPtr> &forms);

    BaseInfos collectBases(const std::vector<WordFormPtr> &forms);

    void collectAssemblies(const std::vector<WordFormPtr> &forms);
};

//////////////////////////////////////////////

// class TermDictionary
// {
// private:
//     std::unordered_map<std::string, TermPropInfo> dictionary;
//     static std::unique_ptr<TermDictionary> instance;

//     // Private constructor for Singleton pattern
//     TermDictionary();

// public:
//     // Deleting copy constructor and assignment operator to prevent copies
//     TermDictionary(const TermDictionary &) = delete;
//     TermDictionary &operator=(const TermDictionary &) = delete;

//     // Singleton access method
//     static TermDictionary &getInstance();

//     // Adds a term along with the document name where it appeared
//     void addTerm(const std::string &term, const std::string &documentName);

//     // Retrieves information about a term
//     //const TermPropInfo *getTermInfo(const std::string &term) const;
// };

#endif
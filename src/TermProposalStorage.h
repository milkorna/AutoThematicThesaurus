#ifndef TERMPROPOSALSTORAGE_H
#define TERMPROPOSALSTORAGE_H

#pragma once

#include <string>
#include <unordered_map>
#include <set>
#include <memory>

#include <xmorphy/morph/WordForm.h>

#include <GrammarComponent.h>
#include "PatternParser.h"
#include <deque>
#include <Logger.h>

struct Position
{
    size_t start;
    size_t end;
    size_t docNum;
    size_t sentNum;
};

struct WordComplex
{
    std::deque<WordFormPtr> words = {};
    std::string textForm = "";
    Position pos;
};

struct WordComplexAgregate
{
    size_t size;
    std::vector<WordComplex> wordComplexes; // maybe set
    std::string form;
    Components comps;
    double m_weight;
};

using WordComplexAgregates = std::unordered_map<std::string, WordComplexAgregate>;
// string -- seq of word in normalized form
using WordComplexCollection = std::vector<WordComplexAgregates>;

using WordComplexPtr = std::shared_ptr<WordComplex>;

class WCModelCollection
{
private:
    std::unordered_map<std::string, WordComplexCollection> dictionary;
    static WCModelCollection *instance;

    // Private constructor for Singleton pattern
    WCModelCollection(){};

    void addWordComplex(const std::string &key, const WordComplex &wordComplex);

    // void addModel(const std::string &key);

public:
    // Singleton access method
    static WCModelCollection *getInstance();

    // Deleting copy constructor and assignment operator to prevent copies
    WCModelCollection(const WCModelCollection &) = delete;
    WCModelCollection &operator=(const WCModelCollection &) = delete;

    size_t size() const;

    void collect(const std::vector<WordFormPtr> &forms, Process &process);

    std::vector<WordComplexPtr> collectBases(const std::vector<WordFormPtr> &forms, Process &process);

    void collectAssemblies(const std::vector<WordFormPtr> &forms, const std::vector<WordComplexPtr> &baseInfos, Process &process);
};

#endif
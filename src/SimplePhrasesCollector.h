#ifndef SIMPLE_PHRASES_COLLECTOR_H
#define SIMPLE_PHRASES_COLLECTOR_H

#include <xmorphy/morph/WordForm.h>

#include <GrammarComponent.h>
#include <PatternParser.h>
#include <Logger.h>

#include <deque>
#include <string>
#include <unordered_map>
#include <set>
#include <memory>

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
    std::string baseName;
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

class SimplePhrasesCollector
{
public:
    static SimplePhrasesCollector &GetCollector()
    {
        static SimplePhrasesCollector instance;
        return instance;
    }

    std::vector<WordComplexPtr> GetCollection() { return m_collection; }

    void Collect(const std::vector<WordFormPtr> &forms, Process &process);

    void Clear() { m_collection.clear(); }

private:
    std::vector<WordComplexPtr> m_collection;
    std::vector<WordFormPtr> m_sentence;
    const GrammarPatternManager &manager;

    SimplePhrasesCollector() : manager(*GrammarPatternManager::GetManager()) {}
    ~SimplePhrasesCollector() {}
    SimplePhrasesCollector(const SimplePhrasesCollector &) = delete;
    SimplePhrasesCollector &operator=(const SimplePhrasesCollector &) = delete;

    bool CheckAside(const std::shared_ptr<WordComplex> &wc, const std::shared_ptr<Model> &model, size_t compIndex, size_t formIndex, size_t &correct, const bool isLeft);
};

#endif
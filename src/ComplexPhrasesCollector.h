#ifndef COMPLEX_PHRASES_COLLECTOR_H
#define COMPLEX_PHRASES_COLLECTOR_H

#include <SimplePhrasesCollector.h>

class ComplexPhrasesCollector
{
public:
    static ComplexPhrasesCollector &GetCollector()
    {
        static ComplexPhrasesCollector collector;
        return collector;
    }

    void Collect(const std::vector<WordFormPtr> &forms, Process &process);

    void Clear() { m_collection.clear(); }

private:
    std::vector<WordComplexPtr> m_collection;
    std::vector<WordFormPtr> m_sentence;
    const GrammarPatternManager &manager;
    const SimplePhrasesCollector &simplePhrasesCollector;
    std::vector<WordComplexPtr> m_simplePhrasesCollection;

    ComplexPhrasesCollector() : manager(*GrammarPatternManager::GetManager()), simplePhrasesCollector(SimplePhrasesCollector::GetCollector()) {}
    ~ComplexPhrasesCollector() {}
    ComplexPhrasesCollector(const ComplexPhrasesCollector &) = delete;
    ComplexPhrasesCollector &operator=(const ComplexPhrasesCollector &) = delete;

    bool CheckBase(const WordComplexPtr &base, const std::shared_ptr<ModelComp> &baseModelComp, bool &headIsMatched, bool &headIsChecked, bool &foundLex, bool &foundTheme);

    bool CheckAside(size_t basePos, const std::shared_ptr<WordComplex> &wc,
                    const std::shared_ptr<Model> &model, size_t compIndex, size_t formIndex,
                    size_t &correct, const bool isLeft, bool &headIsMatched, bool &headIsChecked, bool &foundLex, bool &foundTheme, size_t baseNumFromBasesWC);
};

#endif
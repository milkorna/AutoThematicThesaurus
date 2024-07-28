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

    ComplexPhrasesCollector()
    {
    }
    ~ComplexPhrasesCollector() {}
    ComplexPhrasesCollector(const ComplexPhrasesCollector &) = delete;
    ComplexPhrasesCollector &operator=(const ComplexPhrasesCollector &) = delete;

    bool CheckAside(const std::vector<WordComplexPtr> &basesWC, size_t basePos, const std::shared_ptr<WordComplex> &wc,
                                  const std::shared_ptr<Model> &model, size_t compIndex,
                                  const std::vector<WordFormPtr> &forms, size_t formIndex,
                                  size_t &correct, const bool isLeft, bool &headIsMatched, bool &headIsChecked, bool &foundLex, bool &foundTheme, size_t baseNumFromBasesWC);
};

#endif
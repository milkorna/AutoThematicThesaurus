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

private:
    ComplexPhrasesCollector() {}
    ~ComplexPhrasesCollector() {}
    ComplexPhrasesCollector(const ComplexPhrasesCollector &) = delete;
    ComplexPhrasesCollector &operator=(const ComplexPhrasesCollector &) = delete;
};

#endif
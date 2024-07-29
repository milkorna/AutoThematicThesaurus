#ifndef COMPLEX_PHRASES_COLLECTOR_H
#define COMPLEX_PHRASES_COLLECTOR_H

#include <SimplePhrasesCollector.h>

struct CurrentPhraseStatus {
    size_t correct = 0;
    bool headIsMatched = false;
    bool headIsChecked = false;
    bool foundLex = false;
    bool foundTheme = false;
};

class ComplexPhrasesCollector {
public:
    static ComplexPhrasesCollector& GetCollector()
    {
        static ComplexPhrasesCollector collector;
        return collector;
    }

    void Collect(const std::vector<WordFormPtr>& forms, Process& process);

    void Clear()
    {
        m_collection.clear();
    }

private:
    std::vector<WordComplexPtr> m_collection;
    std::vector<WordFormPtr> m_sentence;
    const GrammarPatternManager& manager;
    const std::vector<WordComplexPtr>& m_simplePhrases;

    ComplexPhrasesCollector()
        : manager(*GrammarPatternManager::GetManager()),
          m_simplePhrases(SimplePhrasesCollector::GetCollector().GetCollection())
    {
    }

    ~ComplexPhrasesCollector()
    {
    }

    ComplexPhrasesCollector(const ComplexPhrasesCollector&) = delete;
    ComplexPhrasesCollector& operator=(const ComplexPhrasesCollector&) = delete;

    bool CheckCurrentSimplePhrase(const WordComplexPtr& curSimplePhr, const std::shared_ptr<ModelComp>& curModelComp,
                                  CurrentPhraseStatus& curPhrStatus);

    bool CheckAside(size_t curSPhPosCmp, const std::shared_ptr<WordComplex>& wc, const std::shared_ptr<Model>& model,
                    size_t compIndex, size_t formIndex, const bool isLeft, CurrentPhraseStatus& curPhrStatus,
                    size_t curSimplePhrInd);

    bool ShouldSkip(size_t smpPhrOffset, size_t curSimplePhrInd, bool isLeft, const std::shared_ptr<WordComplex>& wc,
                    std::shared_ptr<ModelComp> modelComp);

    bool CheckMorphologicalTags(const std::unordered_set<MorphInfo>& morphForms, const Condition& baseCond,
                                CurrentPhraseStatus& curPhrStatus);

    bool CheckWordComponents(const WordComplexPtr& curSimplePhr, const std::shared_ptr<ModelComp>& curModelComp,
                             CurrentPhraseStatus& curPhrStatus);

    bool ProcessModelComponent(const std::shared_ptr<Model>& model, const WordComplexPtr& curSimplePhr,
                               const size_t curSimplePhrInd, CurrentPhraseStatus& curPhrStatus, WordComplexPtr& wc);
};

#endif
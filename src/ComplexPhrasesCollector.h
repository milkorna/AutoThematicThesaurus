#ifndef COMPLEX_PHRASES_COLLECTOR_H
#define COMPLEX_PHRASES_COLLECTOR_H

#include <PhrasesCollectorUtils.h>
#include <SimplePhrasesCollector.h>

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
    std::vector<PHUtils::WordComplexPtr> m_collection;
    std::vector<WordFormPtr> m_sentence;
    const GrammarPatternManager& manager;
    const std::vector<PHUtils::WordComplexPtr>& m_simplePhrases;

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

    bool CheckCurrentSimplePhrase(const PHUtils::WordComplexPtr& curSimplePhr,
                                  const std::shared_ptr<ModelComp>& curModelComp,
                                  PHUtils::CurrentPhraseStatus& curPhrStatus);

    bool CheckAside(size_t curSPhPosCmp, const PHUtils::WordComplexPtr& wc, const std::shared_ptr<Model>& model,
                    size_t compIndex, size_t formIndex, const bool isLeft, PHUtils::CurrentPhraseStatus& curPhrStatus,
                    size_t curSimplePhrInd);

    bool ShouldSkip(size_t smpPhrOffset, size_t curSimplePhrInd, bool isLeft, const PHUtils::WordComplexPtr& wc,
                    std::shared_ptr<ModelComp> modelComp);

    bool CheckMorphologicalTags(const std::unordered_set<MorphInfo>& morphForms, const Condition& baseCond,
                                PHUtils::CurrentPhraseStatus& curPhrStatus);

    bool CheckWordComponents(const PHUtils::WordComplexPtr& curSimplePhr,
                             const std::shared_ptr<ModelComp>& curModelComp,
                             PHUtils::CurrentPhraseStatus& curPhrStatus);

    bool ProcessModelComponent(const std::shared_ptr<Model>& model, const PHUtils::WordComplexPtr& curSimplePhr,
                               const size_t curSimplePhrInd, PHUtils::CurrentPhraseStatus& curPhrStatus,
                               PHUtils::WordComplexPtr& wc);
};

#endif // COMPLEX_PHRASES_COLLECTOR_H
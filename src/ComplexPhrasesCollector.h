#ifndef COMPLEX_PHRASES_COLLECTOR_H
#define COMPLEX_PHRASES_COLLECTOR_H

#include <PhrasesCollectorUtils.h>
#include <SimplePhrasesCollector.h>

class ComplexPhrasesCollector {
public:
    explicit ComplexPhrasesCollector(const std::vector<PHUtils::WordComplexPtr>& simplePhrases,
                                     const std::vector<WordFormPtr>& forms)
        : m_simplePhrases(simplePhrases), m_sentence(forms), m_collection{},
          manager(*GrammarPatternManager::GetManager())
    {
    }

    void Collect(Process& process);

    void Clear()
    {
        m_collection.clear();
    }

    ~ComplexPhrasesCollector() = default;

private:
    const std::vector<PHUtils::WordComplexPtr> m_simplePhrases;
    std::vector<PHUtils::WordComplexPtr> m_collection;
    std::vector<WordFormPtr> m_sentence;
    const GrammarPatternManager& manager;

    bool CheckCurrentSimplePhrase(const PHUtils::WordComplexPtr& curSimplePhr,
                                  const std::shared_ptr<ModelComp>& curModelComp,
                                  PHUtils::CurrentPhraseStatus& curPhrStatus);

    bool CheckAside(size_t curSPhPosCmp, const PHUtils::WordComplexPtr& wc, const std::shared_ptr<Model>& model,
                    size_t compIndex, size_t formIndex, const bool isLeft, PHUtils::CurrentPhraseStatus& curPhrStatus,
                    size_t curSimplePhrInd);

    bool ShouldSkip(size_t smpPhrOffset, size_t curSimplePhrInd, bool isLeft, const PHUtils::WordComplexPtr& wc,
                    std::shared_ptr<ModelComp> modelComp);

    bool CheckMorphologicalTags(const std::unordered_set<MorphInfo>& morphForms, const Condition& cond,
                                PHUtils::CurrentPhraseStatus& curPhrStatus);

    bool CheckWordComponents(const PHUtils::WordComplexPtr& curSimplePhr,
                             const std::shared_ptr<ModelComp>& curModelComp,
                             PHUtils::CurrentPhraseStatus& curPhrStatus);

    bool ProcessModelComponent(const std::shared_ptr<Model>& model, const PHUtils::WordComplexPtr& curSimplePhr,
                               const size_t curSimplePhrInd, PHUtils::CurrentPhraseStatus& curPhrStatus,
                               PHUtils::WordComplexPtr& wc);
};

#endif // COMPLEX_PHRASES_COLLECTOR_H
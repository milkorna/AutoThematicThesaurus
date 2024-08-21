#ifndef COMPLEX_PHRASES_COLLECTOR_H
#define COMPLEX_PHRASES_COLLECTOR_H

#include <PhrasesCollectorUtils.h>
#include <SimplePhrasesCollector.h>

#include <regex>

// \class ComplexPhrasesCollector
// \brief This class collects complex phrases from a given set of simple phrases and word forms.
//        It utilizes the GrammarPatternManager to identify and collect complex phrases based on grammar patterns.
class ComplexPhrasesCollector {
public:
    // \brief Constructor that initializes the ComplexPhrasesCollector with simple phrases and word forms.
    // \param simplePhrases     A vector of WordComplexPtr representing the simple phrases to analyze.
    // \param forms             A vector of WordFormPtr representing the sentence to analyze.
    explicit ComplexPhrasesCollector(const std::vector<PHUtils::WordComplexPtr>& simplePhrases,
                                     const std::vector<WordFormPtr>& forms)
        : m_simplePhrases(simplePhrases), m_sentence(forms), m_collection{},
          manager(*GrammarPatternManager::GetManager())
    {
    }

    // \brief Collects complex phrases from the sentence using the provided process.
    // \param process           The process used for phrase collection.
    void Collect(Process& process);

    void ValidateBoundares();

    // \brief Default destructor for the ComplexPhrasesCollector class.
    ~ComplexPhrasesCollector() = default;

private:
    const std::vector<PHUtils::WordComplexPtr> m_simplePhrases; ///< Vector of simple phrases.
    std::vector<PHUtils::WordComplexPtr> m_collection;          ///< Collection of word complexes.
    std::vector<WordFormPtr> m_sentence;                        ///< Vector of word forms representing the sentence.
    const GrammarPatternManager& manager;                       ///< Reference to the GrammarPatternManager instance.

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
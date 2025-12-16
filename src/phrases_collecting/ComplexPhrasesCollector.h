#pragma once

#include "GrammarPatternManager.h"
#include "ModelComponent.h"
#include "PhrasesCollectorUtils.h"

// \class ComplexPhrasesCollector
// \brief This class collects complex phrases from a given set of simple phrases and word forms.
//        It utilizes the GrammarPatternManager to identify and collect complex phrases based on grammar patterns.
class ComplexPhrasesCollector {
  public:
    // \brief Constructor that initializes the ComplexPhrasesCollector with simple phrases and word forms.
    // \param simplePhrases     A vector of WordComplexPtr representing the simple phrases to analyze.
    // \param forms             A vector of WordFormPtr representing the sentence to analyze.
    explicit ComplexPhrasesCollector(const std::vector<WordComplexPtr>& simplePhrases,
                                     const std::vector<X::WordFormPtr>& forms)
        : m_simplePhrases(simplePhrases), m_sentence(forms), m_collection{} {
    }

    // \brief Collects complex phrases from the sentence using the provided process.
    // \param process           The process used for phrase collection.
    void Collect(Process& process);

    void ValidateBoundares();

    // \brief Default destructor for the ComplexPhrasesCollector class.
    ~ComplexPhrasesCollector() = default;

  private:
    const std::vector<WordComplexPtr> m_simplePhrases; ///< Vector of simple phrases.
    std::vector<WordComplexPtr> m_collection;          ///< Collection of word complexes.
    std::vector<WordFormPtr> m_sentence;               ///< Vector of word forms representing the sentence.

    bool CheckCurrentSimplePhrase(const WordComplexPtr& curSimplePhr, const std::shared_ptr<ModelComp>& curModelComp,
                                  CurrentPhraseStatus& curPhrStatus);

    bool CheckAside(size_t curSPhPosCmp, const WordComplexPtr& wc, const std::shared_ptr<Model>& model,
                    size_t compIndex, size_t formIndex, const bool isLeft, CurrentPhraseStatus& curPhrStatus,
                    size_t curSimplePhrInd);

    bool ShouldSkip(size_t smpPhrOffset, size_t curSimplePhrInd, bool isLeft, const WordComplexPtr& wc,
                    std::shared_ptr<ModelComp> modelComp);

    bool CheckMorphologicalTags(const std::unordered_set<MorphInfo>& morphForms, const Condition& cond,
                                CurrentPhraseStatus& curPhrStatus);

    bool CheckWordComponents(const WordComplexPtr& curSimplePhr, const std::shared_ptr<ModelComp>& curModelComp,
                             CurrentPhraseStatus& curPhrStatus);

    bool ProcessModelComponent(const std::shared_ptr<Model>& model, const WordComplexPtr& curSimplePhr,
                               const size_t curSimplePhrInd, CurrentPhraseStatus& curPhrStatus, WordComplexPtr& wc);

    // \brief Updates the status of the current phrase based on the adjacent phrase.
    // \param wc            A shared pointer to the current word complex.
    // \param asidePhrase   A shared pointer to the adjacent phrase.
    // \param curPhrStatus  A reference to the current phrase status.
    // \param isLeft        Boolean indicating if the update is for the left side.
    void UpdatePhraseStatus(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase,
                            CurrentPhraseStatus& curPhrStatus, bool isLeft);
};

#pragma once

#include "xmorphy/morph/WordForm.h"

#include "GrammarPatternManager.h"
#include "ModelComponent.h"
#include "PhrasesCollectorUtils.h"

// \class SimplePhrasesCollector
// \brief This class collects simple phrases from a given set of word forms.
//        It utilizes the PatternPhrasesStorage to identify and collect phrases based on grammar patterns.
class SimplePhrasesCollector {
  public:
    // \brief Constructor that initializes the SimplePhrasesCollector with a vector of word forms.
    // \param forms     A vector of WordFormPtr representing the sentence to analyze.
    explicit SimplePhrasesCollector(const std::vector<WordFormPtr>& forms) : m_sentence(forms), m_collection{} {
    }

    // \brief Gets the collection of word complexes.
    // \return          A reference to the vector containing the collected word complexes.
    std::vector<WordComplexPtr>& GetCollection() {
        return m_collection;
    }

    // \brief Collects simple phrases from the sentence using the provided process.
    // \param process   The process used for phrase collection.
    void Collect(Process& process);

    // \brief Default destructor for the SimplePhrasesCollector class.
    ~SimplePhrasesCollector() = default;

  private:
    std::vector<WordComplexPtr> m_collection; ///< Collection of word complexes.
    std::vector<WordFormPtr> m_sentence;      ///< Vector of word forms representing the sentence.

    bool CheckAside(const WordComplexPtr& wc, const std::shared_ptr<Model>& model, size_t compIndex, size_t formIndex,
                    size_t& correct, const bool isLeft);
};
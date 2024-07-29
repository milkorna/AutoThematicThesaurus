#ifndef SIMPLE_PHRASES_COLLECTOR_H
#define SIMPLE_PHRASES_COLLECTOR_H

#include <xmorphy/morph/WordForm.h>

#include <GrammarPatternManager.h>
#include <ModelComponent.h>
#include <PhrasesCollectorUtils.h>

#include <unordered_map>

class SimplePhrasesCollector {
public:
    static SimplePhrasesCollector& GetCollector()
    {
        static SimplePhrasesCollector instance;
        return instance;
    }

    std::vector<PHUtils::WordComplexPtr>& GetCollection()
    {
        return m_collection;
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

    SimplePhrasesCollector() : manager(*GrammarPatternManager::GetManager())
    {
    }
    ~SimplePhrasesCollector()
    {
    }

    SimplePhrasesCollector(const SimplePhrasesCollector&) = delete;
    SimplePhrasesCollector& operator=(const SimplePhrasesCollector&) = delete;

    bool CheckAside(const PHUtils::WordComplexPtr& wc, const std::shared_ptr<Model>& model, size_t compIndex,
                    size_t formIndex, size_t& correct, const bool isLeft);
};

#endif // SIMPLE_PHRASES_COLLECTOR_H
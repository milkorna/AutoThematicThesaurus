#ifndef SIMPLE_PHRASES_COLLECTOR_H
#define SIMPLE_PHRASES_COLLECTOR_H

#include <xmorphy/morph/WordForm.h>

#include <GrammarPatternManager.h>
#include <ModelComponent.h>
#include <PhrasesCollectorUtils.h>

#include <mutex>
#include <unordered_map>

class SimplePhrasesCollector {
public:
    explicit SimplePhrasesCollector(const std::vector<WordFormPtr>& forms)
        : m_sentence(forms), m_collection{}, manager(*GrammarPatternManager::GetManager())
    {
    }

    std::vector<PHUtils::WordComplexPtr>& GetCollection()
    {
        return m_collection;
    }

    void Collect(Process& process);

    void Clear()
    {
        m_collection.clear();
    }
    ~SimplePhrasesCollector() = default;

private:
    std::vector<PHUtils::WordComplexPtr> m_collection;
    std::vector<WordFormPtr> m_sentence;
    const GrammarPatternManager& manager;
    std::mutex mutex_;

    // SimplePhrasesCollector(const SimplePhrasesCollector&) = delete;
    // SimplePhrasesCollector& operator=(const SimplePhrasesCollector&) = delete;

    bool CheckAside(const PHUtils::WordComplexPtr& wc, const std::shared_ptr<Model>& model, size_t compIndex,
                    size_t formIndex, size_t& correct, const bool isLeft);
};

#endif // SIMPLE_PHRASES_COLLECTOR_H
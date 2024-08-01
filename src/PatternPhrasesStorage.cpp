#include <PatternPhrasesStorage.h>

void PatternPhrasesStorage::Collect(const std::vector<WordFormPtr>& forms, Process& process)
{
    SimplePhrasesCollector::GetCollector().Collect(forms, process);
    ComplexPhrasesCollector::GetCollector().Collect(forms, process);

    SimplePhrasesCollector::GetCollector().Clear();
    ComplexPhrasesCollector::GetCollector().Clear();
}

void PatternPhrasesStorage::AddPhrase(const std::string& phrase)
{
    std::lock_guard<std::mutex> lock(mutex_);
    phrases.push_back(phrase);
}

const std::vector<std::string>& PatternPhrasesStorage::GetPhrases() const
{
    return phrases;
}

void PatternPhrasesStorage::AddWordComplex(const WordComplexPtr& wc)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string& key = wc->GetKey();

    auto it = clusters.find(key);
    if (it != clusters.end()) {
        auto& cluster = it->second;
        if (std::find(cluster.wordComplexes.begin(), cluster.wordComplexes.end(), wc) == cluster.wordComplexes.end()) {
            cluster.wordComplexes.push_back(wc);
        }
    } else {
        WordComplexCluster newCluster = {wc->words.size(), 1.0, key, wc->modelName, {wc}};
        clusters[key] = newCluster;
    }
}
void PatternPhrasesStorage::AddWordComplexes(const std::vector<PhrasesCollectorUtils::WordComplexPtr> collection)
{
    for (const auto& elem : collection) {
        AddWordComplex(elem);
    }
}
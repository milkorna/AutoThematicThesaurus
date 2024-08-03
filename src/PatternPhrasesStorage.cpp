#include <PatternPhrasesStorage.h>

void PatternPhrasesStorage::Collect(const std::vector<WordFormPtr>& forms, Process& process)
{
    Logger::log("PatternPhrasesStorage", LogLevel::Info, "Entering Collect method.");

    SimplePhrasesCollector simplePhrasesCollector(forms);
    simplePhrasesCollector.Collect(process);
    ComplexPhrasesCollector complexPhrasesCollector(simplePhrasesCollector.GetCollection(), forms);
    complexPhrasesCollector.Collect(process);

    Logger::log("PatternPhrasesStorage", LogLevel::Info, "Leaving Collect method.");
}

void PatternPhrasesStorage::AddPhrase(const std::string& phrase)
{
    phrases.push_back(phrase);
}

const std::vector<std::string>& PatternPhrasesStorage::GetPhrases() const
{
    return phrases;
}

void PatternPhrasesStorage::AddWordComplex(const WordComplexPtr& wc)
{
    Logger::log("AddWordComplex", LogLevel::Info, wc->GetKey());
    const std::string& key = wc->GetKey();

    if (clusters.empty() && clusters.size() == 0) {
        Logger::log("Error", LogLevel::Error, "Clusters is not initialized properly");
        return;
    }

    auto it = clusters.find(key);
    if (it != clusters.end()) {
        auto& cluster = it->second;
        if (std::find(cluster.wordComplexes.begin(), cluster.wordComplexes.end(), wc) == cluster.wordComplexes.end()) {
            cluster.wordComplexes.push_back(wc);
        }
    } else {
        WordComplexCluster newCluster = {wc->words.size(), 1.0, false, key, wc->modelName, {wc}};
        clusters[key] = newCluster;
    }
}
void PatternPhrasesStorage::AddWordComplexes(const std::vector<PhrasesCollectorUtils::WordComplexPtr> collection)
{

    for (const auto& elem : collection) {
        AddWordComplex(elem);
    }
}
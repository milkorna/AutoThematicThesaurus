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
    // std::lock_guard<std::mutex> lock(mutex_);
    phrases.push_back(phrase);
}

const std::vector<std::string>& PatternPhrasesStorage::GetPhrases() const
{
    return phrases;
}

void PatternPhrasesStorage::AddWordComplex(const WordComplexPtr& wc)
{
    Logger::log("AddWordComplex", LogLevel::Info, wc->GetKey());

    // std::lock_guard<std::mutex> lock(mutex_);
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
    // Logger::log("AddWordComplexes", LogLevel::Info, "std::lock_guard<std::mutex> lock(mutex_);");
    {
        // std::lock_guard<std::mutex> lock(mutex_);
        // Logger::log("AddWordComplexes", LogLevel::Info, "Inside mutex lock");

        // Logger::log("AddWordComplexes", LogLevel::Info, "Collection size: " + std::to_string(collection.size()));

        // Logger::log("AddWordComplexes", LogLevel::Info, "for (const auto& elem : collection)");
        for (const auto& elem : collection) {
            //  Logger::log("AddWordComplexes", LogLevel::Info, "Processing element in collection");
            AddWordComplex(elem);
            // Logger::log("AddWordComplexes", LogLevel::Info, "Processed element in collection");
        }
    }
    //    Logger::log("AddWordComplexes", LogLevel::Info, "Mutex lock released");
}
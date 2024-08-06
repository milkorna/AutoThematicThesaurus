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

// not used
void PatternPhrasesStorage::CalculateWeights()
{
    std::unordered_map<std::string, int> substringCount;

    for (const auto& [key, _] : clusters) {
        substringCount[key] = 0;
        for (const auto& [otherKey, __] : clusters) {
            if (key != otherKey && otherKey.find(key) != std::string::npos) {
                substringCount[key]++;
            }
        }
    }

    for (auto& [key, wc] : clusters) {
        int count = substringCount[key] + 1;
        if (wc.phraseSize > 2) {
            wc.m_weight /= count;
        }
    }
}

void PatternPhrasesStorage::ComputeTextMetrics()
{
    int totalDocuments = corpus.GetTotalDocuments();

    for (auto& clusterPair : clusters) {
        auto& cluster = clusterPair.second;

        cluster.tf.resize(cluster.phraseSize, 0.0);
        cluster.idf.resize(cluster.phraseSize, 0.0);
        cluster.tfidf.resize(cluster.phraseSize, 0.0);
        cluster.wordVectors.resize(cluster.phraseSize);

        for (const auto& wordComplex : cluster.wordComplexes) {
            std::unordered_map<std::string, int> termFrequency;
            for (const auto& wordForm : wordComplex->words) {
                std::string word = wordForm->getWordForm().toLowerCase().getRawString();
                boost::to_lower(word);
                termFrequency[word]++;
            }

            for (size_t i = 0; i < wordComplex->words.size(); ++i) {
                std::string word = wordComplex->words[i]->getWordForm().toLowerCase().getRawString();
                boost::to_lower(word);
                cluster.tf[i] += static_cast<double>(termFrequency[word]) / wordComplex->words.size();
                cluster.wordVectors[i] = corpus.GetWordVector(word);
            }
        }

        for (size_t i = 0; i < cluster.phraseSize; ++i) {
            std::string word = cluster.wordComplexes[0]->words[i]->getWordForm().toLowerCase().getRawString();
            boost::to_lower(word);
            cluster.idf[i] = log(static_cast<double>(totalDocuments) / (1 + corpus.GetDocumentFrequency(word)));
        }

        for (size_t i = 0; i < cluster.phraseSize; ++i) {
            cluster.tfidf[i] = cluster.tf[i] * cluster.idf[i];
        }
    }
}

// Function to output data to a file
void PatternPhrasesStorage::OutputClustersToFile(const std::string& filename) const
{
    std::ofstream outFile(filename);

    if (!outFile.is_open()) {
        throw std::runtime_error("Could not open file for writing");
    }

    std::vector<std::string> keys;
    keys.reserve(clusters.size());
    for (const auto& pair : clusters) {
        keys.push_back(pair.first);
    }

    std::sort(keys.begin(), keys.end());

    for (const auto& key : keys) {
        const auto& cluster = clusters.at(key);

        outFile << "Key: " << key << "\n"
                << "Phrase Size: " << cluster.phraseSize << "\n"
                << "Weight: " << cluster.m_weight << "\n"
                << "Topic Match: " << (cluster.topicMatch ? "true" : "false") << "\n"
                << "Model Name: " << cluster.modelName << "\n"
                << "Word Complexes: " << cluster.wordComplexes.size() << "\n"
                << "\n";

        outFile << "Phrases:\n";
        for (const auto& wordComplex : cluster.wordComplexes) {
            outFile << "  Text Form: " << wordComplex->textForm << "\n"
                    << "    Position - Start: " << wordComplex->pos.start << ", End: " << wordComplex->pos.end
                    << ", DocNum: " << wordComplex->pos.docNum << ", SentNum: " << wordComplex->pos.sentNum << "\n";
        }

        outFile << "\n";
    }

    outFile.close();
}
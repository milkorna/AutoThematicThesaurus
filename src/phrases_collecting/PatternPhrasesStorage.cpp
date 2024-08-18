#include <PatternPhrasesStorage.h>
#include <PhrasesCollectorUtils.h>

using json = nlohmann::json;

void PatternPhrasesStorage::Collect(const std::vector<WordFormPtr>& forms, Process& process)
{
    Logger::log("PatternPhrasesStorage", LogLevel::Info, "Entering Collect method.");

    std::unordered_set<std::string> wordsInSentence;
    for (const auto& form : forms) {
        std::string word = GetLemma(form);
        wordsInSentence.insert(word);
    }

    for (const auto& word1 : wordsInSentence) {
        for (const auto& word2 : wordsInSentence) {
            if (word1 != word2) {
                coOccurrenceMap[word1][word2]++;
            }
        }
    }

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
        std::vector<std::string> lemmas;
        std::vector<WordEmbeddingPtr> lemVectors;
        std::unordered_map<std::string, std::set<std::string>> lemmHypernyms;
        std::unordered_map<std::string, std::set<std::string>> lemmHyponyms;
        for (const auto& w : wc->words) {
            const auto lemma = GetLemma(w);
            lemmas.push_back(lemma);
            lemVectors.push_back(std::make_shared<WordEmbedding>(lemma));

            if (hypernymCache.find(lemma) != hypernymCache.end()) {
                lemmHypernyms[lemma] = hypernymCache[lemma];
            } else {
                auto hypernyms = semanticDB.GetRelations(lemma, "hypernym");
                hypernymCache[lemma] = hypernyms;
                lemmHypernyms[lemma] = hypernyms;
            }

            if (hyponymCache.find(lemma) != hyponymCache.end()) {
                lemmHyponyms[lemma] = hyponymCache[lemma];
            } else {
                auto hyponyms = semanticDB.GetRelations(lemma, "hyponym");
                hyponymCache[lemma] = hyponyms;
                lemmHyponyms[lemma] = hyponyms;
            }
        }

        WordComplexCluster newCluster = {wc->words.size(), 1.0,           false,       key, wc->modelName,
                                         lemmas,           {wc},          {},          {},  {},
                                         lemVectors,       lemmHypernyms, lemmHyponyms};
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

// void PatternPhrasesStorage::AddSemanticRelationsToCluster(WordComplexCluster& cluster)
// {
//     static fs::path repoPath = fs::current_path();
//     static std::string semantic_data = (repoPath / "wikiwordnet.db").string();
//     SemanticRelationsDB db(semantic_data);
//     for (const auto& wordComplex : cluster.wordComplexes) {
//         for (const auto& word : wordComplex->words) {
//             std::string textForm = word->getWordForm().toLowerCase().getRawString();
//             boost::to_lower(textForm);
//             // cluster.synonyms[textForm] = db.GetRelations(textForm, "synonym");
//             cluster.hypernyms[textForm] = db.GetRelations(textForm, "hypernym");
//             cluster.hyponyms[textForm] = db.GetRelations(textForm, "hyponym");
//         }
//     }
// }

void PatternPhrasesStorage::ComputeTextMetrics()
{
    int totalDocuments = corpus.GetTotalDocuments();

    for (auto& clusterPair : clusters) {
        auto& cluster = clusterPair.second;

        cluster.tf.resize(cluster.phraseSize, 0.0);
        cluster.idf.resize(cluster.phraseSize, 0.0);
        cluster.tfidf.resize(cluster.phraseSize, 0.0);

        for (size_t i = 0; i < cluster.phraseSize; ++i) {
            const std::string& lemma = cluster.lemmas[i];
            int termFrequency = corpus.GetWordFrequency(lemma);
            int documentFrequency = corpus.GetDocumentFrequency(lemma);

            cluster.tf[i] = static_cast<double>(termFrequency) / corpus.GetTotalWords();
            cluster.idf[i] = log(static_cast<double>(totalDocuments) / (1 + documentFrequency));
            cluster.tfidf[i] = cluster.tf[i] * cluster.idf[i];
        }
    }
}

// Function to output data to a file
void PatternPhrasesStorage::OutputClustersToTextFile(const std::string& filename) const
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
                << "Model Name: " << cluster.modelName << "\n";

        outFile << "Lemmas:\n";
        for (size_t i = 0; i < cluster.lemmas.size(); ++i) {
            outFile << "  Lemma: " << cluster.lemmas[i] << "\n"
                    << "    TF: " << cluster.tf[i] << "\n"
                    << "    IDF: " << cluster.idf[i] << "\n"
                    << "    TF-IDF: " << cluster.tfidf[i] << "\n";
        }
        outFile << "\nWord Complexes: " << cluster.wordComplexes.size() << "\n";

        outFile << "Phrases:\n";
        for (const auto& wordComplex : cluster.wordComplexes) {
            outFile << "  Text Form: " << wordComplex->textForm << "\n"
                    << "    Position - Start: " << wordComplex->pos.start << ", End: " << wordComplex->pos.end
                    << ", DocNum: " << wordComplex->pos.docNum << ", SentNum: " << wordComplex->pos.sentNum << "\n";
        }
        outFile << "\n";

        // Output hypernyms
        outFile << "Hypernyms:\n";
        for (const auto& synPair : cluster.hypernyms) {
            outFile << "  " << synPair.first << ": ";
            for (const auto& syn : synPair.second) {
                outFile << syn << " ";
            }
            outFile << "\n";
        }

        outFile << "\n";

        // Output hyponyms
        outFile << "Hyponyms:\n";
        for (const auto& synPair : cluster.hyponyms) {
            outFile << "  " << synPair.first << ": ";
            for (const auto& syn : synPair.second) {
                outFile << syn << " ";
            }
            outFile << "\n";
        }

        outFile << "\n";
    }

    outFile.close();
}

void PatternPhrasesStorage::OutputClustersToJsonFile(const std::string& filename) const
{
    json j;

    std::vector<std::string> keys;
    keys.reserve(clusters.size());
    for (const auto& pair : clusters) {
        keys.push_back(pair.first);
    }

    std::sort(keys.begin(), keys.end());

    for (const auto& key : keys) {
        const auto& cluster = clusters.at(key);

        json clusterJson;
        clusterJson["Phrase Size"] = cluster.phraseSize;
        clusterJson["Weight"] = cluster.m_weight;
        clusterJson["Topic Match"] = cluster.topicMatch;
        clusterJson["Model Name"] = cluster.modelName;

        std::vector<json> lemmasJson;
        for (size_t i = 0; i < cluster.lemmas.size(); ++i) {
            json lemmaJson;
            lemmaJson["Lemma"] = cluster.lemmas[i];
            lemmaJson["TF"] = cluster.tf[i];
            lemmaJson["IDF"] = cluster.idf[i];
            lemmaJson["TF-IDF"] = cluster.tfidf[i];
            lemmaJson["Hypernyms"] = cluster.hypernyms.at(cluster.lemmas[i]);
            lemmaJson["Hyponyms"] = cluster.hyponyms.at(cluster.lemmas[i]);

            json coOccurrencesJson = nlohmann::json::object();
            auto it1 = coOccurrenceMap.find(cluster.lemmas[i]);
            if (it1 != coOccurrenceMap.end()) {
                for (const auto& coPair : it1->second) {
                    const auto& otherLemma = coPair.first;
                    int frequency = coPair.second;
                    const auto length = static_cast<float>(otherLemma.size()) * 0.5;

                    if (g_options.cleaningStopWords) {
                        const auto& stopWords = GetStopWords();

                        if (stopWords.find(otherLemma) != stopWords.end())
                            continue;
                    }

                    if (length > 3.0 && frequency > 2) {
                        coOccurrencesJson[otherLemma] = frequency;
                    }
                }
            }
            lemmaJson["CoOccurrences"] = coOccurrencesJson;

            // std::vector<float> vectorValues = cluster.wordVectors[i]->GetVector();
            // lemmaJson["Vector"] = vectorValues;
            lemmasJson.push_back(lemmaJson);
        }
        clusterJson["Lemmas"] = lemmasJson;

        clusterJson["Word Complexes"] = cluster.wordComplexes.size();
        std::vector<json> phrases;
        for (const auto& wordComplex : cluster.wordComplexes) {
            json phraseJson;
            phraseJson["Text Form"] = wordComplex->textForm;
            phraseJson["Position"] = {{"Start", wordComplex->pos.start},
                                      {"End", wordComplex->pos.end},
                                      {"DocNum", wordComplex->pos.docNum},
                                      {"SentNum", wordComplex->pos.sentNum}};
            phrases.push_back(phraseJson);
        }
        clusterJson["Phrases"] = phrases;

        // json hypernymsJson = nlohmann::json::object();
        // for (const auto& synPair : cluster.hypernyms) {
        //     hypernymsJson[synPair.first] = synPair.second;
        // }
        // clusterJson["Hypernyms"] = hypernymsJson;

        // json hyponymsJson = nlohmann::json::object();
        // for (const auto& synPair : cluster.hyponyms) {
        //     hyponymsJson[synPair.first] = synPair.second;
        // }
        // clusterJson["Hyponyms"] = hyponymsJson;

        j[key] = clusterJson;
    }

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        throw std::runtime_error("Could not open file for writing");
    }

    outFile << j.dump(4);
    outFile.close();
}
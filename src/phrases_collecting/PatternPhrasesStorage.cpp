#include <PatternPhrasesStorage.h>
#include <PhrasesCollectorUtils.h>

#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/uscript.h>
#include <unicode/ustream.h>
#include <unicode/utypes.h>

using json = nlohmann::json;

void sortLemmasByLeadingNumber(std::deque<std::string>& lemmas)
{
    std::sort(lemmas.begin(), lemmas.end(), [](const std::string& a, const std::string& b) {
        int numA = 0, numB = 0;
        if (!a.empty() && std::isdigit(a[0])) {
            numA = a[0] - '0';
        }
        if (!b.empty() && std::isdigit(b[0])) {
            numB = b[0] - '0';
        }
        return numA < numB;
    });
}

void PatternPhrasesStorage::LoadPhraseStorage()
{
    fs::path repoPath = fs::current_path();
    fs::path outputDir = repoPath / "res";
    fs::create_directories(outputDir);
    std::vector<fs::path> res_files = GetResFiles();

    auto& corpus = TextCorpus::GetCorpus();
    clusters.reserve(corpus.GetTotalTexts() * 2);

    for (const auto& file_path : res_files) {
        std::ifstream file(file_path);

        if (!file.is_open()) {
            continue;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        if (!j.is_array()) {
            continue;
        }

        for (const auto& obj : j) {
            try {
                // Extract data from JSON
                std::string key = obj.at("0_key").get<std::string>();
                bool skip = false;
                icu::UnicodeString unicodeText = icu::UnicodeString::fromUTF8(key);
                for (int32_t i = 0; i < unicodeText.length(); ++i) {
                    UChar32 codepoint = unicodeText.char32At(i);
                    // Check if the character is a digit
                    if (u_isdigit(codepoint)) {
                        skip = true;
                    }
                }
                if (skip) {
                    continue;
                }
                std::string textForm = obj.at("1_textForm").get<std::string>();
                std::string modelName = obj.at("2_modelName").get<std::string>();
                Logger::log("PPStorage", LogLevel::Debug,
                            "Extracted key: " + key + ", textForm: " + textForm + ", modelName: " + modelName);

                Position pos;
                pos.docNum = obj.at("3_docNum").get<size_t>();
                pos.sentNum = obj.at("4_sentNum").get<size_t>();
                pos.start = obj.at("5_start_ind").get<size_t>();
                pos.end = obj.at("6_end_ind").get<size_t>();
                Logger::log("PPStorage", LogLevel::Debug,
                            "Extracted position data: start=" + std::to_string(pos.start) +
                                ", end=" + std::to_string(pos.end) + ", docNum=" + std::to_string(pos.docNum) +
                                ", sentNum=" + std::to_string(pos.sentNum));

                std::deque<std::string> lemmas;
                if (obj.contains("7_lemmas")) {
                    lemmas = obj.at("7_lemmas").get<std::deque<std::string>>();
                    sortLemmasByLeadingNumber(lemmas);
                    for (auto& lemma : lemmas) {
                        size_t pos = lemma.find('_');
                        if (pos != std::string::npos) {
                            lemma = lemma.substr(pos + 1);
                        }
                    }
                }
                Logger::log("PPStorage", LogLevel::Debug,
                            "Extracted lemmas: " + std::to_string(lemmas.size()) + " lemmas found.");

                // Create a WordComplex object
                WordComplexPtr wc = std::make_shared<WordComplex>();
                wc->textForm = textForm;
                wc->pos = pos;
                wc->modelName = modelName;
                wc->lemmas = lemmas;

                auto it = clusters.find(key);
                if (it != clusters.end()) {
                    auto& cluster = it->second;
                    if (std::find(cluster.wordComplexes.begin(), cluster.wordComplexes.end(), wc) ==
                        cluster.wordComplexes.end()) {
                        cluster.wordComplexes.push_back(wc);
                        Logger::log("PPStorage", LogLevel::Debug,
                                    "Added WordComplex to existing cluster for key: " + key);
                    }
                } else {
                    std::vector<std::string> lemmas;
                    std::vector<WordEmbeddingPtr> lemVectors;
                    std::unordered_map<std::string, std::set<std::string>> lemmHypernyms;
                    std::unordered_map<std::string, std::set<std::string>> lemmHyponyms;
                    for (const auto& lemma : wc->lemmas) {
                        lemmas.push_back(lemma);
                        lemVectors.push_back(std::make_shared<WordEmbedding>(lemma));
                        Logger::log("PPStorage", LogLevel::Debug, "Processed lemma: " + lemma);

                        if (g_options.semanticRelations) {

                            if (hypernymCache.find(lemma) != hypernymCache.end()) {
                                lemmHypernyms[lemma] = hypernymCache[lemma];
                            } else {
                                auto hypernyms = semanticDB.GetRelations(lemma, "hypernym");
                                hypernymCache[lemma] = hypernyms;
                                lemmHypernyms[lemma] = hypernyms;
                                Logger::log("PPStorage", LogLevel::Debug,
                                            "Retrieved hypernyms from DB for lemma: " + lemma);
                            }

                            if (hyponymCache.find(lemma) != hyponymCache.end()) {
                                lemmHyponyms[lemma] = hyponymCache[lemma];
                            } else {
                                auto hyponyms = semanticDB.GetRelations(lemma, "hyponym");
                                std::set<std::string> validHyponyms;
                                for (const auto& hyp : hyponyms) {
                                    const WordEmbeddingPtr& myEmbedding = std::make_shared<WordEmbedding>(hyp);
                                    const auto& topicVectors = GetTopicVectors();
                                    for (const auto& topicVecPair : topicVectors) {
                                        const std::string& topicWord = topicVecPair.first;
                                        const WordEmbeddingPtr& topicEmbedding = topicVecPair.second;

                                        float cosineSim = myEmbedding->CosineSimilarity(*topicEmbedding);
                                        if (cosineSim > g_options.topicsHyponymThreshold) {
                                            validHyponyms.insert(hyp);
                                        }
                                    }
                                }

                                hyponymCache[lemma] = validHyponyms;
                                lemmHyponyms[lemma] = validHyponyms;
                                Logger::log("PPStorage", LogLevel::Debug, "Processed hyponyms for lemma: " + lemma);
                            }
                        } else {
                            lemmHypernyms[lemma] = {};
                            lemmHyponyms[lemma] = {};
                        }
                    }

                    WordComplexCluster newCluster = {
                        wc->lemmas.size(), 1.0,           false,       key, wc->modelName, lemmas, {wc}, {}, {}, {},
                        lemVectors,        lemmHypernyms, lemmHyponyms};
                    clusters[key] = newCluster;
                    Logger::log("PPStorage", LogLevel::Debug, "Created new cluster for key: " + key);
                }
            } catch (const std::exception& e) {
                Logger::log("PPStorage", LogLevel::Error, "Error parsing JSON object: " + std::string(e.what()));
            }
        }
    }
    Logger::log("PPStorage", LogLevel::Debug, "LoadPhraseStorage completed.");
}

void PatternPhrasesStorage::Collect(const std::vector<WordFormPtr>& forms, Process& process)
{
    Logger::log("PPStorage", LogLevel::Info, "Entering Collect method.");
    auto& corpus = TextCorpus::GetCorpus();

    if (lastDocumentId != -1 && lastDocumentId != process.m_docNum) {
        for (const auto& lemma : uniqueLemmasInDoc) {
            corpus.UpdateDocumentFrequency(lemma);
        }
        uniqueLemmasInDoc.clear();
    }

    std::unordered_set<std::string> uniqueLemmasInSentence;
    for (const auto& form : forms) {
        std::string lemma = GetLemma(form);
        corpus.UpdateWordFrequency(lemma);
        uniqueLemmasInSentence.insert(lemma);
    }

    uniqueLemmasInDoc.insert(uniqueLemmasInSentence.begin(), uniqueLemmasInSentence.end());
    lastDocumentId = process.m_docNum;

    SimplePhrasesCollector simplePhrasesCollector(forms);
    simplePhrasesCollector.Collect(process);
    ComplexPhrasesCollector complexPhrasesCollector(simplePhrasesCollector.GetCollection(), forms);
    complexPhrasesCollector.Collect(process);

    Logger::log("PPStorage", LogLevel::Info, "Leaving Collect method.");
}

void PatternPhrasesStorage::FinalizeDocumentProcessing()
{
    auto& corpus = TextCorpus::GetCorpus();
    for (const auto& lemma : uniqueLemmasInDoc) {
        corpus.UpdateDocumentFrequency(lemma);
    }
    uniqueLemmasInDoc.clear();
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

            if (g_options.semanticRelations) {

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
                    std::set<std::string> validHyponyms;
                    for (const auto& hyp : hyponyms) {
                        const WordEmbeddingPtr& myEmbedding = std::make_shared<WordEmbedding>(hyp);
                        const auto& topicVectors = GetTopicVectors();
                        for (const auto& topicVecPair : topicVectors) {
                            const std::string& topicWord = topicVecPair.first;
                            const WordEmbeddingPtr& topicEmbedding = topicVecPair.second;

                            float cosineSim = myEmbedding->CosineSimilarity(*topicEmbedding);
                            if (cosineSim > g_options.topicsHyponymThreshold) {
                                validHyponyms.insert(hyp);
                            }
                        }
                    }

                    hyponymCache[lemma] = validHyponyms;
                    lemmHyponyms[lemma] = validHyponyms;
                }
            } else {
                lemmHypernyms[lemma] = {};
                lemmHyponyms[lemma] = {};
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

std::map<std::string, int>
CalculateTopicFrequency(const std::unordered_map<std::string, std::vector<std::string>>& similar_words)
{
    std::map<std::string, int> topicFrequency;
    for (const auto& pair : similar_words) {
        for (const auto& topic : pair.second) {
            topicFrequency[topic]++;
        }
    }
    return topicFrequency;
}

void ApplyTopicFrequencyPenalty(std::unordered_map<std::string, std::vector<std::string>>& similar_words,
                                int frequencyThreshold)
{
    auto topicFrequency = CalculateTopicFrequency(similar_words);
    for (auto& pair : similar_words) {
        pair.second.erase(
            std::remove_if(pair.second.begin(), pair.second.end(),
                           [&](const std::string& topic) { return topicFrequency[topic] > frequencyThreshold; }),
            pair.second.end());
    }
}

void PatternPhrasesStorage::ComputeTextMetrics()
{
    const auto corpus = TextCorpus::GetCorpus();
    int totalDocuments = corpus.GetTotalDocuments();
    const auto& topicVectors = GetTopicVectors();
    static std::unordered_map<std::string, std::vector<std::string>> totalTopics;

    for (auto& clusterPair : clusters) {
        auto& cluster = clusterPair.second;

        cluster.tf.resize(cluster.phraseSize, 0.0);
        cluster.idf.resize(cluster.phraseSize, 0.0);
        cluster.tfidf.resize(cluster.phraseSize, 0.0);

        for (size_t i = 0; i < cluster.phraseSize; ++i) {
            const std::string& lemma = cluster.lemmas[i];
            cluster.tf[i] = corpus.CalculateTF(lemma);
            cluster.idf[i] = corpus.CalculateIDF(lemma);
            cluster.tfidf[i] = cluster.tf[i] * cluster.idf[i];
        }

        const WordEmbeddingPtr& myEmbedding = std::make_shared<WordEmbedding>(cluster.key);
        std::vector<std::string> topics;
        float cosineWeight = 0.6;
        float euclideanWeight = 0.2;
        float manhattanWeight = 0.2;

        for (const auto& topicVecPair : topicVectors) {
            const std::string& topicWord = topicVecPair.first;
            const WordEmbeddingPtr& topicEmbedding = topicVecPair.second;

            float cosineSim = myEmbedding->CosineSimilarity(*topicEmbedding);
            float euclideanDist = myEmbedding->EuclideanDistance(*topicEmbedding);
            float manhattanDist = myEmbedding->ManhattanDistance(*topicEmbedding);

            float combinedScore = cosineWeight * cosineSim + euclideanWeight * (1.0f / (1.0f + euclideanDist)) +
                                  manhattanWeight * (1.0f / (1.0f + manhattanDist));

            if (combinedScore > g_options.topicsThreshold) {
                topics.push_back(topicWord);
            }
            totalTopics[cluster.key] = topics;
        }
    }
    int frequencyThreshold = static_cast<int>(clusters.size() * g_options.freqTrecholdCoeff);
    ApplyTopicFrequencyPenalty(totalTopics, frequencyThreshold);
    for (auto& clusterPair : clusters) {
        auto& cluster = clusterPair.second;
        if (const auto& iter = totalTopics.find(clusterPair.first); iter != totalTopics.end()) {
            if (iter->second.size() > 0 && iter->second.size() < g_options.upperTresholdTopicsNum) {
                cluster.topicMatch = true;
            }
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

            // json coOccurrencesJson = nlohmann::json::object();
            // auto it1 = coOccurrenceMap.find(cluster.lemmas[i]);
            // if (it1 != coOccurrenceMap.end()) {
            //     for (const auto& coPair : it1->second) {
            //         const auto& otherLemma = coPair.first;
            //         int frequency = coPair.second;
            //         const auto length = static_cast<float>(otherLemma.size()) * 0.5;

            //         if (g_options.cleaningStopWords) {
            //             const auto& stopWords = GetStopWords();

            //             if (stopWords.find(otherLemma) != stopWords.end())
            //                 continue;
            //         }

            //         if (length > 3.0 && frequency > g_options.coOccurrenceFrequency) {
            //             coOccurrencesJson[otherLemma] = frequency;
            //         }
            //     }
            // }
            // lemmaJson["CoOccurrences"] = coOccurrencesJson;

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
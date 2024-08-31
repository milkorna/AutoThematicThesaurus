#include <PatternPhrasesStorage.h>
#include <PhrasesCollectorUtils.h>

#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/uscript.h>
#include <unicode/ustream.h>
#include <unicode/utypes.h>

using json = nlohmann::json;

void PatternPhrasesStorage::LoadPhraseStorageFromResultsDir()
{
    fs::path repoPath = fs::current_path();
    fs::path outputDir = repoPath / "res";
    fs::create_directories(outputDir);
    std::vector<fs::path> res_files = GetResFiles();

    auto& corpus = TextCorpus::GetCorpus();
    clusters.reserve(corpus.GetTotalTexts());

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
                if (key.find('_') != std::string::npos) {
                    continue;
                }
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

                Position pos;
                pos.docNum = obj.at("3_docNum").get<size_t>();
                pos.sentNum = obj.at("4_sentNum").get<size_t>();
                pos.start = obj.at("5_start_ind").get<size_t>();
                pos.end = obj.at("6_end_ind").get<size_t>();

                std::deque<std::string> lemmas;
                if (obj.contains("7_lemmas")) {
                    lemmas = obj.at("7_lemmas").get<std::deque<std::string>>();
                    for (auto& lemma : lemmas) {
                        size_t pos = lemma.find('_');
                        if (pos != std::string::npos) {
                            lemma = lemma.substr(pos + 1);
                        }
                    }
                }

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
                    }
                } else {
                    std::vector<std::string> lemmas;
                    std::vector<WordEmbeddingPtr> lemVectors;
                    std::unordered_map<std::string, std::set<std::string>> lemmHypernyms;
                    std::unordered_map<std::string, std::set<std::string>> lemmHyponyms;
                    for (const auto& lemma : wc->lemmas) {
                        lemmas.push_back(lemma);
                        lemVectors.push_back(std::make_shared<WordEmbedding>(lemma));
                        lemmHypernyms[lemma] = {};
                        lemmHyponyms[lemma] = {};
                    }

                    WordComplexCluster newCluster = {
                        wc->lemmas.size(), 1.0,           false,       key, wc->modelName, lemmas, {wc}, {}, {}, {},
                        lemVectors,        lemmHypernyms, lemmHyponyms};
                    clusters[key] = newCluster;
                }
            } catch (const std::exception& e) {
                Logger::log("PPStorage", LogLevel::Error, "Error parsing JSON object: " + std::string(e.what()));
            }
        }
    }
}

void PatternPhrasesStorage::Deserialize(const json& j)
{
    try {
        if (!j.is_object()) {
            throw std::runtime_error("Expected a JSON object.");
        }

        for (auto it = j.begin(); it != j.end(); ++it) {
            std::string key = it.key();
            const json& obj = it.value();

            if (!obj.is_object()) {
                throw std::runtime_error("Expected a JSON object for each cluster.");
            }

            WordComplexCluster cluster;
            cluster.key = key;
            cluster.phraseSize = obj.at("0_phrase_size").get<size_t>();
            cluster.m_weight = obj.at("1_weight").get<double>();
            cluster.topicMatch = obj.at("2_topic_match").get<bool>();
            cluster.modelName = obj.at("3_model_name").get<std::string>();

            // Deserialize Lemmas
            const json& lemmas_json = obj.at("4_lemmas");
            for (const auto& lemma_obj : lemmas_json) {
                std::string lemmaStr = "";
                auto lemmaStrNumbered = lemma_obj.at("0_lemma").get<std::string>();
                size_t pos = lemmaStrNumbered.find('_');
                if (pos != std::string::npos) {
                    lemmaStr = lemmaStrNumbered.substr(pos + 1);
                }
                cluster.lemmas.push_back(lemmaStr);
                cluster.tf.push_back(lemma_obj.at("1_tf").get<double>());
                cluster.idf.push_back(lemma_obj.at("2_idf").get<double>());
                cluster.tfidf.push_back(lemma_obj.at("3_tf-idf").get<double>());
                cluster.hypernyms[lemmaStr] = {}; // lemma_obj.at("4_hypernyms").get<std::set<std::string>>();
                cluster.hyponyms[lemmaStr] = {};  // lemma_obj.at("5_hyponyms").get<std::set<std::string>>();

                // Add word embedding (assuming you need to create an embedding for each lemma)
                cluster.wordVectors.push_back(std::make_shared<WordEmbedding>(lemmaStr));
            }

            // Deserialize WordComplexes (Phrases in your JSON)
            const json& phrases_json = obj.at("6_phrases");
            for (const auto& phrase_obj : phrases_json) {
                WordComplexPtr wc = std::make_shared<WordComplex>();
                wc->textForm = phrase_obj.at("0_text_form").get<std::string>();
                wc->modelName = cluster.modelName;

                wc->pos.start = phrase_obj.at("1_position").at("0_start").get<size_t>();
                wc->pos.end = phrase_obj.at("1_position").at("1_end").get<size_t>();
                wc->pos.docNum = phrase_obj.at("1_position").at("2_doc_num").get<size_t>();
                wc->pos.sentNum = phrase_obj.at("1_position").at("3_sent_num").get<size_t>();

                wc->lemmas.assign(cluster.lemmas.begin(), cluster.lemmas.end());

                cluster.wordComplexes.push_back(wc);
            }

            clusters[key] = cluster;
        }
    } catch (json::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        throw;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }
}

void PatternPhrasesStorage::LoadStorageFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (file.is_open()) {
        json j;
        file >> j;
        Deserialize(j);
    } else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}

void PatternPhrasesStorage::Collect(const std::vector<WordFormPtr>& forms, Process& process)
{
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
}

void PatternPhrasesStorage::FinalizeDocumentProcessing()
{
    auto& corpus = TextCorpus::GetCorpus();
    for (const auto& lemma : uniqueLemmasInDoc) {
        corpus.UpdateDocumentFrequency(lemma);
    }
    uniqueLemmasInDoc.clear();
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

void PatternPhrasesStorage::MergeSimilarClusters()
{
    // Get all keys from the map and sort them
    std::vector<std::string> sortedKeys;
    sortedKeys.reserve(clusters.size());

    for (const auto& pair : clusters) {
        sortedKeys.push_back(pair.first);
    }

    std::sort(sortedKeys.begin(), sortedKeys.end());

    // Iterate over sorted keys and merge similar clusters
    for (size_t i = 1; i < sortedKeys.size(); ++i) {
        std::string& currentKey = sortedKeys[i];
        std::string& previousKey = sortedKeys[i - 1];

        if (AreKeysSimilar(previousKey, currentKey)) {
            // Move all wordComplexes from the current cluster to the previous cluster
            auto& previousCluster = clusters[previousKey];
            auto& currentCluster = clusters[currentKey];

            previousCluster.topicMatch = currentCluster.topicMatch || previousCluster.topicMatch;

            previousCluster.wordComplexes.insert(previousCluster.wordComplexes.end(),
                                                 currentCluster.wordComplexes.begin(),
                                                 currentCluster.wordComplexes.end());

            // Remove the current cluster after the move
            clusters.erase(currentKey);
        }
    }
}

bool PatternPhrasesStorage::AreKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff)
{
    // Split keys into words
    std::istringstream stream1(key1);
    std::istringstream stream2(key2);

    std::vector<std::string> words1, words2;
    std::string word;

    while (stream1 >> word) {
        words1.push_back(word);
    }

    while (stream2 >> word) {
        words2.push_back(word);
    }

    // If the number of words is different, the keys are not considered similar
    if (words1.size() != words2.size()) {
        return false;
    }

    size_t diffCount = 0;

    // Compare the beginnings and endings of each word
    for (size_t i = 0; i < words1.size(); ++i) {
        if ((words1[i].length() <= 8 || words2[i].length() <= 8) && words1[i] != words2[i])
            return false;
        // Extract the initial parts of the words (everything except the last two characters)
        std::string start1 = (words1[i].length() > 2) ? words1[i].substr(0, words1[i].length() - 2) : "";
        std::string start2 = (words2[i].length() > 2) ? words2[i].substr(0, words2[i].length() - 2) : "";

        if (std::abs(static_cast<long>(start1.length()) - static_cast<long>(start2.length())) >= maxDiff) {
            return false;
        }

        // Handle the difference in the lengths of the initial parts
        if (start1.length() != start2.length()) {
            if (start1.length() > start2.length()) {
                // Trim start1 to the length of start2
                start1 = start1.substr(0, start2.length());
            } else {
                // Trim start2 to the length of start1
                start2 = start2.substr(0, start1.length());
            }
        }
        if (start1.empty() || start2.empty())
            continue;

        // Compare the initial parts of the words
        if (start1 != start2) {
            return false; // If the initial parts of the words do not match after trimming, the keys are different
        }

        // Extract the endings of the words (the last two characters)
        std::string end1 = (words1[i].length() >= 2) ? words1[i].substr(words1[i].length() - 2) : words1[i];
        std::string end2 = (words2[i].length() >= 2) ? words2[i].substr(words2[i].length() - 2) : words2[i];

        // Compare the endings of the words
        if (end1 != end2) {
            ++diffCount;
            if (diffCount > maxDiff) {
                return false; // Exceeded the allowed number of differences
            }
        }
    }

    // If the number of differences in the endings does not exceed maxDiff, the keys are considered similar
    return diffCount <= maxDiff;
}

void PatternPhrasesStorage::LoadWikiWNRelations()
{
    for (auto& clusterPair : clusters) {
        WordComplexCluster& cluster = clusterPair.second;
        std::cout << cluster.key << std::endl;

        for (auto lemma : cluster.lemmas) {

            if (hypernymCache.find(lemma) != hypernymCache.end()) {
                cluster.hypernyms[lemma] = hypernymCache[lemma];
            } else {
                auto hypernyms = semanticDB.GetRelations(lemma, "hypernym");
                hypernymCache[lemma] = hypernyms;
                cluster.hypernyms[lemma] = hypernyms;
            }

            if (hyponymCache.find(lemma) != hyponymCache.end()) {
                cluster.hyponyms[lemma] = hyponymCache[lemma];
            } else {
                auto hyponyms = semanticDB.GetRelations(lemma, "hyponym");
                std::set<std::string> validHyponyms;
                if (hyponyms.size() < 150) {
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
                }
                hyponymCache[lemma] = validHyponyms;
                cluster.hyponyms[lemma] = validHyponyms;
            }
        }
    }
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
        clusterJson["0_phrase_size"] = cluster.phraseSize;
        clusterJson["1_weight"] = cluster.m_weight;
        clusterJson["2_topic_match"] = cluster.topicMatch;
        clusterJson["3_model_name"] = cluster.modelName;

        std::vector<json> lemmasJson;
        for (size_t i = 0; i < cluster.lemmas.size(); ++i) {
            json lemmaJson;
            std::string lemmaStrNumbered = std::to_string(i) + "_" + cluster.lemmas[i];
            lemmaJson["0_lemma"] = lemmaStrNumbered;
            lemmaJson["1_tf"] = cluster.tf[i];
            lemmaJson["2_idf"] = cluster.idf[i];
            lemmaJson["3_tf-idf"] = cluster.tfidf[i];
            lemmaJson["4_hypernyms"] = cluster.hypernyms.at(cluster.lemmas[i]);
            lemmaJson["5_hyponyms"] = cluster.hyponyms.at(cluster.lemmas[i]);

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
        clusterJson["4_lemmas"] = lemmasJson;

        clusterJson["5_phrases_count"] = cluster.wordComplexes.size();
        std::vector<json> phrases;
        for (const auto& wordComplex : cluster.wordComplexes) {
            json phraseJson;
            phraseJson["0_text_form"] = wordComplex->textForm;
            phraseJson["1_position"] = {{"0_start", wordComplex->pos.start},
                                        {"1_end", wordComplex->pos.end},
                                        {"2_doc_num", wordComplex->pos.docNum},
                                        {"3_sent_num", wordComplex->pos.sentNum}};
            phrases.push_back(phraseJson);
        }
        clusterJson["6_phrases"] = phrases;
        j[key] = clusterJson;
    }

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        throw std::runtime_error("Could not open file for writing");
    }

    outFile << j.dump(4);
    outFile.close();
}
#include <PatternPhrasesStorage.h>
#include <PhrasesCollectorUtils.h>

#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/uscript.h>
#include <unicode/ustream.h>
#include <unicode/utypes.h>

using json = nlohmann::json;

void PatternPhrasesStorage::AddCluster(const std::string& key, const WordComplexCluster& cluster)
{
    clusters[key] = cluster;
}

WordComplexCluster* PatternPhrasesStorage::FindCluster(const std::string& key)
{
    auto it = clusters.find(key);
    if (it != clusters.end()) {
        return &(it->second);
    }
    return nullptr;
}

void PatternPhrasesStorage::ReserveClusters(size_t count)
{
    clusters.reserve(count);
}

void PatternPhrasesStorage::AddContextsToClusters()
{
    auto& corpus = TokenizedSentenceCorpus::GetCorpus();

    for (auto& clusterPair : clusters) {
        WordComplexCluster& cluster = clusterPair.second;
        cluster.contexts.clear();

        for (const auto& wordComplex : cluster.wordComplexes) {
            const Position& pos = wordComplex->pos;
            const TokenizedSentence* sentence = corpus.GetSentence(pos.docNum, pos.sentNum);

            if (sentence) {
                cluster.contexts.push_back(*sentence);
            }
        }
    }
}

// Checks if the key of phrase1 is a prefix of phrase2's key
bool IsPrefix(const std::string& phrase1Key, const std::string& phrase2Key)
{
    return phrase2Key.find(phrase1Key) == 0; // Checks if phrase1Key is at the beginning of phrase2Key
}

// Checks exclusion conditions based on TF-IDF and frequency
bool ShouldExcludeBasedOnTfidfAndFrequency(const WordComplexCluster& phrase1, const WordComplexCluster& phrase2,
                                           const WordComplexCluster& phrase3)
{
    // Check TF-IDF conditions for phrase1 and phrase2
    bool tfidfCondition =
        !phrase1.tfidf.empty() && !phrase3.tfidf.empty() && phrase1.tfidf[0] < 0.0005 && phrase3.tfidf.back() > 0.0005;

    // Check frequency conditions for phrase1 and phrase2 compared to phrase3
    bool frequencyCondition =
        phrase1.frequency < phrase3.frequency / 10.0 && phrase2.frequency < phrase3.frequency / 10.0;

    // Return true if both conditions are met
    return tfidfCondition && frequencyCondition;
}

void WriteClustersToFile(const std::unordered_set<std::string>& clustersToInclude, const std::string& filename)
{
    // Convert the unordered set to a vector for sorting
    std::vector<std::string> sortedClusters(clustersToInclude.begin(), clustersToInclude.end());

    // Sort the clusters alphabetically
    std::sort(sortedClusters.begin(), sortedClusters.end());

    // Open the file for writing
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << std::endl;
        return;
    }

    // Write each cluster to the file
    for (const auto& cluster : sortedClusters) {
        outFile << cluster << std::endl;
    }

    // Close the file
    outFile.close();
}

std::vector<std::string> Split(const std::string& str)
{
    std::istringstream iss(str);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

static nlohmann::json LoadClassifiedPhrases(const std::string& filePath)
{
    nlohmann::json phraseLabels;
    std::ifstream jsonFile(filePath);
    if (jsonFile.is_open()) {
        jsonFile >> phraseLabels;
        jsonFile.close();
    }
    return phraseLabels;
}

void PatternPhrasesStorage::InitializeAndFilterClusters(double tfidfThreshold, std::set<std::string>& sortedKeys,
                                                        std::unordered_set<std::string>& clustersToInclude)
{
    const auto& clusters = GetClusters();
    std::regex romanNumeralsRegex(R"(^[ivxlcd]+$)", std::regex_constants::icase);

    for (const auto& pair : clusters) {
        const std::string& key = pair.first;
        const WordComplexCluster& cluster = pair.second;

        std::vector<std::string> words = Split(key);
        bool hasRomanNumerals = false;
        for (const std::string& word : words) {
            if (std::regex_match(word, romanNumeralsRegex)) {
                hasRomanNumerals = true;
                break;
            }
        }
        if (hasRomanNumerals) {
            continue;
        }

        sortedKeys.insert(key);
        clustersToInclude.insert(key);

        bool hasLowTfidf = false;
        for (double val : cluster.tfidf) {
            if (val < tfidfThreshold) {
                hasLowTfidf = true;
                break;
            }
        }

        if (hasLowTfidf) {
            if (cluster.frequency <= 1.0 && cluster.topicRelevance <= 0.4) {
                clustersToInclude.erase(key);
                sortedKeys.erase(key);
            }
        }
    }
}

void PatternPhrasesStorage::ApplyClassifiedPhrases(const nlohmann::json& phraseLabels,
                                                   std::set<std::string>& sortedKeys,
                                                   std::unordered_set<std::string>& clustersToInclude)
{
    const auto& clusters = GetClusters();

    for (const auto& phraseData : phraseLabels) {
        std::string phrase;
        std::string label;

        try {
            phrase = phraseData.at("phrase").get<std::string>();
            label = phraseData.at("label").get<std::string>();
        } catch (std::exception& e) {
            continue;
        }

        if (clusters.find(phrase) == clusters.end()) {
            continue;
        }
        const auto& cluster = clusters.at(phrase);

        if ((label == "colloquial phrase" || label == "everyday expression")) {
            if ((cluster.topicRelevance < 0.5 && !cluster.tagMatch) ||
                (label == "everyday expression" && cluster.centralityScore < 0.2)) {
                clustersToInclude.erase(phrase);
                sortedKeys.erase(phrase);
            }
        }

        if (label == "general phrase") {
            if (cluster.frequency < 0.0032 && cluster.topicRelevance <= 0.5 && cluster.centralityScore < 0.5) {
                clustersToInclude.erase(phrase);
                sortedKeys.erase(phrase);
            }
        }
    }
}

void PatternPhrasesStorage::CheckModelPrefixRelationships(std::set<std::string>& sortedKeys,
                                                          std::unordered_set<std::string>& clustersToInclude)
{
    const auto& clusters = GetClusters();

    auto it = sortedKeys.begin();
    while (it != sortedKeys.end()) {
        auto nextIt = std::next(it);
        if (nextIt == sortedKeys.end()) {
            break;
        }

        const auto& key1 = *it;
        const auto& key2 = *nextIt;

        const auto& phrase1 = clusters.at(key1);
        const auto& phrase2 = clusters.at(key2);

        bool conditionModel = ((phrase1.modelName == "Прил + С" && phrase2.modelName == "(Прил + С) + Срд") ||
                               (phrase1.modelName == "Прич + С" && phrase2.modelName == "(Прич + С) + Срд")) &&
                              IsPrefix(key1, key2);

        if (conditionModel) {
            std::size_t pos = key2.find(' ');
            if (pos != std::string::npos) {
                std::string trimmedKey = key2.substr(pos + 1);

                auto thirdIt = clusters.find(trimmedKey);
                if (thirdIt != clusters.end() && thirdIt->second.modelName == "С + Срд") {
                    const auto& phrase3 = thirdIt->second;

                    if (phrase1.centralityScore < 0.15 &&
                        ShouldExcludeBasedOnTfidfAndFrequency(phrase1, phrase2, phrase3)) {
                        clustersToInclude.erase(key1);
                        clustersToInclude.erase(key2);
                    }
                }
            }
        }

        ++it;
    }
}

void PatternPhrasesStorage::CollectTerms(double tfidfThreshold)
{
    std::set<std::string> sortedKeys;
    const auto& clusters = GetClusters();

    InitializeAndFilterClusters(tfidfThreshold, sortedKeys, clustersToInclude);

    auto phraseLabels =
        LoadClassifiedPhrases("/home/milkorna/Documents/AutoThematicThesaurus/my_data/classified_phrases.json");

    ApplyClassifiedPhrases(phraseLabels, sortedKeys, clustersToInclude);

    CheckModelPrefixRelationships(sortedKeys, clustersToInclude);

    WriteClustersToFile(clustersToInclude, "terms.txt");
}

void PatternPhrasesStorage::EvaluateTermRelevance(const LSA& lsa)
{
    const auto& topics = lsa.GetTopics();
    for (auto& [key, cluster] : clusters) {
        int relevantCount = 0;
        for (const auto& topic : topics) {
            for (const auto& word : topic.second) {
                if (std::find(cluster.lemmas.begin(), cluster.lemmas.end(), word) != cluster.lemmas.end()) {
                    relevantCount++;
                }
            }
        }
        cluster.topicRelevance = static_cast<double>(relevantCount) / topics.size();
    }
}

const std::unordered_map<std::string, WordComplexCluster> PatternPhrasesStorage::GetClusters() const
{
    return clusters;
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

double PatternPhrasesStorage::CalculateTopicRelevance(const WordComplexCluster& cluster,
                                                      const std::unordered_map<int, std::vector<std::string>>& topics)
{
    double relevanceScore = 0.0;

    int matches = 0;
    for (const auto& lemma : cluster.lemmas) {
        for (const auto& topic : topics) {
            if (std::find(topic.second.begin(), topic.second.end(), lemma) != topic.second.end()) {
                matches++;
                break;
            }
        }
    }

    if (!cluster.lemmas.empty()) {
        relevanceScore = static_cast<double>(matches) / cluster.lemmas.size();
    }

    return relevanceScore;
}

double PatternPhrasesStorage::CalculateCentrality(const WordComplexCluster& cluster, const MatrixXd& U,
                                                  const std::vector<std::string>& words)
{
    double centralityScore = 0.0;

    std::vector<int> termIndices;
    for (const auto& lemma : cluster.lemmas) {
        auto it = std::find(words.begin(), words.end(), lemma);
        if (it != words.end()) {
            termIndices.push_back(std::distance(words.begin(), it));
        }
    }

    if (!termIndices.empty()) {
        std::vector<double> similarities;
        for (size_t i = 0; i < termIndices.size(); ++i) {
            for (size_t j = i + 1; j < termIndices.size(); ++j) {
                int idx1 = termIndices[i];
                int idx2 = termIndices[j];

                VectorXd vec1 = U.row(idx1);
                VectorXd vec2 = U.row(idx2);

                double cosSim = vec1.dot(vec2) / (vec1.norm() * vec2.norm());
                similarities.push_back(cosSim);
            }
        }

        if (!similarities.empty()) {
            centralityScore = std::accumulate(similarities.begin(), similarities.end(), 0.0) / similarities.size();
        }
    }

    return centralityScore;
}

void PatternPhrasesStorage::UpdateClusterMetrics(const MatrixXd& U, const std::vector<std::string>& words,
                                                 const std::unordered_map<int, std::vector<std::string>>& topics)
{
    for (auto& clusterPair : clusters) {
        auto& cluster = clusterPair.second;

        cluster.topicRelevance = CalculateTopicRelevance(cluster, topics);

        cluster.centralityScore = CalculateCentrality(cluster, U, words);
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
                cluster.tagMatch = true;
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

        if (AreKeysSimilar(previousKey, currentKey) || AreKeysSimilar(previousKey, currentKey, 2, 4, true)) {
            // Move all wordComplexes from the current cluster to the previous cluster
            auto& previousCluster = clusters[previousKey];
            auto& currentCluster = clusters[currentKey];

            previousCluster.tfidf.size();
            for (i = 0; i < previousCluster.tfidf.size(); i++) {
                if (currentCluster.tf[i] > previousCluster.tf[i])
                    previousCluster.tf[i] = currentCluster.tf[i];
                if (currentCluster.idf[i] > previousCluster.idf[i])
                    previousCluster.idf[i] = currentCluster.idf[i];
                if (currentCluster.tfidf[i] > previousCluster.tfidf[i])
                    previousCluster.tfidf[i] = currentCluster.tfidf[i];
            }

            previousCluster.tagMatch = currentCluster.tagMatch || previousCluster.tagMatch;

            previousCluster.wordComplexes.insert(previousCluster.wordComplexes.end(),
                                                 currentCluster.wordComplexes.begin(),
                                                 currentCluster.wordComplexes.end());

            // Remove the current cluster after the move
            clusters.erase(currentKey);
        }
    }
}

bool PatternPhrasesStorage::AreKeysSimilar(const std::string& key1, const std::string& key2, size_t maxDiff,
                                           size_t endLength, bool CheckFirstOnly)
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
    bool firstWordChecked = false;

    // Compare the beginnings and endings of each word
    for (size_t i = 0; i < words1.size(); ++i) {
        if ((words1[i].length() <= 8 || words2[i].length() <= 8) && words1[i] != words2[i]) {
            return false;
        }

        // Extract the initial parts of the words (everything except the last few characters)
        std::string start1 =
            (words1[i].length() > endLength) ? words1[i].substr(0, words1[i].length() - endLength) : "";
        std::string start2 =
            (words2[i].length() > endLength) ? words2[i].substr(0, words2[i].length() - endLength) : "";

        // Check if the difference in length between the initial parts exceeds maxDiff
        if (std::abs(static_cast<long>(start1.length()) - static_cast<long>(start2.length())) >= maxDiff) {
            return false;
        }

        // Adjust the initial parts to be the same length
        if (start1.length() != start2.length()) {
            if (start1.length() > start2.length()) {
                start1 = start1.substr(0, start2.length());
            } else {
                start2 = start2.substr(0, start1.length());
            }
        }

        // Skip comparison if either start is empty
        if (start1.empty() || start2.empty()) {
            continue;
        }

        // Compare the initial parts of the words
        if (start1 != start2) {
            // If initial parts do not match, keys are not similar
            return false;
        }

        // Extract the endings of the words
        std::string end1 =
            (words1[i].length() >= endLength) ? words1[i].substr(words1[i].length() - endLength) : words1[i];
        std::string end2 =
            (words2[i].length() >= endLength) ? words2[i].substr(words2[i].length() - endLength) : words2[i];

        // Compare the endings of the words
        if (end1 != end2) {
            ++diffCount;

            // If CheckFirstOnly is true, only check the first word for differences
            if (CheckFirstOnly) {
                // If the first word has been checked and the difference is not zero, subsequent words must match
                // exactly
                firstWordChecked = true;
                if (diffCount > maxDiff) {
                    return false;
                }
            } else {
                // If not checking the first word only, ensure differences don't exceed maxDiff
                if (diffCount > maxDiff) {
                    return false;
                }
            }
        }

        // If CheckFirstOnly is true and the first word has been checked with differences, ensure remaining words match
        if (CheckFirstOnly && firstWordChecked && i > 0) {
            if (words1[i] != words2[i]) {
                return false;
            }
        }
    }

    // If the number of differences in the endings does not exceed maxDiff, the keys are considered similar
    return diffCount <= maxDiff;
}

void PatternPhrasesStorage::LoadWikiWNRelations()
{

    SemanticRelationsDB semanticDB;

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

void PatternPhrasesStorage::OutputClustersToJsonFile(const std::string& filename, bool mergeNestedClusters,
                                                     bool termsOnly) const
{
    json j;

    std::vector<std::string> keys;

    if (termsOnly) {
        keys.reserve(clustersToInclude.size());
        for (const auto& key : clustersToInclude) {
            keys.push_back(key);
        }

    } else {
        keys.reserve(clusters.size());
        for (const auto& pair : clusters) {
            keys.push_back(pair.first);
        }
    }

    std::sort(keys.begin(), keys.end());

    json* previousClusterJson = nullptr;
    std::string previousKey;

    for (const auto& key : keys) {
        const auto& cluster = clusters.at(key);
        double phrasesCount = static_cast<double>(cluster.wordComplexes.size());

        json clusterJson;
        clusterJson["0_phrase_size"] = cluster.phraseSize;
        clusterJson["1_frequency"] = phrasesCount / static_cast<double>(g_options.textToProcessCount);
        clusterJson["2_topic_relevance"] = cluster.topicRelevance;
        clusterJson["3_centrality_score"] = cluster.centralityScore;
        clusterJson["4_tag_match"] = cluster.tagMatch;
        clusterJson["5_model_name"] = cluster.modelName;

        std::vector<std::string> synonymsJson(cluster.synonyms.begin(), cluster.synonyms.end());
        clusterJson["9_synonyms"] = synonymsJson;

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
            lemmasJson.push_back(lemmaJson);
        }
        clusterJson["6_lemmas"] = lemmasJson;

        clusterJson["7_phrases_count"] = phrasesCount;
        std::vector<json> phrases;
        for (const auto& wordComplex : cluster.wordComplexes) {
            json phraseJson;
            phraseJson["0_text_form"] = wordComplex->textForm;
            phraseJson["1_position"] = {{"0_start", wordComplex->pos.start},
                                        {"1_end", wordComplex->pos.end},
                                        {"2_doc_num", wordComplex->pos.docNum},
                                        {"3_sent_num", wordComplex->pos.sentNum}};

            for (const auto& contextSentence : cluster.contexts) {
                const TokenizedSentence& context = contextSentence;

                if (context.docNum == wordComplex->pos.docNum && context.sentNum == wordComplex->pos.sentNum) {
                    phraseJson["2_context"] = context.originalStr;
                    break;
                }
            }

            phrases.push_back(phraseJson);
        }
        clusterJson["8_phrases"] = phrases;

        // If mergeNestedClusters is true, check for substring relation with the previous key
        if (mergeNestedClusters && previousClusterJson && key.find(previousKey) == 0) {
            // If the key starts with the previous key, treat it as a nested cluster
            json nestedClusterJson = clusterJson;
            nestedClusterJson["00_key"] = key; // Add the key to the nested cluster
            (*previousClusterJson)["nested_clusters"].push_back(nestedClusterJson);
        } else {
            // Save current clusterJson for potential nesting in the next iteration
            j[key] = clusterJson;
            previousClusterJson = &j[key];
            previousKey = key;
        }
    }

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        throw std::runtime_error("Could not open file for writing");
    }

    outFile << j.dump(4);
    outFile.close();
}
#include "PatternPhrasesStorage.h"
#include "CorpusVocabulary.h"
#include "Logger.h"
#include "Options.h"
#include "SemanticRelations.h"
#include "TopicManager.h"

#include <regex>

using json = nlohmann::json;

void PatternPhrasesStorage::addCluster(const std::string& key, const PhraseCluster& cluster) {
    clusters[key] = cluster;
}

PhraseCluster* PatternPhrasesStorage::findCluster(const std::string& key) {
    auto it = clusters.find(key);
    if (it != clusters.end()) {
        return &(it->second);
    }
    return nullptr;
}

void PatternPhrasesStorage::reserveClusters(size_t count) {
    clusters.reserve(count);
}

void PatternPhrasesStorage::addContextsToClusters() {
    Logger::log("PhrasesStorage", LogLevel::Info, "Adding contexts to clusters...");
    auto& corpus = SentenceCorpus::GetCorpus();

    for (auto& clusterPair : clusters) {
        PhraseCluster& cluster = clusterPair.second;
        cluster.contexts.clear();

        for (const auto& phrase : cluster.phrases) {
            const Position& pos = phrase->pos;
            const auto sentence = corpus.getSentence(pos.docId, pos.sentNum);

            if (sentence.has_value()) {
                cluster.contexts.push_back(sentence.value());
            }
        }
    }
}

// // Checks if the key of phrase1 is a prefix of phrase2's key
// bool IsPrefix(const std::string& phrase1Key, const std::string& phrase2Key) {
//     return phrase2Key.find(phrase1Key) == 0; // Checks if phrase1Key is at the beginning of phrase2Key
// }

// // Checks exclusion conditions based on TF-IDF and frequency
// bool ShouldExcludeBasedOnTfidfAndFrequency(const PhraseCluster& phrase1, const PhraseCluster& phrase2,
//                                            const PhraseCluster& phrase3) {
//     // Check TF-IDF conditions for phrase1 and phrase2
//     bool tfidfCondition =
//         !phrase1.tfidf.empty() && !phrase3.tfidf.empty() && phrase1.tfidf[0] < 0.0005 && phrase3.tfidf.back() >
//         0.0005;

//     // Check frequency conditions for phrase1 and phrase2 compared to phrase3
//     bool frequencyCondition =
//         phrase1.frequency < phrase3.frequency / 10.0 && phrase2.frequency < phrase3.frequency / 10.0;

//     // Return true if both conditions are met
//     return tfidfCondition && frequencyCondition;
// }

std::vector<std::string> Split(const std::string& str) {
    std::istringstream iss(str);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

static nlohmann::json LoadClassifiedPhrases(const std::string& filePath) {
    nlohmann::json phraseLabels;
    std::ifstream jsonFile(filePath);
    if (jsonFile.is_open()) {
        jsonFile >> phraseLabels;
        jsonFile.close();
    }
    return phraseLabels;
}

// void PatternPhrasesStorage::initializeAndFilterClusters(double tfidfThreshold, std::set<std::string>& sortedKeys,
//                                                         std::unordered_set<std::string>& clustersToInclude) {
//     const auto& clusters = getClusters();
//     std::regex romanNumeralsRegex(R"(^[ivxlcd]+$)", std::regex_constants::icase);

//     for (const auto& pair : clusters) {
//         const std::string& key = pair.first;
//         const PhraseCluster& cluster = pair.second;

//         std::vector<std::string> words = Split(key);
//         bool hasRomanNumerals = false;
//         for (const std::string& word : words) {
//             if (std::regex_match(word, romanNumeralsRegex)) {
//                 hasRomanNumerals = true;
//                 break;
//             }
//         }
//         if (hasRomanNumerals) {
//             continue;
//         }

//         sortedKeys.insert(key);
//         clustersToInclude.insert(key);

//         bool hasLowTfidf = false;
//         for (double val : cluster.tfidf) {
//             if (val < tfidfThreshold) {
//                 hasLowTfidf = true;
//                 break;
//             }
//         }

//         if (hasLowTfidf) {
//             if (cluster.frequency <= 1.0 && cluster.topicRelevance <= 0.4) {
//                 clustersToInclude.erase(key);
//                 sortedKeys.erase(key);
//             }
//         }
//     }
// }

void PatternPhrasesStorage::applyClassifiedPhrases(const nlohmann::ordered_json& phraseLabels,
                                                   std::set<std::string>& sortedKeys,
                                                   std::unordered_set<std::string>& clustersToInclude) {
    const auto& clusters = getClusters();

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

// void PatternPhrasesStorage::checkModelPrefixRelationships(std::set<std::string>& sortedKeys,
//                                                           std::unordered_set<std::string>& clustersToInclude) {
//     const auto& clusters = getClusters();

//     auto it = sortedKeys.begin();
//     while (it != sortedKeys.end()) {
//         auto nextIt = std::next(it);
//         if (nextIt == sortedKeys.end()) {
//             break;
//         }

//         const auto& key1 = *it;
//         const auto& key2 = *nextIt;

//         const auto& phrase1 = clusters.at(key1);
//         const auto& phrase2 = clusters.at(key2);

//         bool conditionModel = ((phrase1.modelName == "Прил + С" && phrase2.modelName == "(Прил + С) + Срд") ||
//                                (phrase1.modelName == "Прич + С" && phrase2.modelName == "(Прич + С) + Срд")) &&
//                               IsPrefix(key1, key2);

//         if (conditionModel) {
//             std::size_t pos = key2.find(' ');
//             if (pos != std::string::npos) {
//                 std::string trimmedKey = key2.substr(pos + 1);

//                 auto thirdIt = clusters.find(trimmedKey);
//                 if (thirdIt != clusters.end() && thirdIt->second.modelName == "С + Срд") {
//                     const auto& phrase3 = thirdIt->second;

//                     if (phrase1.centralityScore < 0.15 &&
//                         ShouldExcludeBasedOnTfidfAndFrequency(phrase1, phrase2, phrase3)) {
//                         clustersToInclude.erase(key1);
//                         clustersToInclude.erase(key2);
//                     }
//                 }
//             }
//         }

//         ++it;
//     }
// }

// void PatternPhrasesStorage::collectTerms(double tfidfThreshold) {
//     Logger::log("PhrasesStorage", LogLevel::Info, "Collecting terms...");
//     std::set<std::string> sortedKeys;
//     const auto& clusters = getClusters();

//     initializeAndFilterClusters(tfidfThreshold, sortedKeys, clustersToInclude);

//     auto phraseLabels =
//         LoadClassifiedPhrases("/home/milkorna/Documents/AutoThematicThesaurus/my_data/classified_phrases.json");

//     applyClassifiedPhrases(phraseLabels, sortedKeys, clustersToInclude);

//     checkModelPrefixRelationships(sortedKeys, clustersToInclude);

//     WriteClustersToFile(clustersToInclude, "terms.txt");
// }

void PatternPhrasesStorage::evaluateTermRelevance(const LSA& lsa) {
    // const auto& topics = lsa.GetTopics();
    // for (auto& [key, cluster] : clusters) {
    //     int relevantCount = 0;
    //     for (const auto& topic : topics) {
    //         for (const auto& word : topic.second) {
    //             if (std::find(cluster.lemmas.begin(), cluster.lemmas.end(), word) != cluster.lemmas.end()) {
    //                 relevantCount++;
    //             }
    //         }
    //     }
    //     cluster.topicRelevance = static_cast<double>(relevantCount) / topics.size();
    // }
}

const PhraseClusters PatternPhrasesStorage::getClusters() const {
    return clusters;
}

// std::map<std::string, int>
// CalculateTopicFrequency(const std::unordered_map<std::string, std::vector<std::string>>& similar_words) {
//     std::map<std::string, int> topicFrequency;
//     for (const auto& pair : similar_words) {
//         for (const auto& topic : pair.second) {
//             topicFrequency[topic]++;
//         }
//     }
//     return topicFrequency;
// }

// void ApplyTopicFrequencyPenalty(std::unordered_map<std::string, std::vector<std::string>>& similar_words,
//                                 int frequencyThreshold) {
//     auto topicFrequency = CalculateTopicFrequency(similar_words);
//     for (auto& pair : similar_words) {
//         pair.second.erase(
//             std::remove_if(pair.second.begin(), pair.second.end(),
//                            [&](const std::string& topic) { return topicFrequency[topic] > frequencyThreshold; }),
//             pair.second.end());
//     }
// }

// double PatternPhrasesStorage::calculateTopicRelevance(const PhraseCluster& cluster,
//                                                       const std::unordered_map<int, std::vector<std::string>>&
//                                                       topics) {
//     double relevanceScore = 0.0;

//     int matches = 0;
//     for (const auto& lemma : cluster.lemmas) {
//         for (const auto& topic : topics) {
//             if (std::find(topic.second.begin(), topic.second.end(), lemma) != topic.second.end()) {
//                 matches++;
//                 break;
//             }
//         }
//     }

//     if (!cluster.lemmas.empty()) {
//         relevanceScore = static_cast<double>(matches) / cluster.lemmas.size();
//     }

//     return relevanceScore;
// }

// double PatternPhrasesStorage::calculateCentrality(const PhraseCluster& cluster, const MatrixXd& U,
//                                                   const std::vector<std::string>& words) {
//     double centralityScore = 0.0;

//     std::vector<int> termIndices;
//     for (const auto& lemma : cluster.lemmas) {
//         auto it = std::find(words.begin(), words.end(), lemma);
//         if (it != words.end()) {
//             termIndices.push_back(std::distance(words.begin(), it));
//         }
//     }

//     if (!termIndices.empty()) {
//         std::vector<double> similarities;
//         for (size_t i = 0; i < termIndices.size(); ++i) {
//             for (size_t j = i + 1; j < termIndices.size(); ++j) {
//                 int idx1 = termIndices[i];
//                 int idx2 = termIndices[j];

//                 VectorXd vec1 = U.row(idx1);
//                 VectorXd vec2 = U.row(idx2);

//                 double cosSim = vec1.dot(vec2) / (vec1.norm() * vec2.norm());
//                 similarities.push_back(cosSim);
//             }
//         }

//         if (!similarities.empty()) {
//             centralityScore = std::accumulate(similarities.begin(), similarities.end(), 0.0) / similarities.size();
//         }
//     }

//     return centralityScore;
// }

// void PatternPhrasesStorage::calculateLSAMetrics(const MatrixXd& U, const std::vector<std::string>& words,
//                                                 const std::unordered_map<int, std::vector<std::string>>& topics) {
//     Logger::log("PhrasesStorage", LogLevel::Info, "Updating cluster metrics...");

//     for (auto& clusterPair : clusters) {
//         auto& cluster = clusterPair.second;

//         cluster.topicRelevance = calculateTopicRelevance(cluster, topics);

//         cluster.centralityScore = calculateCentrality(cluster, U, words);
//     }
// }

/*
 * Calculation of topicRelevance based on vectors from the U matrix (and optionally Sigma):
 *
 * - For each lemma calculate its vector: row(U) (if desired, scale by Sigma).
 * - Determines which component in this vector is the largest modulo and counts ratio = max^2 / sumOfSquares.
 * - Summarizes the ratio for all lemmas and take the average.
 */
// double PatternPhrasesStorage::calculateTopicRelevance(const PhraseCluster& cluster, const Eigen::MatrixXd& U,
//                                                       const Eigen::MatrixXd& Sigma,
//                                                       const std::vector<std::string>& words,
//                                                       const LSA_MetricsConfig& config) {
//     // How much of the component is actually used
//     int usedCols = config.maxComponents.has_value() ? std::min<int>(config.maxComponents.value(), U.cols()) :
//     U.cols();

//     double sumAll = 0.0;
//     int validLemmaCount = 0;

//     for (const auto& lemma : cluster.lemmas) {
//         // Find index for lemma
//         auto it = std::find(words.begin(), words.end(), lemma);
//         if (it == words.end()) {
//             continue;
//         }

//         int rowIndex = static_cast<int>(std::distance(words.begin(), it));

//         Eigen::VectorXd vec = U.row(rowIndex).head(usedCols);

//         // If needs to multiply by Sigma
//         if (config.applySigmaScaling && Sigma.cols() == Sigma.rows()) {
//             for (int d = 0; d < usedCols; ++d) {
//                 double s = Sigma(d, d); // diagonal
//                 vec[d] *= s;
//             }
//         }

//         double sumSq = vec.squaredNorm();
//         if (sumSq < 1e-15) {
//             continue; // vector is almost zero, skipping
//         }

//         // Find max^2
//         double maxSq = 0.0;
//         for (int d = 0; d < usedCols; ++d) {
//             double sq = vec[d] * vec[d];
//             if (sq > maxSq) {
//                 maxSq = sq;
//             }
//         }

//         double lemmaRelevance = maxSq / sumSq;
//         sumAll += lemmaRelevance;
//         validLemmaCount++;
//     }

//     if (validLemmaCount == 0) {
//         return 0.0;
//     }

//     return sumAll / static_cast<double>(validLemmaCount);
// }

// /*
//  * Calculation of using vectors from U (and Sigma).
//  * 1) The vectors of all lemmas of the cluster are collected
//  * 2) The "centroid" (the average of the vectors) is searched for
//  * 3) For each lemma, the similarity (or distance) to the centroid is read.
//  * 4) Cluster average = centrality
//  */
// double PatternPhrasesStorage::calculateCentrality(const PhraseCluster& cluster, const Eigen::MatrixXd& U,
//                                                   const Eigen::MatrixXd& Sigma, const std::vector<std::string>&
//                                                   words, const LSA_MetricsConfig& config) {
//     // Сколько используем компонент
//     int usedCols = config.maxComponents.has_value() ? std::min<int>(config.maxComponents.value(), U.cols()) :
//     U.cols();

//     // Собираем вектора всех лемм кластера
//     std::vector<Eigen::VectorXd> lemmaVectors;
//     lemmaVectors.reserve(cluster.lemmas.size());

//     for (const auto& lemma : cluster.lemmas) {
//         auto it = std::find(words.begin(), words.end(), lemma);
//         if (it == words.end()) {
//             continue;
//         }

//         int rowIndex = static_cast<int>(std::distance(words.begin(), it));
//         Eigen::VectorXd vec = U.row(rowIndex).head(usedCols);

//         if (config.applySigmaScaling && Sigma.cols() == Sigma.rows()) {
//             for (int d = 0; d < usedCols; ++d) {
//                 double s = Sigma(d, d);
//                 vec[d] *= s;
//             }
//         }
//         lemmaVectors.push_back(vec);
//     }

//     if (lemmaVectors.empty()) {
//         return 0.0;
//     }

//     // Вычисляем центроид
//     Eigen::VectorXd centroid = Eigen::VectorXd::Zero(usedCols);
//     for (const auto& v : lemmaVectors) {
//         centroid += v;
//     }
//     centroid /= static_cast<double>(lemmaVectors.size());

//     // Теперь считаем среднее сходство (или обратное расстояние)
//     // между леммой и центроидом
//     double sumScore = 0.0;
//     for (const auto& v : lemmaVectors) {
//         if (config.useCosineForCentrality) {
//             // Косинусное сходство
//             double denom = v.norm() * centroid.norm();
//             if (denom < 1e-15) {
//                 sumScore += 0.0;
//             } else {
//                 sumScore += (v.dot(centroid) / denom);
//             }
//         } else {
//             // Евклидова метрика -> пусть centrality = 1 / (1 + dist)
//             double dist = (v - centroid).norm();
//             sumScore += 1.0 / (1.0 + dist);
//         }
//     }

//     return sumScore / static_cast<double>(lemmaVectors.size());
// }

// void PatternPhrasesStorage::calculateLSAMetrics(const Eigen::MatrixXd& U, const std::vector<std::string>& words,
//                                                 const Eigen::MatrixXd& Sigma, const LSA_MetricsConfig& config) {
//     Logger::log("PhrasesStorage", LogLevel::Info, "Updating cluster metrics with advanced LSA approach...");

//     for (auto& clusterPair : clusters) {
//         auto& cluster = clusterPair.second;

//         cluster.topicRelevance = calculateTopicRelevance(cluster, U, Sigma, words, config);

//         cluster.centralityScore = calculateCentrality(cluster, U, Sigma, words, config);
//     }
// }

// void PatternPhrasesStorage::computeTextMetrics() {
//     Logger::log("PhrasesStorage", LogLevel::Info, "Computing text metrics...");
//     const auto corpus = CorpusVocabulary::GetCorpus();
//     int totalDocuments = corpus.getDocumentCount();
//     const auto& topicVectors = TopicManager::getTopicVectors();
//     static std::unordered_map<std::string, std::vector<std::string>> totalTopics;
//     auto& options = Options::getOptions();

//     for (auto& clusterPair : clusters) {
//         auto& cluster = clusterPair.second;

//         cluster.tf.resize(cluster.phraseSize, 0.0);
//         cluster.idf.resize(cluster.phraseSize, 0.0);
//         cluster.tfidf.resize(cluster.phraseSize, 0.0);

//         for (size_t i = 0; i < cluster.phraseSize; ++i) {
//             const std::string& lemma = cluster.lemmas[i];
//             // cluster.tf[i] = corpus.CalculateTF(lemma); TODO
//             // cluster.idf[i] = corpus.CalculateIDF(lemma);
//             cluster.tfidf[i] = cluster.tf[i] * cluster.idf[i];
//         }

//         const WordEmbeddingPtr& myEmbedding = std::make_shared<WordEmbedding>(cluster.key);
//         std::vector<std::string> topics;
//         double cosineWeight = 0.6;
//         double euclideanWeight = 0.2;
//         double manhattanWeight = 0.2;

//         for (const auto& topicVecPair : topicVectors) {
//             const std::string& topicWord = topicVecPair.first;
//             const WordEmbeddingPtr& topicEmbedding = topicVecPair.second;

//             double cosineSim = myEmbedding->cosineSimilarity(*topicEmbedding);
//             double euclideanDist = myEmbedding->euclideanDistance(*topicEmbedding);
//             double manhattanDist = myEmbedding->manhattanDistance(*topicEmbedding);

//             double combinedScore = cosineWeight * cosineSim + euclideanWeight * (1.0f / (1.0f + euclideanDist)) +
//                                    manhattanWeight * (1.0f / (1.0f + manhattanDist));

//             if (combinedScore > options.topicsThreshold) {
//                 topics.push_back(topicWord);
//             }
//             totalTopics[cluster.key] = topics;
//         }
//     }
//     int frequencyThreshold = static_cast<int>(clusters.size() * options.freqThresholdCoeff);
//     ApplyTopicFrequencyPenalty(totalTopics, frequencyThreshold);
//     for (auto& clusterPair : clusters) {
//         auto& cluster = clusterPair.second;
//         if (const auto& iter = totalTopics.find(clusterPair.first); iter != totalTopics.end()) {
//             if (iter->second.size() > 0 && iter->second.size() < options.thresholdTopicsCount) {
//                 cluster.tagMatch = true;
//             }
//         }
//     }
// }

// void PatternPhrasesStorage::loadWikiWNRelations() {
//     Logger::log("PhrasesStorage", LogLevel::Info, "Loading WikiWordNet relations...");
//     SemanticRelationsDB semanticDB;

//     auto& options = Options::getOptions();

//     for (auto& clusterPair : clusters) {
//         PhraseCluster& cluster = clusterPair.second;
//         std::cout << cluster.key << std::endl;

//         for (auto lemma : cluster.lemmas) {

//             if (hypernymCache.find(lemma) != hypernymCache.end()) {
//                 cluster.hypernyms[lemma] = hypernymCache[lemma];
//             } else {
//                 auto hypernyms = semanticDB.GetRelations(lemma, "hypernym");
//                 hypernymCache[lemma] = hypernyms;
//                 cluster.hypernyms[lemma] = hypernyms;
//             }

//             if (hyponymCache.find(lemma) != hyponymCache.end()) {
//                 cluster.hyponyms[lemma] = hyponymCache[lemma];
//             } else {
//                 auto hyponyms = semanticDB.GetRelations(lemma, "hyponym");
//                 std::set<std::string> validHyponyms;
//                 if (hyponyms.size() < 150) {
//                     for (const auto& hyp : hyponyms) {
//                         const WordEmbeddingPtr& myEmbedding = std::make_shared<WordEmbedding>(hyp);
//                         const auto& topicVectors = TopicManager::getTopicVectors();

//                         for (const auto& topicVecPair : topicVectors) {
//                             const std::string& topicWord = topicVecPair.first;
//                             const WordEmbeddingPtr& topicEmbedding = topicVecPair.second;

//                             double cosineSim = myEmbedding->cosineSimilarity(*topicEmbedding);
//                             if (cosineSim > options.topicsHyponymThreshold) {
//                                 validHyponyms.insert(hyp);
//                             }
//                         }
//                     }
//                 }
//                 hyponymCache[lemma] = validHyponyms;
//                 cluster.hyponyms[lemma] = validHyponyms;
//             }
//         }
//     }
// }

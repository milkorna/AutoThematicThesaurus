#include "ClusterSerializer.h"
#include "Logger.h"
#include <algorithm>
#include <sstream>

json ClusterSerializer::serializeCluster(const WordComplexCluster& cluster, double frequency) const {
    json clusterJson;

    // Основные поля кластера
    clusterJson["0_phrase_size"] = cluster.phraseSize;
    clusterJson["1_frequency"] = frequency;
    clusterJson["2_topic_relevance"] = cluster.topicRelevance;
    clusterJson["3_centrality_score"] = cluster.centralityScore;
    clusterJson["4_tag_match"] = cluster.tagMatch;
    clusterJson["5_model_name"] = cluster.modelName;

    // Синонимы
    std::vector<std::string> synonymsVec(cluster.synonyms.begin(), cluster.synonyms.end());
    clusterJson["9_synonyms"] = synonymsVec;

    // Леммы с метриками
    clusterJson["6_lemmas"] = serializeLemmas(cluster);

    // Количество фраз
    clusterJson["7_phrases_count"] = static_cast<double>(cluster.wordComplexes.size());

    // Фразы
    clusterJson["8_phrases"] = serializeWordComplexes(cluster);

    return clusterJson;
}

json ClusterSerializer::serialize(const std::unordered_map<std::string, WordComplexCluster>& clusters,
                                  const std::unordered_map<std::string, double>& frequencies, bool mergeNested) const {

    json resultJson;

    // Получаем отсортированные ключи
    std::vector<std::string> sortedKeys;
    sortedKeys.reserve(clusters.size());

    for (const auto& pair : clusters) {
        sortedKeys.push_back(pair.first);
    }
    std::sort(sortedKeys.begin(), sortedKeys.end());

    // Для отслеживания вложенности
    json* previousClusterJson = nullptr;
    std::string previousKey;

    for (const auto& key : sortedKeys) {
        const auto& cluster = clusters.at(key);

        // Получаем частоту или вычисляем по умолчанию
        double frequency = 0.0;
        auto freqIt = frequencies.find(key);
        if (freqIt != frequencies.end()) {
            frequency = freqIt->second;
        }

        // Сериализуем кластер
        json clusterJson = serializeCluster(cluster, frequency);

        // Логирование
        Logger::log("ClusterSerializer", LogLevel::Debug, "Serialized cluster: " + key);

        // Обработка вложенности
        if (mergeNested && previousClusterJson != nullptr && key.find(previousKey) == 0) {
            // Текущий ключ начинается с предыдущего - это вложенный кластер
            json nestedClusterJson = clusterJson;
            nestedClusterJson["00_key"] = key;
            (*previousClusterJson)["nested_clusters"].push_back(nestedClusterJson);
        } else {
            // Это отдельный кластер верхнего уровня
            resultJson[key] = clusterJson;
            previousClusterJson = &resultJson[key];
            previousKey = key;
        }
    }

    return resultJson;
}

json ClusterSerializer::serializeLemmas(const WordComplexCluster& cluster) const {
    json lemmasArray = json::array();

    for (size_t i = 0; i < cluster.lemmas.size(); ++i) {
        json lemmaObj = createLemmaObject(cluster.lemmas[i], i, cluster);
        lemmasArray.push_back(lemmaObj);
    }

    return lemmasArray;
}

json ClusterSerializer::createLemmaObject(const std::string& lemma, size_t index,
                                          const WordComplexCluster& cluster) const {
    json lemmaJson;

    // Леммы с индексом для порядка
    std::string lemmaStrNumbered = std::to_string(index) + "_" + lemma;
    lemmaJson["0_lemma"] = lemmaStrNumbered;

    // TF, IDF, TF-IDF
    if (index < cluster.tf.size()) {
        lemmaJson["1_tf"] = cluster.tf[index];
    }
    if (index < cluster.idf.size()) {
        lemmaJson["2_idf"] = cluster.idf[index];
    }
    if (index < cluster.tfidf.size()) {
        lemmaJson["3_tf-idf"] = cluster.tfidf[index];
    }

    // Семантические отношения
    json semanticRelations = serializeSemanticRelations(lemma, cluster);
    lemmaJson["4_hypernyms"] = semanticRelations["hypernyms"];
    lemmaJson["5_hyponyms"] = semanticRelations["hyponyms"];

    return lemmaJson;
}

json ClusterSerializer::serializeSemanticRelations(const std::string& lemma, const WordComplexCluster& cluster) const {
    json relationsJson;
    relationsJson["hypernyms"] = json::array();
    relationsJson["hyponyms"] = json::array();

    // Ищем отношения для леммы
    auto hypernymIt = cluster.hypernyms.find(lemma);
    if (hypernymIt != cluster.hypernyms.end()) {
        relationsJson["hypernyms"] = hypernymIt->second;
    }

    auto hyponymIt = cluster.hyponyms.find(lemma);
    if (hyponymIt != cluster.hyponyms.end()) {
        relationsJson["hyponyms"] = hyponymIt->second;
    }

    return relationsJson;
}

json ClusterSerializer::serializeWordComplexes(const WordComplexCluster& cluster) const {
    json phrasesArray = json::array();

    for (const auto& wordComplex : cluster.wordComplexes) {
        json phraseObj = createPhraseObject(wordComplex, cluster.contexts);
        phrasesArray.push_back(phraseObj);
    }

    return phrasesArray;
}

json ClusterSerializer::createPhraseObject(const WordComplexPtr& wordComplex,
                                           const std::vector<TokenizedSentence>& contexts) const {

    json phraseJson;

    phraseJson["0_text_form"] = wordComplex->textForm;

    // Позиция с вложенной структурой
    phraseJson["1_position"] = {{"0_start", wordComplex->pos.start},
                                {"1_end", wordComplex->pos.end},
                                {"2_doc_num", wordComplex->pos.docNum},
                                {"3_sent_num", wordComplex->pos.sentNum}};

    // Ищем контекст (оригинальное предложение)
    for (const auto& context : contexts) {
        if (context.docNum == wordComplex->pos.docNum && context.sentNum == wordComplex->pos.sentNum) {
            phraseJson["2_context"] = context.originalStr;
            break;
        }
    }

    return phraseJson;
}

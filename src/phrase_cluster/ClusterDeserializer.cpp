#include "ClusterDeserializer.h"
#include "Logger.h"

#include <unicode/uchar.h>

using json = nlohmann::json;

WordComplexCluster ClusterDeserializer::deserializeCluster(const json& obj, const std::string& key) {
    try {
        if (!obj.is_object()) {
            throw std::runtime_error("Expected JSON object for cluster");
        }

        WordComplexCluster cluster;
        cluster.key = key;
        cluster.phraseSize = obj.at("phrase_size").get<size_t>();
        cluster.frequency = obj.at("frequency").get<double>();
        cluster.topicRelevance = obj.at("topic_relevance").get<double>();
        cluster.centralityScore = obj.at("centrality_score").get<double>();
        cluster.tagMatch = obj.at("tag_match").get<bool>();
        cluster.modelName = obj.at("model_name").get<std::string>();

        // Synonyms (optional field)
        if (obj.contains("synonyms")) {
            cluster.synonyms = obj.at("synonyms").get<std::unordered_set<std::string>>();
        }

        // Deserialize lemmas with metrics
        deserializeLemmas(obj.at("lemmas"), cluster);

        // Deserialize word complexes (phrases)
        deserializeWordComplexes(obj.at("phrases"), cluster);

        Logger::log("ClusterDeserializer", LogLevel::Debug, "Successfully deserialized cluster: " + key);

        return cluster;

    } catch (const json::exception& e) {
        Logger::log("ClusterDeserializer", LogLevel::Error,
                    "JSON error while deserializing cluster '" + key + "': " + std::string(e.what()));
        throw std::runtime_error("Failed to deserialize cluster: " + std::string(e.what()));
    } catch (const std::exception& e) {
        Logger::log("ClusterDeserializer", LogLevel::Error,
                    "Error deserializing cluster '" + key + "': " + std::string(e.what()));
        throw;
    }
}

WordComplexPtr ClusterDeserializer::deserializePhraseResult(const json& obj) {
    try {
        // Validate key format
        std::string key = obj.at("key").get<std::string>();

        if (!isValidPhraseKey(key)) {
            return nullptr; // Skip invalid keys
        }

        std::string textForm = obj.at("textForm").get<std::string>();
        std::string modelName = obj.at("modelName").get<std::string>();

        Position pos;
        pos.docId = obj.at("docId").get<std::string>();
        pos.sentNum = obj.at("sentNum").get<size_t>();
        pos.start = obj.at("start_ind").get<size_t>();
        pos.end = obj.at("end_ind").get<size_t>();

        // Extract lemmas
        std::deque<std::string> lemmas;
        if (obj.contains("lemmas")) {
            lemmas = obj.at("lemmas").get<std::deque<std::string>>();

            // // Clean up numbered lemmas (e.g., "0_lemma_name" -> "lemma_name")
            // for (auto& lemma : lemmas) {
            //     lemma = extractLemmaString(lemma);
            // }
        }

        // Create word complex
        WordComplexPtr wc = std::make_shared<WordComplex>();
        wc->textForm = textForm;
        wc->pos = pos;
        wc->modelName = modelName;
        wc->lemmas = lemmas;

        return wc;

    } catch (const std::exception& e) {
        Logger::log("ClusterDeserializer", LogLevel::Error,
                    "Error parsing phrase result JSON: " + std::string(e.what()));
        return nullptr;
    }
}

void ClusterDeserializer::deserializeLemmas(const json& lemmas_json, WordComplexCluster& cluster) {
    if (!lemmas_json.is_array()) {
        throw std::runtime_error("Lemmas field must be an array");
    }

    for (const auto& lemma_obj : lemmas_json) {
        if (!lemma_obj.is_object()) {
            throw std::runtime_error("Each lemma must be a JSON object");
        }

        // Extract lemma string
        std::string lemma = lemma_obj.at("lemma").get<std::string>();
        //  std::string lemmaStr = extractLemmaString(lemmaStrNumbered);

        // Add to cluster
        cluster.lemmas.push_back(lemma);
        cluster.tf.push_back(lemma_obj.at("tf").get<double>());
        cluster.idf.push_back(lemma_obj.at("idf").get<double>());
        cluster.tfidf.push_back(lemma_obj.at("tf-idf").get<double>());

        // Semantic relations
        cluster.hypernyms[lemma] = lemma_obj.at("hypernyms").get<std::set<std::string>>();
        cluster.hyponyms[lemma] = lemma_obj.at("hyponyms").get<std::set<std::string>>();

        // Word embedding
        cluster.wordVectors.push_back(std::make_shared<WordEmbedding>(lemma));
    }
}

void ClusterDeserializer::deserializeWordComplexes(const json& phrases_json, WordComplexCluster& cluster) {
    if (!phrases_json.is_array()) {
        throw std::runtime_error("Phrases field must be an array");
    }

    for (const auto& phrase_obj : phrases_json) {
        if (!phrase_obj.is_object()) {
            throw std::runtime_error("Each phrase must be a JSON object");
        }

        WordComplexPtr wc = std::make_shared<WordComplex>();
        wc->textForm = phrase_obj.at("text_form").get<std::string>();
        wc->modelName = cluster.modelName;

        // Extract position
        const auto& posObj = phrase_obj.at("position");
        wc->pos.start = posObj.at("start").get<size_t>();
        wc->pos.end = posObj.at("end").get<size_t>();
        wc->pos.docId = posObj.at("doc_num").get<size_t>();
        wc->pos.sentNum = posObj.at("sent_num").get<size_t>();

        // Copy lemmas from cluster
        wc->lemmas.assign(cluster.lemmas.begin(), cluster.lemmas.end());

        cluster.wordComplexes.push_back(wc);
    }
}

// std::string ClusterDeserializer::extractLemmaString(const std::string& numberedLemma) const {
//     size_t pos = numberedLemma.find('_');
//     if (pos != std::string::npos) {
//         return numberedLemma.substr(pos + 1);
//     }
//     return numberedLemma;
// }

bool ClusterDeserializer::isValidPhraseKey(const std::string& key) const {
    // Skip keys containing underscores
    if (key.find('_') != std::string::npos) {
        return false;
    }

    // Check for digits using ICU
    icu::UnicodeString unicodeText = icu::UnicodeString::fromUTF8(key);
    for (int32_t i = 0; i < unicodeText.length(); ++i) {
        UChar32 codepoint = unicodeText.char32At(i);
        if (u_isdigit(codepoint)) {
            return false;
        }
    }

    return true;
}

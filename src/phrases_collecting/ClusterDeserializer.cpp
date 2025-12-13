#include "ClusterDeserializer.h"
#include "Logger.h"
#include <unicode/uchar.h>
#include <algorithm>

using json = nlohmann::json;

WordComplexCluster ClusterDeserializer::deserialize(const json& obj, const std::string& key) {
    try {
        if (!obj.is_object()) {
            throw std::runtime_error("Expected JSON object for cluster");
        }

        WordComplexCluster cluster;
        cluster.key = key;
        cluster.phraseSize = obj.at("0_phrase_size").get<size_t>();
        cluster.frequency = obj.at("1_frequency").get<double>();
        cluster.topicRelevance = obj.at("2_topic_relevance").get<double>();
        cluster.centralityScore = obj.at("3_centrality_score").get<double>();
        cluster.tagMatch = obj.at("4_tag_match").get<bool>();
        cluster.modelName = obj.at("5_model_name").get<std::string>();

        // Synonyms (optional field)
        if (obj.contains("9_synonyms")) {
            cluster.synonyms = obj.at("9_synonyms").get<std::unordered_set<std::string>>();
        }

        // Deserialize lemmas with metrics
        deserializeLemmas(obj.at("6_lemmas"), cluster);

        // Deserialize word complexes (phrases)
        deserializeWordComplexes(obj.at("8_phrases"), cluster);

        Logger::log("ClusterDeserializer", LogLevel::Debug, 
                   "Successfully deserialized cluster: " + key);

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
        std::string key = obj.at("0_key").get<std::string>();
        
        if (!isValidPhraseKey(key)) {
            return nullptr;  // Skip invalid keys
        }

        std::string textForm = obj.at("1_textForm").get<std::string>();
        std::string modelName = obj.at("2_modelName").get<std::string>();

        Position pos;
        pos.docNum = obj.at("3_docNum").get<size_t>();
        pos.sentNum = obj.at("4_sentNum").get<size_t>();
        pos.start = obj.at("5_start_ind").get<size_t>();
        pos.end = obj.at("6_end_ind").get<size_t>();

        // Extract lemmas
        std::deque<std::string> lemmas;
        if (obj.contains("7_lemmas")) {
            lemmas = obj.at("7_lemmas").get<std::deque<std::string>>();
            
            // Clean up numbered lemmas (e.g., "0_lemma_name" -> "lemma_name")
            for (auto& lemma : lemmas) {
                lemma = extractLemmaString(lemma);
            }
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
        std::string lemmaStrNumbered = lemma_obj.at("0_lemma").get<std::string>();
        std::string lemmaStr = extractLemmaString(lemmaStrNumbered);

        // Add to cluster
        cluster.lemmas.push_back(lemmaStr);
        cluster.tf.push_back(lemma_obj.at("1_tf").get<double>());
        cluster.idf.push_back(lemma_obj.at("2_idf").get<double>());
        cluster.tfidf.push_back(lemma_obj.at("3_tf-idf").get<double>());

        // Semantic relations
        cluster.hypernyms[lemmaStr] = lemma_obj.at("4_hypernyms").get<std::set<std::string>>();
        cluster.hyponyms[lemmaStr] = lemma_obj.at("5_hyponyms").get<std::set<std::string>>();

        // Word embedding
        cluster.wordVectors.push_back(std::make_shared<WordEmbedding>(lemmaStr));
    }
}

void ClusterDeserializer::deserializeWordComplexes(const json& phrases_json, 
                                                    WordComplexCluster& cluster) {
    if (!phrases_json.is_array()) {
        throw std::runtime_error("Phrases field must be an array");
    }

    for (const auto& phrase_obj : phrases_json) {
        if (!phrase_obj.is_object()) {
            throw std::runtime_error("Each phrase must be a JSON object");
        }

        WordComplexPtr wc = std::make_shared<WordComplex>();
        wc->textForm = phrase_obj.at("0_text_form").get<std::string>();
        wc->modelName = cluster.modelName;

        // Extract position
        const auto& posObj = phrase_obj.at("1_position");
        wc->pos.start = posObj.at("0_start").get<size_t>();
        wc->pos.end = posObj.at("1_end").get<size_t>();
        wc->pos.docNum = posObj.at("2_doc_num").get<size_t>();
        wc->pos.sentNum = posObj.at("3_sent_num").get<size_t>();

        // Copy lemmas from cluster
        wc->lemmas.assign(cluster.lemmas.begin(), cluster.lemmas.end());

        cluster.wordComplexes.push_back(wc);
    }
}

std::string ClusterDeserializer::extractLemmaString(const std::string& numberedLemma) const {
    size_t pos = numberedLemma.find('_');
    if (pos != std::string::npos) {
        return numberedLemma.substr(pos + 1);
    }
    return numberedLemma;
}

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

#ifndef PHRASES_STORAGE_LOADER_H
#define PHRASES_STORAGE_LOADER_H

#include <PatternPhrasesStorage.h>
using json = nlohmann::json;

class PhrasesStorageLoader {
public:
    void LoadStorageFromFile(PatternPhrasesStorage& storage, const std::string& filename)
    {
        std::ifstream file(filename);
        if (file.is_open()) {
            json j;
            file >> j;
            Deserialize(storage, j);
        } else {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }
    }

    void LoadPhraseStorageFromResultsDir(PatternPhrasesStorage& storage)
    {
        fs::path repoPath = fs::current_path();
        fs::path outputDir = g_options.resDir;
        fs::create_directories(outputDir);
        std::vector<fs::path> res_files = GetResFiles();

        auto& corpus = TextCorpus::GetCorpus();
        storage.ReserveClusters(corpus.GetTotalTexts());

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

                    auto cluster = storage.FindCluster(key);
                    if (cluster != nullptr) {

                        if (std::find(cluster->wordComplexes.begin(), cluster->wordComplexes.end(), wc) ==
                            cluster->wordComplexes.end()) {
                            cluster->wordComplexes.push_back(wc);
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

                        WordComplexCluster newCluster = {wc->lemmas.size(), false,         1.0,         0.0, 0.0, key,
                                                         wc->modelName,     lemmas,        {wc},        {},  {},  {},
                                                         lemVectors,        lemmHypernyms, lemmHyponyms};
                        storage.AddCluster(key, newCluster);
                    }
                } catch (const std::exception& e) {
                    Logger::log("", LogLevel::Error, "Error parsing JSON object: " + std::string(e.what()));
                }
            }
        }
    }

private:
    void Deserialize(PatternPhrasesStorage& storage, const json& j)
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
                cluster.frequency = obj.at("1_frequency").get<double>();
                cluster.topicRelevance = obj.at("2_topic_relevance").get<double>();
                cluster.centralityScore = obj.at("3_centrality_score").get<double>();
                cluster.tagMatch = obj.at("4_tag_match").get<bool>();
                cluster.modelName = obj.at("5_model_name").get<std::string>();

                std::unordered_set<std::string> synonyms;
                if (obj.contains("9_synonyms")) {
                    cluster.synonyms = obj.at("9_synonyms").get<std::unordered_set<std::string>>();
                }

                // Deserialize Lemmas
                const json& lemmas_json = obj.at("6_lemmas");
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
                    cluster.hypernyms[lemmaStr] = lemma_obj.at("4_hypernyms").get<std::set<std::string>>();
                    cluster.hyponyms[lemmaStr] = lemma_obj.at("5_hyponyms").get<std::set<std::string>>();

                    // Add word embedding (assuming you need to create an embedding for each lemma)
                    cluster.wordVectors.push_back(std::make_shared<WordEmbedding>(lemmaStr));
                }

                // Deserialize WordComplexes (Phrases in your JSON)
                const json& phrases_json = obj.at("8_phrases");
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

                storage.AddCluster(key, cluster);
                // clusters[key] = cluster;
            }
        } catch (json::exception& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
            throw;
        } catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            throw;
        }
    }
};

#endif

#include "xmorphy/graphem/SentenceSplitter.h"
#include "xmorphy/graphem/Tokenizer.h"
#include "xmorphy/ml/SingleWordDisambiguate.h"
#include "xmorphy/ml/TFJoinedModel.h"
#include "xmorphy/ml/TFMorphemicSplitter.h"
#include "xmorphy/morph/Processor.h"
#include "xmorphy/utils/UniString.h"

#include "Logger.h"
#include "MorphAnalyzer.h"
#include "PhrasesCollectorUtils.h"
#include "TokenizedSentenceCorpus.h"

#include <string>

void TokenizedSentenceCorpus::build(const std::vector<fs::path>& files) {
    Logger::log("", LogLevel::Info, "Building and saving tokenized sentence corpus...");

    try {
        auto& morphAnalyzer = MorphAnalyzer::getInstance();

        for (unsigned int i = 0; i < files.size(); ++i) {
            size_t docNum = extractNumberFromPath(files[i].string());
            size_t sentNum = 0;
            X::Tokenizer tok;
            X::TFMorphemicSplitter morphemic_splitter;
            std::ifstream input = files[i];
            X::SentenceSplitter ssplitter(input);
            X::Processor analyzer;
            X::SingleWordDisambiguate disamb;
            X::TFJoinedModel joiner;

            do {
                std::string data;
                ssplitter.readSentence(data);
                if (data.empty())
                    continue;

                std::vector<X::TokenPtr> tokens = tok.analyze(X::UniString(data));
                std::vector<X::WordFormPtr> forms = analyzer.analyze(tokens);

                RemoveSeparatorTokens(forms);
                disamb.disambiguate(forms);
                joiner.disambiguateAndMorphemicSplit(forms);

                std::string normalizedData;

                for (auto& form : forms) {
                    morphemic_splitter.split(form);
                    if (form->getTokenType() != X::TokenTypeTag::WORD)
                        continue;
                    normalizedData.append(morphAnalyzer.getLemma(form) + " ");
                }
                if (!normalizedData.empty()) {
                    normalizedData.pop_back();
                    addSentence(docNum, sentNum, data, normalizedData);
                }
                sentNum++;
            } while (!ssplitter.eof());
        }

    } catch (const std::exception& e) {
        Logger::log("", LogLevel::Error, "Exception caught: " + std::string(e.what()));
    } catch (...) {
        Logger::log("", LogLevel::Error, "Unknown exception caught");
    }
    Logger::log("Main", LogLevel::Info, "Tokenized corpus build completed successfully.");
}

// Adds a sentence to the corpus.
void TokenizedSentenceCorpus::addSentence(const size_t docNum, const size_t sentNum, const std::string& data,
                                          const std::string& normalizedData) {
    TokenizedSentence sentence = {docNum, sentNum, data, normalizedData};
    sentenceMap[docNum][sentNum] = sentence; // Insert the sentence into the map
    sentencesCount++;
}

// Retrieves a sentence by document and sentence number.
const TokenizedSentence* TokenizedSentenceCorpus::getSentence(size_t docNum, size_t sentNum) const {
    auto docIt = sentenceMap.find(docNum);
    if (docIt != sentenceMap.end()) {
        auto sentIt = docIt->second.find(sentNum);
        if (sentIt != docIt->second.end()) {
            return &sentIt->second; // Return a pointer to the found sentence
        }
    }
    return nullptr; // Return nullptr if the sentence is not found
}

// Serializes the corpus data to JSON format.
json TokenizedSentenceCorpus::serialize() const {
    json j;
    j["totalSentences"] = sentencesCount;
    j["sentences"] = json::array();

    for (const auto& [docNum, sentencesMap] : sentenceMap) {
        for (const auto& [sentNum, sentence] : sentencesMap) {
            j["sentences"].push_back({{"docNum", sentence.docNum},
                                      {"sentNum", sentence.sentNum},
                                      {"originalStr", sentence.originalStr},
                                      {"normalizedStr", sentence.normalizedStr}});
        }
    }

    return j;
}

// Deserializes the corpus data from JSON format.
void TokenizedSentenceCorpus::deserialize(const json& j) {
    sentenceMap.clear(); // Clear existing data before loading new ones
    sentencesCount = j.at("totalSentences").get<int>();

    for (const auto& item : j.at("sentences")) {
        TokenizedSentence sentence;
        sentence.docNum = item.at("docNum").get<size_t>();
        sentence.sentNum = item.at("sentNum").get<size_t>();
        sentence.originalStr = item.at("originalStr").get<std::string>();
        sentence.normalizedStr = item.at("normalizedStr").get<std::string>();
        sentenceMap[sentence.docNum][sentence.sentNum] = sentence;
    }
}

// Saves the serialized corpus data to a file.
void TokenizedSentenceCorpus::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename + " for saving.");
    }

    json j = serialize();
    file << j.dump(4); // Dump JSON with indentation of 4 spaces for readability
    file.close();
}

// Loads the corpus data from a file, deserializes it, and updates the corpus.
void TokenizedSentenceCorpus::load(const std::string& filename) {
    Logger::log("TokenizedSentenceCorpus", LogLevel::Info, "Loading tokenized sentences from file: " + filename);
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename + " for loading.");
    }

    json j;
    file >> j;
    file.close();
    deserialize(j);

    Logger::log("TokenizedSentenceCorpus", LogLevel::Info,
                "Sentences loaded successfully. Total sentences: " + std::to_string(sentencesCount));
}
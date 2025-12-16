
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

    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    X::Tokenizer tokenizer;
    X::TFMorphemicSplitter morphemicSplitter;
    X::Processor analyzer;
    X::SingleWordDisambiguate disambiguater;
    X::TFJoinedModel joiner;

    try {
        for (const auto& file : files) {
            size_t docNum = util::path::extractNumberFromPath(file);
            size_t sentNum = 0;

            std::ifstream input{file};
            if (!input) {
                Logger::log("RawTextProcessor", LogLevel::Error, "Failed to open input file: " + file.string());
                return;
            }
            X::SentenceSplitter sentenceSplitter(input);

            while (!sentenceSplitter.eof()) {
                std::string rawSentence;
                sentenceSplitter.readSentence(rawSentence);

                if (rawSentence.empty())
                    continue;

                // Tokenization
                std::vector<X::TokenPtr> tokens = tokenizer.analyze(X::UniString(rawSentence));

                // Morphological analysis
                X::Sentence sentence = analyzer.analyze(tokens);

                // Дизамбигуация
                disambiguater.disambiguate(sentence);

                // Mорфемное разбиение и дизамбигуация
                joiner.disambiguateAndMorphemicSplit(sentence);

                std::string normalizedSentence;

                for (auto& token : sentence) {
                    morphemicSplitter.split(token);
                    if (token->getTokenType() == X::TokenTypeTag::WORD ||
                        token->getTokenType() == X::TokenTypeTag::WRNM) {
                        normalizedSentence.append(morphAnalyzer.getLemma(token) + " ");
                    }
                }
                if (!normalizedSentence.empty()) {
                    normalizedSentence.pop_back();
                    addSentence(docNum, sentNum, rawSentence, normalizedSentence);
                }
                sentNum++;
            };
        }

    } catch (const std::exception& e) {
        Logger::log("TokenizedSentenceCorpus", LogLevel::Error, "Exception caught: " + std::string(e.what()));
    } catch (...) {
        Logger::log("TokenizedSentenceCorpus", LogLevel::Error, "Unknown exception caught");
    }
    Logger::log("TokenizedSentenceCorpus", LogLevel::Info, "Tokenized corpus build completed successfully.");
}

// Adds a sentence to the corpus.
void TokenizedSentenceCorpus::addSentence(const size_t docNum, const size_t sentNum, const std::string& data,
                                          const std::string& normalizedData) {
    TokenizedSentence sentence = {docNum, sentNum, data, normalizedData};
    sentenceMap[docNum][sentNum] = sentence; // Insert the sentence into the map
    sentencesCount++;
}

// Retrieves a sentence by document and sentence number.
const std::optional<TokenizedSentence> TokenizedSentenceCorpus::getSentence(size_t docNum, size_t sentNum) const {
    auto docIt = sentenceMap.find(docNum);
    if (docIt != sentenceMap.end()) {
        auto sentIt = docIt->second.find(sentNum);
        if (sentIt != docIt->second.end()) {
            return sentIt->second; // Return a pointer to the found sentence
        }
    }
    return std::nullopt; // Return nullptr if the sentence is not found
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
#include "RawTextProcessor.h"
#include "xmorphy/graphem/SentenceSplitter.h"
#include "xmorphy/graphem/Tokenizer.h"
#include "xmorphy/ml/SingleWordDisambiguate.h"
#include "xmorphy/ml/TFJoinedModel.h"
#include "xmorphy/ml/TFMorphemicSplitter.h"
#include "xmorphy/morph/Processor.h"
#include "xmorphy/utils/UniString.h"

#include "ComplexPhrasesCollector.h"
#include "MorphAnalyzer.h"
#include "PhrasesCollectorUtils.h"
#include "SimplePhrasesCollector.h"
#include "TextCorpus.h"
#include "TextCorpusLoader.h"

#include "TopicManager.h"
#include <regex>

using json = nlohmann::json;

void RawTextProcessor::processRawData(const std::vector<fs::path>& files) {
    Logger::log("", LogLevel::Info, "Processing raw text data...");

    fs::path outputDir = options.resDir;
    fs::create_directories(outputDir);

    auto& corpus = TextCorpus::GetCorpus();

    X::Tokenizer tokenizer;
    X::TFMorphemicSplitter morphemicSplitter;
    X::Processor analyzer;
    X::SingleWordDisambiguate disambiguater;
    X::TFJoinedModel joiner;

    try {
        for (const auto& file : files) {
            Logger::log("RawTextProcessor", LogLevel::Info, "Processing file " + file.filename().string());

            // Load raw text into corpus
            corpus.LoadTextsFromFile(file);

            // Process file to extract phrases
            std::string filename = file.filename().replace_extension(".json").string();
            fs::path outputFile = outputDir / ("res_" + filename);

            std::ofstream outFile(outputFile);
            if (!outFile) {
                Logger::log("RawTextProcessor", LogLevel::Error, "Failed to create JSON file: " + outputFile.string());
                return;
            }
            outFile << "[]" << std::endl;
            Logger::log("RawTextProcessor", LogLevel::Debug, "Created empty JSON file: " + outputFile.string());

            Process processContext(file, outputFile);

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

                RemoveSeparatorTokens(sentence);
                disambiguater.disambiguate(sentence);
                joiner.disambiguateAndMorphemicSplit(sentence);

                for (auto& form : sentence) {
                    morphemicSplitter.split(form);
                }

                Logger::log("RawTextProcessor", LogLevel::Info, "Read sentence: " + rawSentence);
                collect(sentence, processContext);

                processContext.sentNum++;
            };
            finalizeDocumentProcessing();
        }

        // Save final corpus state to disk
        TextCorpusLoader::save(corpus, options.corpusFile.string());
        Logger::log("RawTextProcessor", LogLevel::Info, "Successfully saved corpus to: " + options.corpusFile.string());
    } catch (const std::exception& e) {
        Logger::log("RawTextProcessor", LogLevel::Error, "Exception caught: " + std::string(e.what()));
    } catch (...) {
        Logger::log("RawTextProcessor", LogLevel::Error, "Unknown exception caught");
    }
}

void RawTextProcessor::collect(const std::vector<WordFormPtr>& forms, Process& process) {
    auto& corpus = TextCorpus::GetCorpus();

    if (lastDocumentId != -1 && lastDocumentId != process.docNum) {
        for (const auto& lemma : uniqueLemmasInDoc) {
            corpus.UpdateDocumentFrequency(lemma);
        }
        uniqueLemmasInDoc.clear();
    }

    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    std::unordered_set<std::string> uniqueLemmasInSentence;
    for (const auto& form : forms) {
        std::string lemma = morphAnalyzer.getLemma(form);
        corpus.UpdateWordFrequency(lemma);
        uniqueLemmasInSentence.insert(lemma);
    }

    uniqueLemmasInDoc.insert(uniqueLemmasInSentence.begin(), uniqueLemmasInSentence.end());
    lastDocumentId = process.docNum;

    SimplePhrasesCollector simplePhrasesCollector(forms);
    simplePhrasesCollector.Collect(process);
    ComplexPhrasesCollector complexPhrasesCollector(simplePhrasesCollector.GetCollection(), forms);
    complexPhrasesCollector.Collect(process);
}

void RawTextProcessor::finalizeDocumentProcessing() {
    auto& corpus = TextCorpus::GetCorpus();
    for (const auto& lemma : uniqueLemmasInDoc) {
        corpus.UpdateDocumentFrequency(lemma);
    }
    uniqueLemmasInDoc.clear();
}
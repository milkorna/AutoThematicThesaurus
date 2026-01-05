#include "RawTextProcessor.h"

#include "ComplexPhrasesCollector.h"
#include "MorphAnalyzer.h"
#include "SimplePhrasesCollector.h"
#include "TextCorpus.h"
#include "TextCorpusLoader.h"
#include "TopicManager.h"

#include "xmorphy/graphem/SentenceSplitter.h"
#include "xmorphy/graphem/Tokenizer.h"
#include "xmorphy/ml/SingleWordDisambiguate.h"
#include "xmorphy/ml/TFJoinedModel.h"
#include "xmorphy/ml/TFMorphemicSplitter.h"
#include "xmorphy/morph/Processor.h"
#include "xmorphy/utils/UniString.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

void removeSeparators(std::vector<X::WordFormPtr>& sentence) {
    sentence.erase(
        std::remove_if(sentence.begin(), sentence.end(),
                       [](const X::WordFormPtr& form) { return form->getTokenType() == X::TokenTypeTag::SEPR; }),
        sentence.end());
}

void RawTextProcessor::processRawData(const std::vector<DocumentRecord>& documents) {
    Logger::log("RawTextProcessor", LogLevel::Info, "Processing " + std::to_string(documents.size()) + " documents...");

    if (documents.empty()) {
        Logger::log("RawTextProcessor", LogLevel::Warning, "No documents to process");
        return;
    }

    fs::path outputDir = options.resDir;
    fs::create_directories(outputDir);

    auto& corpus = TextCorpus::GetCorpus();

    X::Tokenizer tokenizer;
    X::TFMorphemicSplitter morphemicSplitter;
    X::Processor analyzer;
    X::SingleWordDisambiguate disambiguater;
    X::TFJoinedModel joiner;

    try {
        for (const auto& doc : documents) {
            Logger::log("RawTextProcessor", LogLevel::Info, "Processing document: " + doc.doc_id);

            // Получаем текст для обработки
            std::string textToProcess = doc.getProcessingText(options.mergeDocumentTitleAndText);

            if (textToProcess.empty()) {
                Logger::log("RawTextProcessor", LogLevel::Warning,
                            "Document " + doc.doc_id + " has empty processing text, skipping");
                continue;
            }

            // Создаем выходной файл для этого документа
            std::string outputFilename = doc.doc_id + "_res.json";
            fs::path outputFile = outputDir / outputFilename;

            std::ofstream outFile(outputFile);
            if (!outFile) {
                Logger::log("RawTextProcessor", LogLevel::Error, "Failed to create JSON file: " + outputFile.string());
                continue;
            }

            outFile << "[]" << std::endl;
            Logger::log("RawTextProcessor", LogLevel::Debug, "Created empty JSON file: " + outputFile.string());

            Process processContext(doc.doc_id, outputFile);

            // Передаем текст в SentenceSplitter через stringstream
            std::istringstream textStream(textToProcess);
            X::SentenceSplitter sentenceSplitter(textStream);

            while (!sentenceSplitter.eof()) {
                std::string rawSentence;
                sentenceSplitter.readSentence(rawSentence);

                if (rawSentence.empty())
                    continue;

                // Tokenization
                std::vector<X::TokenPtr> tokens = tokenizer.analyze(X::UniString(rawSentence));

                // Morphological analysis
                X::Sentence sentence = analyzer.analyze(tokens);

                removeSeparators(sentence);
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

    if (lastDocumentId.empty() && lastDocumentId != process.docId) {
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
    lastDocumentId = process.docId;

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
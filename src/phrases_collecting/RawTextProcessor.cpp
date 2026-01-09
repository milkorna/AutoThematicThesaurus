#include "RawTextProcessor.h"

#include "ComplexPhrasesCollector.h"
#include "CorpusVocabulary.h"
#include "MorphAnalyzer.h"
#include "Options.h"
#include "SimplePhrasesCollector.h"
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

void RawTextProcessor::processRawData(std::vector<Document>& documents) {
    Logger::log("RawTextProcessor", LogLevel::Info, "Processing " + std::to_string(documents.size()) + " documents...");

    if (documents.empty()) {
        Logger::log("RawTextProcessor", LogLevel::Warning, "No documents to process");
        return;
    }

    Options& options = Options::getOptions();
    fs::path outputDir = options.resDir;
    fs::create_directories(outputDir);

    CorpusVocabulary& corpus = CorpusVocabulary::GetCorpus();
    X::Tokenizer tokenizer;
    X::TFMorphemicSplitter morphemicSplitter;
    X::Processor analyzer;
    X::SingleWordDisambiguate disambiguater;
    X::TFJoinedModel joiner;

    try {
        for (auto& doc : documents) {
            Logger::log("RawTextProcessor", LogLevel::Info, "Processing document: " + doc.getDocId());

            // Получаем текст для обработки
            std::string textToProcess = doc.getText(options.mergeDocumentTitleAndText);

            if (textToProcess.empty()) {
                Logger::log("RawTextProcessor", LogLevel::Warning,
                            "Document " + doc.getDocId() + " has empty processing text, skipping");
                continue;
            }

            // Сохраняем character_count
            doc.setCharacterCount(countUTF8Characters(textToProcess));

            // Создаем выходной файл для этого документа
            std::string outputFilename = doc.getDocId() + "_res.json";
            fs::path outputFile = outputDir / outputFilename;

            std::ofstream outFile(outputFile);
            if (!outFile) {
                Logger::log("RawTextProcessor", LogLevel::Error, "Failed to create JSON file: " + outputFile.string());
                continue;
            }

            outFile << "[]" << std::endl;
            Logger::log("RawTextProcessor", LogLevel::Debug, "Created empty JSON file: " + outputFile.string());

            Process processContext(doc.getDocId(), outputFile);

            // Передаем текст в SentenceSplitter через stringstream
            std::istringstream textStream(textToProcess);
            X::SentenceSplitter sentenceSplitter(textStream);

            size_t globalOffsetInDocument = 0;
            bool firstSentence = true;

            while (!sentenceSplitter.eof()) {
                std::string rawSentence;
                sentenceSplitter.readSentence(rawSentence);

                if (rawSentence.empty())
                    continue;

                doc.incrementSentenceCount();

                // Tokenization
                std::vector<X::TokenPtr> tokens = tokenizer.analyze(X::UniString(rawSentence));

                std::vector<std::pair<size_t, size_t>> tokenSpans;
                tokenSpans.reserve(tokens.size()); // Зарезервировать память заранее для оптимизации

                for (const auto& token : tokens) {
                    // Пропускаем separators
                    if (token->getType() == X::TokenTypeTag::SEPR) {
                        continue;
                    }

                    // Используем готовые позиции из tokenizer - быстро и надёжно!
                    size_t startByte = token->getStartPosUnicode();
                    size_t lengthBytes = token->getLength();

                    tokenSpans.push_back({startByte, startByte + lengthBytes});
                }

                // Передаём информацию в Process
                processContext.setSentenceData(rawSentence, tokenSpans, globalOffsetInDocument);

                // Morphological analysis
                X::Sentence sentence = analyzer.analyze(tokens);

                removeSeparators(sentence);
                disambiguater.disambiguate(sentence);
                joiner.disambiguateAndMorphemicSplit(sentence);

                for (auto& form : sentence) {
                    morphemicSplitter.split(form);
                }

                doc.incrementWordCount(sentence.size());

                Logger::log("RawTextProcessor", LogLevel::Info, "Read sentence: " + rawSentence);
                collect(sentence, processContext, doc);

                globalOffsetInDocument += tokens.back()->getStartPosUnicode() + tokens.back()->getLength();
                if (options.mergeDocumentTitleAndText && firstSentence) {
                    firstSentence = false;
                    globalOffsetInDocument++;
                }
                processContext.nextSentence();
            };

            for (const auto& lemma : doc.getUniqueLemmas()) {
                corpus.updateDocumentFrequency(lemma);
            }
            corpus.incrementDocumentCount();

            Logger::log("RawTextProcessor", LogLevel::Info,
                        "Document " + doc.getDocId() + " COMPLETE: " + "sentences=" +
                            std::to_string(doc.getSentenceCount()) + " words=" + std::to_string(doc.getWordCount()) +
                            " lemmas=" + std::to_string(doc.getUniqueLemmasCount()) +
                            " chars=" + std::to_string(doc.getCharacterCount()));
        }

        // Save final corpus state to disk
        corpus.save(options.corpusFile.string());
        Logger::log("RawTextProcessor", LogLevel::Info, "Successfully saved corpus to: " + options.corpusFile.string());
    } catch (const std::exception& e) {
        Logger::log("RawTextProcessor", LogLevel::Error, "Exception caught: " + std::string(e.what()));
    } catch (...) {
        Logger::log("RawTextProcessor", LogLevel::Error, "Unknown exception caught");
    }
}

void RawTextProcessor::collect(const std::vector<X::WordFormPtr>& forms, Process& process, Document& currentDoc) {
    auto& corpus = CorpusVocabulary::GetCorpus();
    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    std::unordered_set<std::string> uniqueLemmasInSentence;
    for (const auto& form : forms) {
        std::string lemma = morphAnalyzer.getLemma(form);

        corpus.updateWordFrequency(lemma);

        // ОБНОВЛЯЕМ локальную статистику документа
        currentDoc.incrementWordFrequency(lemma);

        uniqueLemmasInSentence.insert(lemma);
    }

    // Сохраняем уникальные леммы документа
    currentDoc.addUniqueLemmasFromSentence(uniqueLemmasInSentence);

    SimplePhrasesCollector simplePhrasesCollector(forms);
    simplePhrasesCollector.collect(process);
    ComplexPhrasesCollector complexPhrasesCollector(simplePhrasesCollector.getCollection(), forms);
    complexPhrasesCollector.collect(process);
}
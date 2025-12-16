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

void RawTextProcessor::processRawData() {
    Logger::log("", LogLevel::Info, "Processing raw text data...");

    fs::path outputDir = options.resDir;
    fs::create_directories(outputDir);

    auto& corpus = TextCorpus::GetCorpus();

    try {
        std::vector<fs::path> filesToProcess = GetFilesToProcess();
        Logger::log("RawTextProcessor", LogLevel::Info,
                    "Found " + std::to_string(filesToProcess.size()) + " files to process");

        // Process each file
        for (unsigned int i = 0; i < filesToProcess.size(); ++i) {
            Logger::log("RawTextProcessor", LogLevel::Info,
                        "Processing file " + std::to_string(i + 1) + "/" + std::to_string(filesToProcess.size()) +
                            ": " + filesToProcess[i].filename().string());

            // Load raw text into corpus
            corpus.LoadTextsFromFile(filesToProcess[i]);

            // Process file to extract phrases
            processFile(filesToProcess[i], outputDir);
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

void RawTextProcessor::processFile(const fs::path& inputFile, const fs::path& outputDir) {
    std::string filename = inputFile.filename().replace_extension(".json").string();
    fs::path outputFile = outputDir / ("res_" + filename);

    std::ofstream outFile(outputFile);
    if (!outFile) {
        Logger::log("RawTextProcessor", LogLevel::Error, "Failed to create JSON file: " + outputFile.string());
        return;
    }
    outFile << "[]" << std::endl;
    Logger::log("RawTextProcessor", LogLevel::Debug, "Created empty JSON file: " + outputFile.string());

    X::Tokenizer tokenizer;
    X::TFMorphemicSplitter morphemicSplitter;
    Process processContext(inputFile, outputFile);

    std::ifstream input(inputFile);
    if (!input) {
        Logger::log("RawTextProcessor", LogLevel::Error, "Failed to open input file: " + inputFile.string());
        return;
    }

    X::SentenceSplitter sentenceSplitter(input);
    X::Processor analyzer;
    X::SingleWordDisambiguate disamb;
    X::TFJoinedModel joiner;

    std::string sentence;
    while (!sentenceSplitter.eof()) {
        sentenceSplitter.readSentence(sentence);

        if (sentence.empty())
            continue;

        // Tokenization
        std::vector<X::TokenPtr> tokens = tokenizer.analyze(X::UniString(sentence));

        // Morphological analysis
        X::Sentence forms = analyzer.analyze(tokens);

        RemoveSeparatorTokens(forms);
        disamb.disambiguate(forms);
        joiner.disambiguateAndMorphemicSplit(forms);

        for (auto& form : forms) {
            morphemicSplitter.split(form);
        }

        Logger::log("RawTextProcessor", LogLevel::Info, "Read sentence: " + sentence);
        collect(forms, processContext);

        processContext.sentNum++;
    };
    finalizeDocumentProcessing();
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
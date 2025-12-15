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

void RawTextProcessor::processFile(const fs::path& inputFile, const fs::path& outputDir) {
    std::string filename = inputFile.filename().replace_extension(".json").string();
    fs::path outputFile = outputDir / ("res_" + filename);

    std::ofstream outFile(outputFile);
    if (!outFile) {
        Logger::log("ProcessFile", LogLevel::Error, "Failed to create JSON file: " + outputFile.string());
        return;
    }
    outFile << "[]" << std::endl;
    Logger::log("ProcessFile", LogLevel::Debug, "Created empty JSON file: " + outputFile.string());

    X::Tokenizer tok;
    X::TFMorphemicSplitter morphemic_splitter;
    Process process(inputFile, outputFile);
    std::ifstream input(inputFile);
    if (!input) {
        Logger::log("ProcessFile", LogLevel::Error, "Failed to open input file: " + inputFile.string());
        return;
    }
    X::SentenceSplitter ssplitter(input);
    X::Processor analyzer;
    X::SingleWordDisambiguate disamb;
    X::TFJoinedModel joiner;

    do {
        std::string sentence;
        ssplitter.readSentence(sentence);
        if (sentence.empty())
            continue;

        std::vector<X::TokenPtr> tokens = tok.analyze(X::UniString(sentence));
        std::vector<X::WordFormPtr> forms = analyzer.analyze(tokens);

        RemoveSeparatorTokens(forms);
        disamb.disambiguate(forms);
        joiner.disambiguateAndMorphemicSplit(forms);

        for (auto& form : forms) {
            morphemic_splitter.split(form);
        }

        Logger::log("SentenceReading", LogLevel::Info, "Read sentence: " + sentence);
        collect(forms, process);

        process.sentNum++;
    } while (!ssplitter.eof());
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

void RawTextProcessor::processRawData() {
    auto& options = Options::getOptions();
    Logger::log("", LogLevel::Info, "Building phrase storage...");
    fs::path outputDir = options.resDir;
    fs::create_directories(outputDir);

    auto& corpus = TextCorpus::GetCorpus();
    try {
        std::vector<fs::path> files_to_process = GetFilesToProcess();

        for (unsigned int i = 0; i < files_to_process.size(); ++i) {
            corpus.LoadTextsFromFile(files_to_process[i]);
            processFile(files_to_process[i], outputDir);
        }

        TextCorpusLoader::save(corpus, options.corpusFile.string());
    } catch (const std::exception& e) {
        Logger::log("", LogLevel::Error, "Exception caught: " + std::string(e.what()));
    } catch (...) {
        Logger::log("", LogLevel::Error, "Unknown exception caught");
    }
}

void RawTextProcessor::finalizeDocumentProcessing() {
    auto& corpus = TextCorpus::GetCorpus();
    for (const auto& lemma : uniqueLemmasInDoc) {
        corpus.UpdateDocumentFrequency(lemma);
    }
    uniqueLemmasInDoc.clear();
}
#include <boost/program_options.hpp>
#include <xmorphy/graphem/SentenceSplitter.h>
#include <xmorphy/graphem/Tokenizer.h>
#include <xmorphy/ml/Disambiguator.h>
#include <xmorphy/ml/MorphemicSplitter.h>
#include <xmorphy/ml/SingleWordDisambiguate.h>
#include <xmorphy/ml/TFDisambiguator.h>
#include <xmorphy/ml/TFJoinedModel.h>
#include <xmorphy/ml/TFMorphemicSplitter.h>
#include <xmorphy/morph/JSONEachSentenceFormater.h>
#include <xmorphy/morph/PrettyFormater.h>
#include <xmorphy/morph/Processor.h>
#include <xmorphy/morph/TSVFormater.h>
#include <xmorphy/morph/WordFormPrinter.h>
#include <xmorphy/utils/UniString.h>

#include <Component.h>
#include <Logger.h>
#include <OutputRedirector.h>
#include <PatternPhrasesStorage.h>

#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

void removeSeparatorTokens(std::vector<WordFormPtr>& forms)
{
    forms.erase(std::remove_if(forms.begin(), forms.end(),
                               [](const WordFormPtr& form) { return form->getTokenType() == TokenTypeTag::SEPR; }),
                forms.end());
}

void processText(const std::string& inputFile, const std::string& outputFile)
{
    OutputRedirector redir("log.txt");
    Process process(inputFile, outputFile);

    Tokenizer tok;
    TFMorphemicSplitter morphemic_splitter;
    SentenceSplitter ssplitter(process.m_input);
    Processor analyzer;
    SingleWordDisambiguate disamb;
    TFJoinedModel joiner;
    redir.restore();

    do {
        std::string sentence;
        ssplitter.readSentence(sentence);

        if (sentence.empty())
            continue;

        std::vector<TokenPtr> tokens = tok.analyze(UniString(sentence));
        std::vector<WordFormPtr> forms = analyzer.analyze(tokens);

        OutputRedirector redirector("log.txt");

        removeSeparatorTokens(forms);
        disamb.disambiguate(forms);
        joiner.disambiguateAndMorphemicSplit(forms);

        for (auto& form : forms) {
            morphemic_splitter.split(form);
        }

        redirector.restore();

        Logger::log("SentenceReading", LogLevel::Info, "Read sentence: " + sentence);
        Logger::log("TokenAnalysis", LogLevel::Debug, "Token count: " + std::to_string(tokens.size()));
        Logger::log("FormAnalysis", LogLevel::Debug, "Form count: " + std::to_string(forms.size()));

        PatternPhrasesStorage::GetStorage().Collect(forms, process);

        process.m_output.flush();
        process.m_sentNum++;
    } while (!ssplitter.eof());
}

int main()
{
    Logger::enableLogging(true);
    Logger::setGlobalLogLevel(LogLevel::Info);

    fs::path repoPath = fs::current_path();
    fs::path inputDir = repoPath / "my_data/texts";
    fs::path outputDir = repoPath / "res";

    fs::create_directories(outputDir);

    try {
        std::string filePath = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/patterns.txt";
        GrammarPatternManager::GetManager()->readPatterns(filePath);
        // GrammarPatternManager::GetManager()->printPatterns();
    } catch (const std::exception& e) {
        Logger::log("main", LogLevel::Error, "Exception caught: " + std::string(e.what()));
        return EXIT_FAILURE;
    } catch (...) {
        Logger::log("main", LogLevel::Error, "Unknown exception caught");
        return EXIT_FAILURE;
    }

    try {

        for (const auto& entry : fs::directory_iterator(inputDir)) {
            if (entry.is_regular_file()) {
                std::string inputFile = entry.path().string();
                std::string filename = entry.path().filename().string();

                if (filename.find("art") == 0 && filename.find("_text.txt") != std::string::npos) {
                    std::string outputFile = (outputDir / ("res_" + filename)).string();

                    auto start = std::chrono::high_resolution_clock::now();
                    processText(inputFile, outputFile);
                    auto end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> duration = end - start;

                    Logger::log("main", LogLevel::Info,
                                "processText() for " + filename + " took " + std::to_string(duration.count()) +
                                    " seconds.");
                }
            }
        }
    } catch (const std::exception& e) {
        Logger::log("main", LogLevel::Error, "Exception caught: " + std::string(e.what()));
        return EXIT_FAILURE;
    } catch (...) {
        Logger::log("main", LogLevel::Error, "Unknown exception caught");
        return EXIT_FAILURE;
    }

    return 0;
}

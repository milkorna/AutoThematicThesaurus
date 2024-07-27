#include <vector>
#include <memory>
#include <string>
#include <filesystem>
#include <GrammarPatternManager.h>
#include <Logger.h>
#include <TermProposalStorage.h>
#include <GrammarComponent.h>
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

void removeSeparatorTokens(std::vector<WordFormPtr> &forms)
{
    forms.erase(
        std::remove_if(forms.begin(), forms.end(),
                       [](const WordFormPtr &form)
                       {
                           return form->getTokenType() == TokenTypeTag::SEPR;
                       }),
        forms.end());
}

void processText(const std::string &inputFile, const std::string &outputFile)
{
    Process process(inputFile, outputFile);

    Tokenizer tok;
    TFMorphemicSplitter morphemic_splitter;
    SentenceSplitter ssplitter(process.m_input);
    Processor analyzer;
    SingleWordDisambiguate disamb;
    TFJoinedModel joiner;

    do
    {
        std::string sentence;
        ssplitter.readSentence(sentence);

        if (sentence.empty())
            continue;

        std::vector<TokenPtr> tokens = tok.analyze(UniString(sentence));
        std::vector<WordFormPtr> forms = analyzer.analyze(tokens);

        removeSeparatorTokens(forms);
        disamb.disambiguate(forms);
        joiner.disambiguateAndMorphemicSplit(forms);

        for (auto &form : forms)
        {
            morphemic_splitter.split(form);
            // Logger::log("MorphemicSplit", LogLevel::Debug, "Morphemic split applied on normal form: " + form->getWordForm().getRawString());
        }

        Logger::log("SentenceReading", LogLevel::Debug, "Read sentence: " + sentence);
        Logger::log("TokenAnalysis", LogLevel::Debug, "Token count: " + std::to_string(tokens.size()));
        Logger::log("FormAnalysis", LogLevel::Debug, "Form count: " + std::to_string(forms.size()));

        const auto &wcCollection = WCModelCollection::getInstance();
        wcCollection->collect(forms, process);

        process.m_output.flush();
        process.m_sentNum++;

    } while (!ssplitter.eof());
}

int main()
{
    Logger::enableLogging(true);
    Logger::setGlobalLogLevel(LogLevel::Debug); // Setting the global logging level
    // Logger::setModuleLogLevel("MyClass", LogLevel::Debug); // Setting the logging level for a specific module

    // auto &dictionary = TermDictionary::getInstance();
    Logger::log("main", LogLevel::Debug, "Current path is " + std::string(std::filesystem::current_path()));

    const auto &manager = GrammarPatternManager::getInstance();

    try
    {
        std::string filePath = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/patterns.txt";

        manager->readPatterns(filePath);
        manager->printPatterns();
    }
    catch (const std::exception &e)
    {
        // std::cerr << "Exception caught: " << e.what() << std::endl;
        Logger::log("main", LogLevel::Error, "Exception caught: " + std::string(e.what()));
        return EXIT_FAILURE;
    }
    catch (...)
    {
        Logger::log("main", LogLevel::Error, "Unknown exception caught");
        return EXIT_FAILURE;
    }

    std::string inputFile = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/texts/art325014_text.txt";
    std::string outputFile = "/home/milkorna/Documents/AutoThematicThesaurus/res/res_art325014_text.txt";

    try
    {
        processText(inputFile, outputFile);
    }
    catch (const std::exception &e)
    {
        Logger::log("main", LogLevel::Error, "Exception caught: " + std::string(e.what()));
        return EXIT_FAILURE;
    }
    catch (...)
    {
        Logger::log("main", LogLevel::Error, "Unknown exception caught");
        return EXIT_FAILURE;
    }

    return 0;
}

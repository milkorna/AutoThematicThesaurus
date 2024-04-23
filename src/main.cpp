#include <vector>
#include <memory>
#include <string>
#include <filesystem>
#include <GrammarPatternManager.h>
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

using namespace X;
using namespace std;

struct Options
{
    bool disambiguate = true;
    bool context_disambiguate = true;
    bool morphemic_split = true;
};

int main()
{
    // auto &dictionary = TermDictionary::getInstance();
    std::cout << "Current path is " << std::filesystem::current_path() << '\n';

    const auto &manager = GrammarPatternManager::getInstance();

    try
    {
        std::string filePath = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/patterns.txt";

        manager->readPatterns(filePath);
        manager->printPatterns();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught" << std::endl;
        return EXIT_FAILURE;
    }

    Options opts;

    std::string input_file;
    std::string output_file;

    std::istream *is = new ifstream(input_file);
    std::ostream *os = new ofstream(output_file);

    Tokenizer tok;
    TFDisambiguator tf_disambig;
    TFMorphemicSplitter morphemic_splitter;
    SentenceSplitter ssplitter(*is);
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

        disamb.disambiguate(forms);
        bool joined_model_failed = true;
        joined_model_failed = !joiner.disambiguateAndMorphemicSplit(forms); //?

        if (joined_model_failed)
        {
            tf_disambig.disambiguate(forms);
            for (auto &form : forms)
            {
                morphemic_splitter.split(form);
            }
        }

        const auto &wcCollection = WCModelCollection::getInstance();
        wcCollection->collect(forms);

        os->flush();
    } while (!ssplitter.eof());

    return 0;
}

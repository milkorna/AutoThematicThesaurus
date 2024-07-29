#ifndef PHRASES_COLLECTOR_UTILS_H
#define PHRASES_COLLECTOR_UTILS_H

#include <xmorphy/morph/WordForm.h>

#include <GrammarComponent.h>
#include <Logger.h>
#include <PatternParser.h>
#include <PhrasesCollectorUtils.h>

#include <deque>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

namespace PhrasesCollectorUtils {

    struct Position {
        size_t start;
        size_t end;
        size_t docNum;
        size_t sentNum;
    };

    struct WordComplex {
        std::deque<X::WordFormPtr> words = {};
        std::string textForm = "";
        Position pos;
        std::string modelName;
    };

    struct WordComplexAgregate {
        size_t size;
        std::vector<WordComplex> wordComplexes; // maybe set
        std::string form;
        Components comps;
        double m_weight;
    };

    using WordComplexAgregates = std::unordered_map<std::string, WordComplexAgregate>;
    // string -- seq of word in normalized form
    using WordComplexCollection = std::vector<WordComplexAgregates>;

    using WordComplexPtr = std::shared_ptr<WordComplex>;

    struct CurrentPhraseStatus {
        size_t correct = 0;
        bool headIsMatched = false;
        bool headIsChecked = false;
        bool foundLex = false;
        bool foundTheme = false;
    };

    void LogCurrentSimplePhrase(const WordComplexPtr& curSimplePhr);
    void LogCurrentComplexModel(const std::string& name);

    void UpdateWordComplex(const WordComplexPtr& wc, const X::WordFormPtr& form, const std::string& formFromText,
                           bool isLeft);
    void AddWordsToFront(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase);
    void AddWordsToBack(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase);
    void UpdatePhraseStatus(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase,
                            CurrentPhraseStatus& curPhrStatus, bool isLeft);

    void OutputResults(const std::vector<WordComplexPtr>& collection, Process& process);
}

namespace PHUtils = PhrasesCollectorUtils;

#endif
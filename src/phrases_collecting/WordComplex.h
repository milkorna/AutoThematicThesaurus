#ifndef WORD_COMPLEX_H
#define WORD_COMPLEX_H

#include <xmorphy/morph/WordForm.h>

#include <ModelComponent.h>
#include <PatternParser.h>
#include <PhrasesCollectorUtils.h>
#include <WordComplex.h>

#include <deque>
#include <memory>
#include <string>

namespace PhrasesCollectorUtils {

    struct Position {
        size_t start;
        size_t end;
        size_t docNum;
        size_t sentNum;
    };

    class WordComplex {
    public:
        std::deque<X::WordFormPtr> words = {};
        std::string textForm = "";
        Position pos;
        std::string modelName;

        bool operator==(const WordComplex& other) const;

        const std::string GetKey() const;
    };

    using WordComplexPtr = std::shared_ptr<WordComplex>;

    WordComplexPtr InicializeWordComplex(const WordComplexPtr& curSimplePhr, const std::string& modelName);
    WordComplexPtr InicializeWordComplex(const size_t tokenInd, const WordFormPtr token, const std::string modelName,
                                         const Process& process);

    void UpdateWordComplex(const WordComplexPtr& wc, const WordFormPtr& form, const std::string& formFromText,
                           bool isLeft);

    void AddWordsToFront(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase);
    void AddWordsToBack(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase);
}

namespace PHUtils = PhrasesCollectorUtils;

#endif // WORD_COMPLEX_H
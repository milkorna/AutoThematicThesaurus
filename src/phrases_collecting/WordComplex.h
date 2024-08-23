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

    // \struct Position
    // \brief This structure represents the position of a word or phrase within a document.
    struct Position {
        size_t start;   ///< Start position of the word or phrase.
        size_t end;     ///< End position of the word or phrase.
        size_t docNum;  ///< Document number.
        size_t sentNum; ///< Sentence number.
    };

    // \class WordComplex
    // \brief This class represents a complex word or phrase, including its text form, position, and model name.
    class WordComplex {
    public:
        std::deque<X::WordFormPtr> words = {}; ///< Deque containing word forms.
        std::deque<std::string> lemmas = {};
        std::string textForm = ""; ///< Text form of the word or phrase.
        Position pos;              ///< Position of the word or phrase within the document.
        std::string modelName;     ///< Name of the model associated with the word or phrase.

        // \brief Equality operator for comparing two WordComplex objects.
        // \param other     The other WordComplex object to compare with.
        // \return          True if the objects are equal, false otherwise.
        bool operator==(const WordComplex& other) const;

        // \brief Gets a key representing the WordComplex object.
        // \return          A string key representing the WordComplex object.
        const std::string GetKey() const;
    };

    using WordComplexPtr = std::shared_ptr<WordComplex>;

    // \brief Initializes a WordComplex object with the given parameters.
    // \param curSimplePhr  A pointer to the current simple phrase.
    // \param modelName     The name of the model.
    // \return              A shared pointer to the initialized WordComplex object.
    WordComplexPtr InicializeWordComplex(const WordComplexPtr& curSimplePhr, const std::string& modelName);

    // \brief Initializes a WordComplex object with the given parameters.
    // \param tokenInd      The index of the token.
    // \param token         The WordFormPtr token.
    // \param modelName     The name of the model.
    // \param process       The process associated with the initialization.
    // \return              A shared pointer to the initialized WordComplex object.
    WordComplexPtr InicializeWordComplex(const size_t tokenInd, const WordFormPtr token, const std::string modelName,
                                         const Process& process);

    // \brief Updates a WordComplex object with the given form and text form.
    // \param wc            A shared pointer to the WordComplex object to update.
    // \param form          The WordFormPtr form to add.
    // \param formFromText  The form from the text to add.
    // \param isLeft        A boolean indicating if the form is added to the left.
    void UpdateWordComplex(const WordComplexPtr& wc, const WordFormPtr& form, const std::string& formFromText,
                           bool isLeft);

    // \brief Adds words to the front of a WordComplex object.
    // \param wc            A shared pointer to the WordComplex object to update.
    // \param asidePhrase   A shared pointer to the aside phrase to add.
    void AddWordsToFront(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase);

    // \brief Adds words to the back of a WordComplex object.
    // \param wc            A shared pointer to the WordComplex object to update.
    // \param asidePhrase   A shared pointer to the aside phrase to add.
    void AddWordsToBack(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase);
}

namespace PHUtils = PhrasesCollectorUtils;

#endif // WORD_COMPLEX_H
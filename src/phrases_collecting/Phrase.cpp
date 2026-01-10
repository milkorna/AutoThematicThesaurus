#include "Phrase.h"

#include "MorphAnalyzer.h"
#include "Process.h"

PhrasePtr Phrase::createFromPhrase(const PhrasePtr& basePhrase, const std::string& modelName) {
    PhrasePtr wc = std::make_shared<Phrase>();
    wc->words = basePhrase->words;
    wc->lemmas = basePhrase->lemmas;
    wc->textForm = basePhrase->textForm;
    wc->pos = basePhrase->pos;
    wc->modelName = modelName;
    return wc;
}

PhrasePtr Phrase::createFromToken(const size_t tokenInd, const X::WordFormPtr token, const std::string modelName,
                                  const Process& process) {
    auto& morphAnalyzer = MorphAnalyzer::getInstance();
    PhrasePtr wc = std::make_shared<Phrase>();
    wc->words.push_back(token);
    wc->lemmas.push_back(morphAnalyzer.getLemma(token));
    wc->textForm = token->getWordForm().getRawString();
    wc->pos.start = tokenInd;
    wc->pos.end = tokenInd;
    wc->pos.docId = process.getDocId();
    wc->pos.sentNum = process.getSentNum();
    // charStart и charEnd будут установлены позже в Process::outputResults()
    wc->modelName = modelName;

    return wc;
}

void Phrase::addWordToLeft(const X::WordFormPtr& wordForm) {
    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    words.emplace_front(wordForm);
    lemmas.emplace_front(morphAnalyzer.getLemma(wordForm));
    pos.start--;
    textForm.insert(0, wordForm->getWordForm().getRawString() + " ");
}

void Phrase::addWordToRight(const X::WordFormPtr& wordForm) {
    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    words.emplace_back(wordForm);
    lemmas.emplace_back(morphAnalyzer.getLemma(wordForm));
    pos.end++;
    textForm.append(" " + wordForm->getWordForm().getRawString());
}

void Phrase::mergeLeft(const PhrasePtr& otherPhrase) {
    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    for (auto rit = otherPhrase->words.rbegin(); rit != otherPhrase->words.rend(); ++rit) {
        words.emplace_front(*rit);
        lemmas.emplace_front(morphAnalyzer.getLemma(*rit));
    }
}

void Phrase::mergeRight(const PhrasePtr& otherPhrase) {
    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    for (const auto& word : otherPhrase->words) {
        words.emplace_back(word);
        lemmas.emplace_back(morphAnalyzer.getLemma(word));
    }
}

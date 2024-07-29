#include <PhrasesCollectorUtils.h>

using namespace X;

namespace PhrasesCollectorUtils {

    void LogCurrentSimplePhrase(const WordComplexPtr& curSimplePhr)
    {
        Logger::log("CURRENT SIMPLE PHRASE", LogLevel::Info, curSimplePhr->textForm + " || " + curSimplePhr->modelName);
    }

    void LogCurrentComplexModel(const std::string& name)
    {
        Logger::log("CURRENT COMPLEX MODEL", LogLevel::Info, name);
    }

    void UpdateWordComplex(const WordComplexPtr& wc, const WordFormPtr& form, const std::string& formFromText,
                           bool isLeft)
    {
        if (isLeft) {
            wc->words.push_front(form);
            wc->pos.start--;
            wc->textForm.insert(0, formFromText + " ");
        } else {
            wc->words.push_back(form);
            wc->pos.end++;
            wc->textForm.append(" " + formFromText);
        }
    }

    void AddWordsToFront(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase)
    {
        for (auto rit = asidePhrase->words.rbegin(); rit != asidePhrase->words.rend(); ++rit) {
            wc->words.push_front(*rit);
        }
    }

    void AddWordsToBack(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase)
    {
        for (const auto& word : asidePhrase->words) {
            wc->words.push_back(word);
        }
    }

    void UpdatePhraseStatus(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase,
                            CurrentPhraseStatus& curPhrStatus, bool isLeft)
    {
        curPhrStatus.correct++;
        if (isLeft) {
            wc->pos.start = asidePhrase->pos.start;
            wc->textForm.insert(0, asidePhrase->textForm + " ");
        } else {
            wc->pos.end = asidePhrase->pos.end;
            wc->textForm.append(" " + asidePhrase->textForm);
        }
    }

    void OutputResults(const std::vector<WordComplexPtr>& collection, Process& process)
    {
        for (const auto& wc : collection) {
            process.m_output << process.m_docNum << " " << process.m_sentNum << " start_ind = " << wc->pos.start
                             << " end_ind = " << wc->pos.end << "\t||\t" << wc->textForm << "\t||\t" << wc->modelName
                             << std::endl;
        }
    }
}
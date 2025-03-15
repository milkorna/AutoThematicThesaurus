#include <PatternPhrasesStorage.h>
#include <SimplePhrasesCollector.h>
#include <StringFilters.h>

#include <utility>

using namespace PhrasesCollectorUtils;

static bool HeadCheck(const std::shared_ptr<Model>& simpleModel, const X::WordFormPtr& form)
{
    if (!simpleModel->getHead()->getCondition().check(simpleModel->getHead()->getSPTag(), form)) {
        return false;
    }
    return true;
}

static bool HaveSpHead(const std::unordered_set<X::MorphInfo>& currFormMorphInfo)
{
    for (const auto& morphForm : currFormMorphInfo) {
        Logger::log("HaveSpHead", LogLevel::Debug, "MorphForm: " + morphForm.normalForm.getRawString());
        const auto& spSet = GrammarPatternManager::GetManager()->getUsedHeadSp();
        if (spSet.find(morphForm.sp.toString()) == spSet.end()) {
            Logger::log("HaveSpHead", LogLevel::Debug, "No head with " + morphForm.sp.toString() + " speach of word");
        } else {
            Logger::log("HaveSpHead", LogLevel::Debug,
                        "Found head with " + morphForm.sp.toString() + " speach of word");
            return true;
        }
    }
    return false;
}

bool SimplePhrasesCollector::CheckAside(const std::shared_ptr<WordComplex>& wc, const std::shared_ptr<Model>& model,
                                        size_t compIndex, size_t tokenInd, size_t& correct, const bool isLeft)
{
    const auto& comp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[compIndex]);
    const auto& token = m_sentence[tokenInd];

    if (g_options.cleanStopWords) {
        const auto& stopWords = GetStopWords();

        if (stopWords.find(token->getWordForm().toLowerCase().getRawString()) != stopWords.end())
            return false;

        const auto normalForm = GetLemma(token);
        if (stopWords.find(normalForm) != stopWords.end())
            return false;
    }

    if (StringFilters::CheckForMisclassifications(token) || MorphAnanlysisError(token) ||
        !HaveSp(token->getMorphInfo()))
        return false;

    std::string formFromText = token->getWordForm().getRawString();

    if (!comp->getCondition().check(comp->getSPTag(), token))
        return false;
    UpdateWordComplex(wc, token, formFromText, isLeft);

    ++correct;
    size_t nextCompIndex = isLeft ? compIndex - 1 : compIndex + 1;
    size_t nextTokenInd = isLeft ? tokenInd - 1 : tokenInd + 1;

    if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->size() - 1)) {
        CheckAside(wc, model, nextCompIndex, nextTokenInd, correct, isLeft);
    } else {
        m_collection.push_back(std::make_shared<WordComplex>(*wc));
        if (comp->isRec() && ((isLeft && tokenInd > 0) || (!isLeft && tokenInd < m_sentence.size() - 1))) {
            if (CheckAside(wc, model, compIndex, nextTokenInd, correct, isLeft)) {
                return true;
            } else {
                return false;
            }
        }
    }

    return false;
}

void SimplePhrasesCollector::Collect(Process& process)
{
    const auto& simplePatterns = manager.getSimplePatterns();

    for (size_t tokenInd = 0; tokenInd < m_sentence.size(); tokenInd++) {
        const auto token = m_sentence[tokenInd];

        if (g_options.cleanStopWords) {
            const auto& stopWords = GetStopWords();

            if (stopWords.find(token->getWordForm().toLowerCase().getRawString()) != stopWords.end())
                continue;

            const auto normalForm = GetLemma(token);
            if (stopWords.find(normalForm) != stopWords.end())
                continue;
        }

        if (StringFilters::CheckForMisclassifications(token) || MorphAnanlysisError(token) ||
            !HaveSp(token->getMorphInfo()))
            continue;

        if (!HaveSpHead(token->getMorphInfo()))
            continue;

        for (const auto& [name, model] : simplePatterns) {

            if (!HeadCheck(model, token))
                continue;

            size_t headPos = *model->getHeadPos();
            size_t correct = 0;

            WordComplexPtr wc = InicializeWordComplex(tokenInd, token, model->getForm(), process);
            ++correct;

            if (headPos != 0 && tokenInd != 0 && CheckAside(wc, model, headPos - 1, tokenInd - 1, correct, true)) {
                break;
            }

            if (headPos != model->size() - 1 && CheckAside(wc, model, headPos + 1, tokenInd + 1, correct, false)) {
                break;
            }
        }
    }
    OutputResults(m_collection, process);
}

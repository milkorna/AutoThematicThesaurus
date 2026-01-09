#include "SimplePhrasesCollector.h"
#include "GrammarPatternManager.h"
#include "Logger.h"
#include "MorphAnalyzer.h"
#include "Options.h"
#include "Process.h"
#include "StopWordsManager.h"
#include "StringUtils.h"

static bool HaveSpHead(const std::unordered_set<X::MorphInfo>& currFormMorphInfo) {
    const auto& manager = GrammarPatternManager::GetManager();

    for (const auto& morphForm : currFormMorphInfo) {
        Logger::log("HaveSpHead", LogLevel::Debug, "MorphForm: " + morphForm.normalForm.getRawString());
        const auto& spSet = manager.getUsedHeadSp();
        if (!spSet.contains(morphForm.sp.toString())) {
            Logger::log("HaveSpHead", LogLevel::Debug, "No head with " + morphForm.sp.toString() + " speach of word");
        } else {
            Logger::log("HaveSpHead", LogLevel::Debug,
                        "Found head with " + morphForm.sp.toString() + " speech of word");
            return true;
        }
    }
    return false;
}

bool SimplePhrasesCollector::checkAside(const std::shared_ptr<WordComplex>& wc, const std::shared_ptr<Model>& model,
                                        size_t compIndex, size_t tokenInd, size_t& correct, const bool isLeft) {
    auto& options = Options::getOptions();
    const auto& comp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[compIndex]);
    const auto& token = m_sentence[tokenInd];
    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    if (options.cleanStopWords) {
        if (StopWordsManager::isStopWord(token->getWordForm().toLowerCase().getRawString()))
            return false;

        const auto normalForm = morphAnalyzer.getLemma(token);
        if (StopWordsManager::isStopWord(normalForm))
            return false;
    }

    const std::string formFromText = token->getWordForm().getRawString();
    if (token->getTokenType() == X::TokenTypeTag::PNCT || token->getTokenType() == X::TokenTypeTag::NUMB) {
        return false;
    }

    if (morphAnalyzer.isMorphAnalysisError(token))
        return false;

    if (!comp->condition().check(comp->getSPTag(), token))
        return false;
    updateWordComplex(wc, token, formFromText, isLeft);

    ++correct;
    const size_t nextCompIndex = isLeft ? compIndex - 1 : compIndex + 1;
    const size_t nextTokenInd = isLeft ? tokenInd - 1 : tokenInd + 1;

    if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->size() - 1)) {
        if (!checkAside(wc, model, nextCompIndex, nextTokenInd, correct, isLeft)) {
            return false;
        }

    } else {
        m_collection.push_back(std::make_shared<WordComplex>(*wc));
        if (comp->isRec() && ((isLeft && tokenInd > 0) || (!isLeft && tokenInd < m_sentence.size() - 1))) {
            if (checkAside(wc, model, compIndex, nextTokenInd, correct, isLeft)) {
                return true;
            } else {
                return false;
            }
        }
    }

    return false;
}

void SimplePhrasesCollector::collect(Process& process) {
    const auto& options = Options::getOptions();
    const auto& patterns = GrammarPatternManager::GetManager();
    const auto& simplePatterns = patterns.getSimplePatterns();
    const auto& morphAnalyzer = MorphAnalyzer::getInstance();

    for (size_t tokenInd = 0; tokenInd < m_sentence.size(); tokenInd++) {
        const auto token = m_sentence[tokenInd];

        if (options.cleanStopWords) {
            if (StopWordsManager::isStopWord(token->getWordForm().toLowerCase().getRawString())) {
                continue;
            }

            const auto normalForm = morphAnalyzer.getLemma(token);
            if (StopWordsManager::isStopWord(normalForm)) {
                continue;
            }
        }

        if (token->getTokenType() == X::TokenTypeTag::PNCT || token->getTokenType() == X::TokenTypeTag::NUMB) {
            continue;
        }

        if (morphAnalyzer.isMorphAnalysisError(token)) {
            continue;
        }

        if (!HaveSpHead(token->getMorphInfo()))
            continue;

        for (const auto& [name, model] : simplePatterns) {
            if (!model->getHead()->condition().check(model->getHead()->getSPTag(), token)) {
                continue;
            }

            const size_t headPos = *model->getHeadPos();
            size_t correct = 0;

            WordComplexPtr wc = initializeWordComplex(tokenInd, token, model->getForm(), process);
            ++correct;

            if (headPos != 0 && tokenInd != 0 && checkAside(wc, model, headPos - 1, tokenInd - 1, correct, true)) {
                break;
            }

            if (headPos != model->size() - 1 && tokenInd + 1 < m_sentence.size() &&
                checkAside(wc, model, headPos + 1, tokenInd + 1, correct, false)) {
                break;
            }
        }
    }
    process.outputResults(m_collection);
}

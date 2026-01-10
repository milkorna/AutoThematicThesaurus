#include "SimplePhrasesCollector.h"
#include "GrammarPatternManager.h"
#include "Logger.h"
#include "Options.h"
#include "Process.h"

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

bool SimplePhrasesCollector::checkAside(const PhrasePtr& wc, const std::shared_ptr<Model>& model, size_t compIndex,
                                        size_t tokenInd, size_t& correct, const bool isLeft) {
    auto& options = Options::getOptions();
    const auto& comp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[compIndex]);
    const auto& token = m_sentence[tokenInd];
    if (!m_validator.isTokenValid(token)) {
        return false;
    }

    const std::string formFromText = token->getWordForm().getRawString();
    if (!comp->condition().check(comp->getSPTag(), token)) {
        return false;
    }

    if (isLeft) {
        wc->addWordToLeft(token);
    } else {
        wc->addWordToRight(token);
    }

    ++correct;
    const size_t nextCompIndex = isLeft ? compIndex - 1 : compIndex + 1;
    const size_t nextTokenInd = isLeft ? tokenInd - 1 : tokenInd + 1;

    if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->size() - 1)) {
        if (!checkAside(wc, model, nextCompIndex, nextTokenInd, correct, isLeft)) {
            return false;
        }

    } else {
        m_collection.push_back(std::make_shared<Phrase>(*wc));
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
    const auto& simplePatterns = GrammarPatternManager::GetManager().getSimplePatterns();

    for (size_t tokenInd = 0; tokenInd < m_sentence.size(); tokenInd++) {
        const auto token = m_sentence[tokenInd];
        if (!m_validator.isTokenValid(token)) {
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

            auto wc = Phrase::createFromToken(tokenInd, token, model->getForm(), process);
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

#include <SimplePhrasesCollector.h>
#include <utility>

static bool HeadCheck(const std::shared_ptr<Model> &baseModel, const X::WordFormPtr &form)
{
    if (!baseModel->getHead()->getCondition().check(baseModel->getHead()->getSPTag(), form))
    {
        return false;
    }
    return true;
}

static bool HaveSpHead(const std::unordered_set<X::MorphInfo> &currFormMorphInfo)
{
    for (const auto &morphForm : currFormMorphInfo)
    {
        Logger::log("HaveSpHead", LogLevel::Debug, "MorphForm: " + morphForm.normalForm.getRawString());
        const auto &spSet = GrammarPatternManager::GetManager()->getUsedHeadSp();
        if (spSet.find(morphForm.sp.toString()) == spSet.end())
        {
            Logger::log("HaveSpHead", LogLevel::Debug, "No head with " + morphForm.sp.toString() + " speach of word");
        }
        else
        {
            Logger::log("HaveSpHead", LogLevel::Debug, "Found head with " + morphForm.sp.toString() + " speach of word");
            return true;
        }
    }
    return false;
}

static bool CheckForMisclassifications(const X::WordFormPtr &form)
{
    std::unordered_set<char> punctuation = {'!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~'};
    const auto str = form->getWordForm().getRawString();

    for (char c : str)
    {
        if (!std::isdigit(c) && punctuation.find(c) == punctuation.end())
            return false;
    }
    return true;
}

static void updateWordComplex(const std::shared_ptr<WordComplex> &wc, const WordFormPtr &form, const std::string &formFromText, bool isLeft)
{
    if (isLeft)
    {
        wc->words.push_front(form);
        wc->pos.start--;
        wc->textForm.insert(0, formFromText + " ");
    }
    else
    {
        wc->words.push_back(form);
        wc->pos.end++;
        wc->textForm.append(" " + formFromText);
    }
}

bool SimplePhrasesCollector::CheckAside(const std::shared_ptr<WordComplex> &wc, const std::shared_ptr<Model> &model, size_t compIndex, size_t tokenInd, size_t &correct, const bool isLeft)
{
    const auto &comp = std::dynamic_pointer_cast<WordComp>(model->getComponents()[compIndex]);
    const auto &token = m_sentence[tokenInd];

    std::string formFromText = token->getWordForm().getRawString();
    Logger::log("CheckAside", LogLevel::Debug, "FormFromText: " + formFromText);

    if (CheckForMisclassifications(token))
        return false;

    if (!comp->getCondition().check(comp->getSPTag(), token))
    {
        Logger::log("CheckAside", LogLevel::Debug, "check failed.");
        return false;
    }
    // also need to update found themes and lex

    // add exception check like 2 = adj

    // updateWordComplex(wc, token, formFromText, isLeft);
    if (isLeft)
    {
        wc->words.push_front(token);
        wc->pos.start--;
        wc->textForm.insert(0, formFromText + " ");
    }
    else
    {
        wc->words.push_back(token);
        wc->pos.end++;
        wc->textForm.append(" " + formFromText);
    }

    ++correct;
    size_t nextCompIndex = isLeft ? compIndex - 1 : compIndex + 1;
    size_t nextTokenInd = isLeft ? tokenInd - 1 : tokenInd + 1;

    if ((isLeft && compIndex > 0) || (!isLeft && compIndex < model->size() - 1))
    {
        CheckAside(wc, model, nextCompIndex, nextTokenInd, correct, isLeft);
    }
    else
    {
        m_collection.push_back(std::make_shared<WordComplex>(*wc));
        if (comp->isRec() && ((isLeft && tokenInd > 0) || (!isLeft && tokenInd < m_sentence.size() - 1)))
        {
            if (CheckAside(wc, model, compIndex, nextTokenInd, correct, isLeft))
            {
                return true;
            }
            else
            {
                Logger::log("Recursive checkAside", LogLevel::Debug, "Failed, stop recursive.");
                return false;
            }
        }
    }

    // Logger::log("checkAside", LogLevel::Debug, "Exiting function, the return value is TRUE.");
    return false;
}

static WordComplexPtr inicializeWordComplex(const size_t tokenInd, const WordFormPtr token, const std::string modelName, const Process &process)
{
    WordComplexPtr wc = std::make_shared<WordComplex>();
    wc->words.push_back(token);
    wc->textForm = token->getWordForm().getRawString();
    wc->pos = {tokenInd,
               tokenInd,
               process.m_docNum,
               process.m_sentNum};
    wc->baseName = modelName;

    return wc;
}

void SimplePhrasesCollector::Collect(const std::vector<WordFormPtr> &forms, Process &process)
{
    Logger::log("Collect", LogLevel::Debug, "Starting base collection process.");
    const auto &collection = SimplePhrasesCollector::GetCollector();
    const auto &simplePatterns = manager.getSimplePatterns();

    WordComplexAgregate wcAgregate;

    m_sentence = forms;

    // std::string agregateForm;

    // WordComplexCollection collectedBases;
    for (size_t tokenInd = 0; tokenInd < m_sentence.size(); tokenInd++)
    {
        const auto token = m_sentence[tokenInd];

        if (!HaveSpHead(token->getMorphInfo()))
            continue;

        for (const auto &[name, model] : simplePatterns)
        {
            Logger::log("Collect", LogLevel::Debug, "Current base: " + model->getForm());

            size_t correct = 0;
            bool headIsMatched = HeadCheck(model, token);
            if (!headIsMatched)
                continue;
            size_t headPos = *model->getHeadPos(); // TODO: make save logic
            ++correct;

            auto wc = inicializeWordComplex(tokenInd, token, model->getForm(), process);

            if (headPos != 0 && tokenInd != 0 &&
                CheckAside(wc, model, headPos - 1, tokenInd - 1, correct, true))
            {
                break;
            }

            if (headPos != model->size() - 1 &&
                CheckAside(wc, model, headPos + 1, tokenInd + 1, correct, false))
            {
                break;
            }

            // add to collection with base.second.form key
        }
    }

    for (const auto &wc : m_collection)
    {
        process.m_output << process.m_docNum << " " << process.m_sentNum << " start_ind = " << wc->pos.start << " end_ind = " << wc->pos.end << "\t||\t" << wc->textForm << "\t||\t" << wc->baseName << std::endl;
    }
}

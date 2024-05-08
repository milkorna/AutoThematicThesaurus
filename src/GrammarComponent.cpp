#include <GrammarComponent.h>

void WordComp::print() const
{
    Logger::log("GrammarComponent", LogLevel::Info, "\t\tword sp: " + this->getSPTag().toString());

    if (const auto &cond = this->getCondition(); !cond.isEmpty())
    {
        Logger::log("GrammarComponent", LogLevel::Info, ", mt: " + cond.getMorphTag().toString());

        if (const auto &addCond = cond.getAdditional(); !addCond.isEmpty())
        {
            Logger::log("GrammarComponent", LogLevel::Info, ", lex: " + addCond.m_exLex);
        }
    }
    std::cout << std::endl;
}

void Model::printWords() const
{
    for (const auto &comps : this->getComponents())
    {
        if (const auto &c = comps.get(); c->isWord())
        {
            WordComp *wc = dynamic_cast<WordComp *>(c);
            // std::cout << "word form: " << this->getForm() << ", sp: " << this->getSPTag().toString();
            wc->print();
        }
        else
        {
            Logger::log("GrammarComponent", LogLevel::Info, "\tmodel form: " + c->getForm() + ", comps: ");
            Model *m = dynamic_cast<Model *>(c);
            m->printWords();
            std::cout << std::endl;
        }
    }
}

std::shared_ptr<WordComp> Model::getHead() const
{
    for (const auto &comp : m_comps)
    {
        // Check if the component is a WordComp
        if (auto wordComp = std::dynamic_pointer_cast<WordComp>(comp))
        {
            // Check the SyntaxRole of the WordComp
            if (wordComp->getCondition().getSyntaxRole() == SyntaxRole::Head)
            {
                return wordComp;
            }
        }
        // If the component is a ModelComp, search its components recursively
        else if (auto modelComp = std::dynamic_pointer_cast<ModelComp>(comp))
        {
            auto headWordComp = modelComp->getHead();
            if (headWordComp != nullptr)
            {
                return headWordComp;
            }
        }
    }
    // No 'WordComp' with 'SyntaxRole::Head' found in this 'ModelComp' or its children 'ModelComp's.
    return nullptr;
}

size_t Model::getHeadPos() const // TODO: Make shorter
{
    for (size_t compInd = 0; compInd < m_comps.size(); compInd++)
    {
        if (auto wordComp = std::dynamic_pointer_cast<WordComp>(m_comps[compInd]))
        {
            // Check the SyntaxRole of the WordComp
            if (wordComp->getCondition().getSyntaxRole() == SyntaxRole::Head)
            {
                return compInd;
            }
        }
        // If the component is a ModelComp, search its components recursively
        else if (auto modelComp = std::dynamic_pointer_cast<ModelComp>(m_comps[compInd]))
        {
            if (modelComp->getCondition().getSyntaxRole() == SyntaxRole::Head)
            {
                return compInd;
            }
        }
    }
    // std optional or smf
    return 0;
}

size_t Model::getSize() const
{
    if (this->isModel())
    {
        return this->m_comps.size();
    }
    else
        return 0; // or 1, whatever
}

template <typename AttrType>
static bool checkAttribute(bool (X::UniMorphTag::*hasAttribute)() const,
                           AttrType (X::UniMorphTag::*getAttribute)() const,
                           const X::UniMorphTag &baseTag,
                           const X::UniMorphTag &formTag)
{
    if ((baseTag.*hasAttribute)())
    {
        bool result = (formTag.*hasAttribute)() && (baseTag.*getAttribute)() == (formTag.*getAttribute)();
        Logger::log("checkAttribute", LogLevel::Debug, "Attribute check: " + std::to_string(result));
        return result;
    }
    // Logger::log("checkAttribute", LogLevel::Debug, "No attribute to check, returning true.");
    return true;
}

bool Condition::morphTagCheck(const MorphInfo &morphForm) const
{
    const auto &compMorphTag = this->getMorphTag();
    return checkAttribute(&X::UniMorphTag::hasAnimacy, &X::UniMorphTag::getAnimacy, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasNumber, &X::UniMorphTag::getNumber, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasTense, &X::UniMorphTag::getTense, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasCmp, &X::UniMorphTag::getCmp, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasVerbForm, &X::UniMorphTag::getVerbForm, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasMood, &X::UniMorphTag::getMood, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasPerson, &X::UniMorphTag::getPerson, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasVariance, &X::UniMorphTag::getVariance, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasVoice, &X::UniMorphTag::getVoice, compMorphTag, morphForm.tag) &&
           checkAttribute(&X::UniMorphTag::hasAspect, &X::UniMorphTag::getAspect, compMorphTag, morphForm.tag);
}

bool Model::checkComponentsMatch(const WordFormPtr &wordForm) const
{
    // for (const auto &comp : m_comps)
    // {
    //     if (!comp->matches(wordForm))
    //     {
    //         return false;
    //     }
    // }
    return true;
}

const bool ModelComp::matches(const std::vector<WordFormPtr> &lexemes, size_t position) const
{
    return true; // checkComponentsMatch(wordForm);
}

const bool WordComp::matches(const WordFormPtr &wordForm, size_t &position) const
{

    // for (const auto &morphInfo : wordForm->getMorphInfo())
    // {
    //     if (morphInfo.tag == this->m_cond.getMorphTag())
    //     {
    //         return true;
    //     }
    // }
    return false;
}

const bool Word::matches(const std::vector<WordFormPtr> &lexemes, size_t &position) const
{
    size_t startPosition = position;
    bool matched = false;
    // while (position < lexemes.size() /*&& TODO*/)
    // {
    //     matched = true;
    //     ++position;
    //     if (!m_cond.m_rec)
    //         break;
    // }
    return matched;
}

const bool Word::matches(const WordFormPtr &wordForm, size_t &position) const
{
    return false;
}

void Model::addComponent(const std::shared_ptr<Component> &component)
{
    m_comps.push_back(component);
}

bool Model::matches(const std::vector<WordFormPtr> &lexemes, size_t &position) const
{
    size_t tempPosition = position;
    // do
    // {
    //     tempPosition = position;
    //     for (const auto &component : m_comps)
    //     {
    //         if (!component->matches(lexemes, tempPosition))
    //         {
    //             position = tempPosition;
    //             return m_cond.m_rec ? true : false;
    //         }
    //     }
    //     if (!m_cond.m_rec)
    //         break;
    // } while (tempPosition != position);

    // position = tempPosition;
    return true;
}

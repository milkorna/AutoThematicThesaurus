#include <GrammarComponent.h>

void WordComp::print() const
{
    std::cout << "\t\tword sp: " << this->getSPTag().toString();
    if (const auto &cond = this->getCondition(); !cond.isEmpty())
    {
        std::cout << ", mt: " << cond.getMorphTag().toString();
        if (const auto &addCond = cond.getAdditional(); !addCond.isEmpty())
        {
            std::cout << ", lex: " << addCond.m_exLex;
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
            std::cout << "\tmodel form: " << c->getForm() << ", comps: " << std::endl;
            Model *m = dynamic_cast<Model *>(c);
            m->printWords();
            std::cout << std::endl;
        }
    }
}

std::shared_ptr<WordComp> Model::getHead() const
{
    // for (const auto &comp : m_comps)
    // {
    //     // Check if the component is a WordComp
    //     if (auto wordComp = std::dynamic_pointer_cast<WordComp>(comp))
    //     {
    //         // Check the SyntaxRole of the WordComp
    //         if (wordComp->getCondition().getSyntaxRole() == SyntaxRole::Head)
    //         {
    //             return wordComp;
    //         }
    //     }
    //     // If the component is a ModelComp, search its components recursively
    //     else if (auto modelComp = std::dynamic_pointer_cast<ModelComp>(comp))
    //     {
    //         auto headWordComp = modelComp->getHead();
    //         if (headWordComp != nullptr)
    //         {
    //             return headWordComp;
    //         }
    //     }
    // }
    // No 'WordComp' with 'SyntaxRole::Head' found in this 'ModelComp' or its children 'ModelComp's.
    return nullptr;
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
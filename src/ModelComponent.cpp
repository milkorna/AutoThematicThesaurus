#include <ModelComponent.h>

using namespace X;

Model::Model(const std::string& form, const Components& comps) : m_form(form), m_comps(comps)
{
}

const UniSPTag Model::getSPTag() const
{
    return UniSPTag::X;
}
const std::string Model::getForm() const
{
    return m_form;
}
const Components Model::getComponents() const
{
    return m_comps;
}
const std::shared_ptr<Component> Model::getComponent(const size_t ind) const
{
    return m_comps[ind];
}
const std::shared_ptr<WordComp> Model::getWordComponent(const size_t ind) const
{
    return std::dynamic_pointer_cast<WordComp>(m_comps[ind]);
}
const std::shared_ptr<ModelComp> Model::getModelComponent(const size_t ind) const
{
    return std::dynamic_pointer_cast<ModelComp>(m_comps[ind]);
}

const bool Model::isWord() const
{
    return false;
}
const bool Model::isModel() const
{
    return true;
}
const std::optional<bool> Model::isHead() const
{
    return std::nullopt;
}

void Model::printWords() const
{
    for (const auto& comps : this->getComponents()) {
        if (const auto& c = comps.get(); c->isWord()) {
            WordComp* wc = dynamic_cast<WordComp*>(c);
            // std::cout << "word form: " << this->getForm() << ", sp: " << this->getSPTag().toString();
            wc->print();
        } else {
            Logger::log("GrammarComponent", LogLevel::Info, "\tmodel form: " + c->getForm() + ", comps: ");
            Model* m = dynamic_cast<Model*>(c);
            m->printWords();
            std::cout << std::endl;
        }
    }
}

std::shared_ptr<WordComp> Model::getHead() const
{
    for (const auto& comp : m_comps) {
        // Check if the component is a WordComp
        if (auto wordComp = std::dynamic_pointer_cast<WordComp>(comp)) {
            // Check the SyntaxRole of the WordComp
            if (wordComp->getCondition().getSyntaxRole() == SyntaxRole::Head) {
                return wordComp;
            }
        }
        // If the component is a ModelComp, search its components recursively
        else if (auto modelComp = std::dynamic_pointer_cast<ModelComp>(comp)) {
            auto headWordComp = modelComp->getHead();
            if (headWordComp != nullptr) {
                return headWordComp;
            }
        }
    }
    // No 'WordComp' with 'SyntaxRole::Head' found in this 'ModelComp' or its children 'ModelComp's.
    return nullptr;
}

std::optional<size_t> Model::getHeadPos() const // TODO: Make shorter
{
    for (size_t compInd = 0; compInd < m_comps.size(); compInd++) {
        if (auto wordComp = std::dynamic_pointer_cast<WordComp>(m_comps[compInd])) {
            // Check the SyntaxRole of the WordComp
            if (wordComp->getCondition().getSyntaxRole() == SyntaxRole::Head) {
                return compInd;
            }
        }
        // If the component is a ModelComp, search its components recursively
        else if (auto modelComp = std::dynamic_pointer_cast<ModelComp>(m_comps[compInd])) {
            if (modelComp->getCondition().getSyntaxRole() == SyntaxRole::Head) {
                return compInd;
            }
        }
    }
    return std::nullopt;
}

std::optional<size_t> Model::getModelCompIndByForm(const std::string& form) const // TODO: Make shorter
{
    for (size_t compInd = 0; compInd < m_comps.size(); compInd++) {
        if (auto modelComp = std::dynamic_pointer_cast<ModelComp>(m_comps[compInd])) {
            if (modelComp->getForm() == form) {
                return compInd;
            }
        }
    }
    // std optional or smf
    return std::nullopt;
}

size_t Model::size() const
{
    return this->isModel() ? this->m_comps.size() : 0;
}

void Model::addComponent(const std::shared_ptr<Component>& component)
{
    m_comps.push_back(component);
}

ModelComp::ModelComp(const std::string& form, const Components& comps, const Condition& cond)
    : Model(form, comps), m_cond(cond)
{
}

const Condition ModelComp::getCondition() const
{
    return m_cond;
}

const std::optional<bool> ModelComp::isHead() const
{
    auto role = m_cond.getSyntaxRole();
    if (role == SyntaxRole::Head)
        return true;
    if (role == SyntaxRole::Dependent || role == SyntaxRole::Independent)
        return false;
    return std::nullopt;
}
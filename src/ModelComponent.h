#ifndef MODEL_COMPONENT_H
#define MODEL_COMPONENT_H

#include <WordComponent.h>

class ModelComp;

// Derived class representing a grammatical model.
class Model : public Component {
    std::string m_form;
    Components m_comps;

public:
    Model(const std::string& form = "", const Components& comps = {}) : m_form(form), m_comps(comps) {};

    const X::UniSPTag getSPTag() const override
    {
        return UniSPTag::X;
    }
    const std::string getForm() const override
    {
        return m_form;
    }
    const Components getComponents() const override
    {
        return m_comps;
    }
    const std::shared_ptr<Component> getComponent(const size_t ind) const
    {
        return m_comps[ind];
    }
    const std::shared_ptr<WordComp> getWordComponent(const size_t ind) const
    {
        return std::dynamic_pointer_cast<WordComp>(m_comps[ind]);
    }
    const std::shared_ptr<ModelComp> getModelComponent(const size_t ind) const
    {
        return std::dynamic_pointer_cast<ModelComp>(m_comps[ind]);
    }

    const bool isWord() const override
    {
        return false;
    }
    const bool isModel() const override
    {
        return true;
    }
    const std::optional<bool> isHead() const
    {
        return std::nullopt;
    }

    void addCondition(const std::shared_ptr<Condition>& Condition);
    bool checkComponentsMatch(const X::WordFormPtr& wordForm) const;

    void addComponent(const std::shared_ptr<Component>& component);

    std::optional<size_t> getModelCompIndByForm(const std::string& form) const;

    std::shared_ptr<WordComp> getHead() const;
    std::optional<size_t> getHeadPos() const;
    size_t size() const;

    void printWords() const;
};

// Derived class representing a grammatical model with specific conditions.
class ModelComp : public Model {
    Condition m_cond;

public:
    ModelComp(const std::string& form = "", const Components& comps = {}, const Condition& cond = {})
        : Model(form, comps), m_cond(cond) {};

    const Condition getCondition() const
    {
        return m_cond;
    }

    const std::optional<bool> isHead() const
    {
        auto role = m_cond.getSyntaxRole();
        if (role == SyntaxRole::Head)
            return true;
        if (role == SyntaxRole::Dependent || role == SyntaxRole::Independent)
            return false;
        return std::nullopt;
    }

    void addComponent(const std::shared_ptr<Component>& component);
};

#endif // GRAMMAR_COMPONENTS_H

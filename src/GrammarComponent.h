#ifndef GRAMMAR_COMPONENTS_H
#define GRAMMAR_COMPONENTS_H

#include <GrammarCondition.h>

using namespace X;

class Component;
class WordComp;
class ModelComp;

// Type definition for a vector of shared pointers to Components.
using Components = std::vector<std::shared_ptr<Component>>;

// Abstract base class for grammatical components.
class Component {
public:
    virtual ~Component() = default;
    virtual const UniSPTag getSPTag() const = 0;
    virtual const std::string getForm() const = 0;
    virtual const Components getComponents() const = 0;

    virtual const bool isWord() const = 0;
    virtual const bool isModel() const = 0;
    virtual const std::optional<bool> isHead() const = 0;
};

// Derived class representing a Word in the grammar system.
class Word : public Component {
    UniSPTag m_sp;

public:
    Word(UniSPTag sp = UniSPTag::X) : m_sp(sp)
    {
    }

    const UniSPTag getSPTag() const override
    {
        return m_sp;
    }
    const std::string getForm() const override
    {
        return "";
    }
    const Components getComponents() const override
    {
        return {};
    }

    const bool isWord() const override
    {
        return true;
    }
    const bool isModel() const override
    {
        return false;
    }
    const std::optional<bool> isHead() const
    {
        return std::nullopt;
    }
};

// Derived class representing a Word with specific conditions.
class WordComp : public Word {
    Condition m_cond;

public:
    WordComp(const UniSPTag& sp = UniSPTag::X, const Condition& cond = Condition()) : Word(sp), m_cond(cond)
    {
    }

    const Condition getCondition() const
    {
        return m_cond;
    }
    const bool isRec()
    {
        return m_cond.getAdditional().m_rec;
    }
    const std::optional<bool> isHead() const
    {
        auto role = m_cond.getSyntaxRole();
        if (role == SyntaxRole::Head) {
            return true;
        }
        if (role == SyntaxRole::Dependent || role == SyntaxRole::Independent) {
            return false;
        }
        return std::nullopt;
    }

    ~WordComp() override
    {
    }

    void print() const;
};

// Derived class representing a grammatical model.
class Model : public Component {
    std::string m_form;
    Components m_comps;

public:
    Model(const std::string& form = "", const Components& comps = {}) : m_form(form), m_comps(comps) {};

    const UniSPTag getSPTag() const override
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
    bool checkComponentsMatch(const WordFormPtr& wordForm) const;

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

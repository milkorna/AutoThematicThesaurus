#ifndef WORD_COMPONENT_H
#define WORD_COMPONENT_H

#include <GrammarComponent.h>

// Derived class representing a Word in the grammar system.
class Word : public Component {
    UniSPTag m_sp;

public:
    Word(UniSPTag sp = UniSPTag::X) : m_sp(sp)
    {
    }

    const X::UniSPTag getSPTag() const override
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

#endif
#ifndef WORD_COMPONENT_H
#define WORD_COMPONENT_H

#include <Component.h>

// Derived class representing a Word in the grammar system.
class Word : public Component {
    UniSPTag m_sp;

public:
    Word(UniSPTag sp = UniSPTag::X);

    const X::UniSPTag getSPTag() const override;
    const std::string getForm() const override;
    const Components getComponents() const override;

    const bool isWord() const override;
    const bool isModel() const override;

    const std::optional<bool> isHead() const;
};

// Derived class representing a Word with specific conditions.
class WordComp : public Word {
    Condition m_cond;

public:
    WordComp(const UniSPTag& sp = UniSPTag::X, const Condition& cond = Condition());

    const Condition getCondition() const;
    const bool isRec();
    const std::optional<bool> isHead() const;

    ~WordComp() override
    {
    }

    void print() const;
};

#endif
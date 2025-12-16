#pragma once

#include <xmorphy/morph/WordForm.h>
#include <xmorphy/tag/UniMorphTag.h>
#include <xmorphy/tag/UniSPTag.h>

#include "Component.h"
#include "GrammarCondition.h"

// Derived class representing a Word in the grammar system.
class Word : public Component {
    UniSPTag m_sp;

  public:
    explicit Word(UniSPTag sp = UniSPTag::X);

    [[nodiscard]] const X::UniSPTag getSPTag() const override;
    [[nodiscard]] const std::string getForm() const override;
    [[nodiscard]] const Components getComponents() const override;

    [[nodiscard]] const bool isWord() const override;
    [[nodiscard]] const bool isModel() const override;

    virtual ~Word() = default;
};

// Derived class representing a Word with specific conditions.
class WordComp : public Word {
    Condition m_cond;

  public:
    explicit WordComp(const UniSPTag& sp = UniSPTag::X, const Condition& cond = Condition());
    ~WordComp() override = default;

    [[nodiscard]] const Condition& condition() const noexcept;

    [[nodiscard]] const bool isRec() const;

    [[nodiscard]] const std::optional<bool> isHead() const override;

    void print() const;
};

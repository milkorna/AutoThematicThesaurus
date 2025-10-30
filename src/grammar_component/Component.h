#pragma once

#include <xmorphy/morph/WordForm.h>
#include <xmorphy/tag/UniMorphTag.h>
#include <xmorphy/tag/UniSPTag.h>

#include <vector>
#include <memory>

using namespace X;

class Component;

// Type definition for a vector of shared pointers to Components.
using Components = std::vector<std::shared_ptr<Component>>;

// Abstract class for grammatical components.
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

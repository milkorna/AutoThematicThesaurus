#include "WordComponent.h"
#include "Logger.h"

using namespace X;

Word::Word(UniSPTag sp) : m_sp(sp) {
}

const X::UniSPTag Word::getSPTag() const {
    return m_sp;
}

const std::string Word::getForm() const {
    return "";
}

const Components Word::getComponents() const {
    return {};
}

const bool Word::isWord() const {
    return true;
}

const bool Word::isModel() const {
    return false;
}

void WordComp::print() const {
    try {
        Logger::log("\t\tsp", LogLevel::Info, this->getSPTag().toString());
    } catch (...) {
        Logger::log("\t\tsp", LogLevel::Warning, "failed to stringify SP tag");
    }

    if (const auto& cond = this->condition(); !cond.isDefault()) {
        try {
            Logger::log("\t\t\t\tmt", LogLevel::Info, cond.getMorphTag().toString());
        } catch (...) {
            Logger::log("\t\t\t\tmt", LogLevel::Warning, "failed to stringify morph tag");
        }

        if (cond.hasExactLexeme()) {
            Logger::log("\t\t\t\tlex", LogLevel::Info, cond.getExactLexeme());
        }
    }
}

WordComp::WordComp(const UniSPTag& sp, const Condition& cond) : Word(sp), m_cond(cond) {
}

const Condition& WordComp::condition() const noexcept {
    return m_cond;
}

const bool WordComp::isRec() const {
    return m_cond.isRecursive();
}

const std::optional<bool> WordComp::isHead() const {
    auto role = m_cond.getSyntaxRole();
    if (role == SyntaxRole::Head) {
        return true;
    }
    if (role == SyntaxRole::Dependent || role == SyntaxRole::Independent) {
        return false;
    }
    return std::nullopt;
}
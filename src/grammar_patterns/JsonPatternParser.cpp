#include "JsonPatternParser.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
using json = nlohmann::json;

static const std::map<std::string, X::UniMorphTag>& tagMap() {
    using X::UniMorphTag;
    static const std::map<std::string, UniMorphTag> m = {
        {"Gender=Masc", UniMorphTag::Masc},    {"Gender=Fem", UniMorphTag::Fem},
        {"Gender=Neut", UniMorphTag::Neut},    {"Animacy=Anim", UniMorphTag::Anim},
        {"Animacy=Inan", UniMorphTag::Inan},   {"Number=Sing", UniMorphTag::Sing},
        {"Number=Plur", UniMorphTag::Plur},    {"Case=Ins", UniMorphTag::Ins},
        {"Case=Acc", UniMorphTag::Acc},        {"Case=Nom", UniMorphTag::Nom},
        {"Case=Dat", UniMorphTag::Dat},        {"Case=Gen", UniMorphTag::Gen},
        {"Case=Loc", UniMorphTag::Loc},        {"Case=Voc", UniMorphTag::Voc},
        {"Degree=Cmp", UniMorphTag::Cmp},      {"Degree=Sup", UniMorphTag::Sup},
        {"Degree=Pos", UniMorphTag::Pos},      {"VerbForm=Fin", UniMorphTag::Fin},
        {"VerbForm=Inf", UniMorphTag::Inf},    {"VerbForm=Conv", UniMorphTag::Conv},
        {"VerbForm=Part", UniMorphTag::Part},  {"Mood=Imp", UniMorphTag::Imp},
        {"Mood=Ind", UniMorphTag::Ind},        {"Person=1", UniMorphTag::_1},
        {"Person=2", UniMorphTag::_2},         {"Person=3", UniMorphTag::_3},
        {"Tense=Fut", UniMorphTag::Fut},       {"Tense=Past", UniMorphTag::Past},
        {"Tense=Pres", UniMorphTag::Pres},     {"Tense=Notpast", UniMorphTag::Notpast},
        {"Variant=Short", UniMorphTag::Short}, {"Voice=Act", UniMorphTag::Act},
        {"Voice=Pass", UniMorphTag::Pass},     {"Voice=Mid", UniMorphTag::Mid},
        {"NumForm=Digit", UniMorphTag::Digit}, {"Aspect=Perf", UniMorphTag::Perf},
        {"Aspect=Imp", UniMorphTag::Imp}};
    return m;
}

SyntaxRole roleFromString(const std::string& s) {
    if (s == "head")
        return SyntaxRole::Head;
    if (s == "dependent")
        return SyntaxRole::Dependent;
    if (s == "independent")
        return SyntaxRole::Independent;

    Logger::log("JsonPatternParser", LogLevel::Warning, "unknown role: " + s + " (fallback to Independent)");
    return SyntaxRole::Independent;
}

bool isEnabled(const json& pat) {
    // по умолчанию включено
    if (!pat.contains("enabled"))
        return true;
    if (!pat.at("enabled").is_boolean())
        return true;
    return pat.at("enabled").get<bool>();
}

X::UniMorphTag featuresFromJson(const json& features) {
    X::UniMorphTag acc = X::UniMorphTag::UNKN;
    bool hasAny = false;
    const auto& M = tagMap();

    for (auto it = features.begin(); it != features.end(); ++it) {
        if (!it.value().is_string())
            continue;
        const auto& k = it.key();
        const auto& v = it.value().get_ref<const std::string&>();

        std::string token;
        token.reserve(k.size() + 1 + v.size());
        token.append(k).push_back('=');
        token.append(v);

        if (auto mapIt = M.find(token); mapIt != M.end()) {
            acc = hasAny ? (acc | mapIt->second) : mapIt->second;
            hasAny = true;
        } else {
            Logger::log("JsonPatternParser", LogLevel::Warning, "unknown feature token: " + token);
        }
    }
    return hasAny ? acc : X::UniMorphTag::UNKN;
}

} // namespace

// ====== Конструкторы ======
JsonPatternParser::JsonPatternParser(const fs::path& filePath) {
    std::ifstream in(filePath);
    if (!in) {
        throw std::runtime_error("JsonPatternParser: cannot open file " + filePath.string());
    }
    json arr;
    in >> arr;
    if (!arr.is_array()) {
        throw std::runtime_error("JsonPatternParser: top-level JSON must be an array");
    }

    for (const auto& item : arr) {
        if (!item.is_object())
            continue;
        if (!item.contains("name")) {
            Logger::log("JsonPatternParser", LogLevel::Warning, "pattern without 'name' skipped");
            continue;
        }
        const auto name = item.at("name").get<std::string>();
        if (rawPatterns_.count(name)) {
            Logger::log("JsonPatternParser", LogLevel::Warning, "duplicate pattern name: " + name + " (overwriting)");
        }
        rawPatterns_[name] = item;
    }
}

JsonPatternParser::JsonPatternParser(const json& arr) {
    if (!arr.is_array()) {
        throw std::runtime_error("JsonPatternParser: top-level JSON must be an array");
    }
    for (const auto& item : arr) {
        if (!item.is_object())
            continue;
        if (!item.contains("name"))
            continue;
        const auto name = item.at("name").get<std::string>();
        rawPatterns_[name] = item;
    }
}

// ====== Публичный метод ======
void JsonPatternParser::parseAll() {
    auto* manager = GrammarPatternManager::GetManager();
    size_t added = 0;

    for (const auto& [name, pat] : rawPatterns_) {
        if (!isEnabled(pat))
            continue;

        try {
            if (auto model = buildModel(name)) {
                manager->addPattern(name, std::move(model));
                ++added;
            }
        } catch (const std::exception& e) {
            Logger::log("JsonPatternParser", LogLevel::Error, "failed build for '" + name + "': " + e.what());
        }
    }
    manager->divide();
    Logger::log("JsonPatternParser", LogLevel::Info, "parseAll: added " + std::to_string(added) + " patterns");
}

// ====== Приватные ======
std::shared_ptr<Model> JsonPatternParser::buildModel(const std::string& name) {
    // memo
    if (auto it = built_.find(name); it != built_.end()) {
        return it->second;
    }

    // lookup raw
    auto it = rawPatterns_.find(name);
    if (it == rawPatterns_.end()) {
        throw std::runtime_error("unknown pattern: " + name);
    }
    const json& pat = it->second;

    // цикл?
    if (visiting_.count(name)) {
        throw std::runtime_error("cyclic pattern dependency detected at: " + name);
    }
    visiting_.insert(name);

    if (!pat.contains("body") || !pat.at("body").is_array()) {
        visiting_.erase(name);
        throw std::runtime_error("pattern '" + name + "' has no array 'body'");
    }

    Components comps = buildComponents(pat.at("body"));
    auto model = std::make_shared<Model>(name, comps);

    visiting_.erase(name);
    built_[name] = model;
    return model;
}

Components JsonPatternParser::buildComponents(const json& body) {
    Components out;
    out.reserve(body.size());

    for (const auto& item : body) {
        if (!item.is_object() || !item.contains("type") || !item.contains("role")) {
            Logger::log("JsonPatternParser", LogLevel::Warning, "body item missing required fields (type/role)");
            continue;
        }
        const auto type = item.at("type").get<std::string>();
        if (type == "word") {
            auto w = buildWordComp(item);
            if (w)
                out.push_back(w);
        } else if (type == "pattern") {
            auto m = buildPatternComp(item);
            if (m)
                out.push_back(m);
        } else {
            Logger::log("JsonPatternParser", LogLevel::Warning, "unknown body item type: " + type);
        }
    }

    return out;
}

std::shared_ptr<WordComp> JsonPatternParser::buildWordComp(const json& item) {
    // role
    const auto role = roleFromString(item.at("role").get<std::string>());

    // pos
    if (!item.contains("pos")) {
        Logger::log("JsonPatternParser", LogLevel::Error, "word item has no 'pos' ");
        return nullptr;
    }
    const std::string posStr = item.at("pos").get<std::string>();
    X::UniSPTag sp(posStr);

    // учтём статистику частей речи
    GrammarPatternManager::GetManager()->addUsedSp(sp.toString(), role == SyntaxRole::Head);

    // features
    X::UniMorphTag tag = X::UniMorphTag::UNKN; // по умолчанию UNKN (см. конструктор по умолчанию)

    if (item.contains("features") && item.at("features").is_object()) {
        tag = featuresFromJson(item.at("features"));
    }

    // additional
    Additional add;
    if (item.contains("recursive") && item.at("recursive").is_boolean()) {
        add.m_rec = item.at("recursive").get<bool>();
    }
    if (item.contains("exact_lexeme") && item.at("exact_lexeme").is_string()) {
        add.m_exLex = item.at("exact_lexeme").get<std::string>();
    }
    Condition cond(role, tag, add);
    return std::make_shared<WordComp>(sp, cond);
}

std::shared_ptr<ModelComp> JsonPatternParser::buildPatternComp(const json& item) {
    const auto role = roleFromString(item.at("role").get<std::string>());
    if (!item.contains("pattern") || !item.at("pattern").is_string()) {
        Logger::log("JsonPatternParser", LogLevel::Error, "pattern item has no 'pattern' name");
        return nullptr;
    }
    const std::string refName = item.at("pattern").get<std::string>();
    // Рекурсивно построим/получим ссылочный паттерн
    auto refModel = buildModel(refName);
    if (!refModel) {
        Logger::log("JsonPatternParser", LogLevel::Error, "unable to build referenced pattern: " + refName);
        return nullptr;
    }

    // features (к паттерну) — например Case=Gen для всей группы
    X::UniMorphTag tag = X::UniMorphTag::UNKN;
    if (item.contains("features") && item.at("features").is_object()) {
        tag = featuresFromJson(item.at("features"));
    }

    // additional (обычно пусто для вложенных, но поддерживаем)
    Additional add;
    if (item.contains("recursive") && item.at("recursive").is_boolean()) {
        add.m_rec = item.at("recursive").get<bool>();
    }
    if (item.contains("exact_lexeme") && item.at("exact_lexeme").is_string()) {
        add.m_exLex = item.at("exact_lexeme").get<std::string>();
    }
    Condition cond(role, tag, add);
    return std::make_shared<ModelComp>(refName, refModel->getComponents(), cond);
}

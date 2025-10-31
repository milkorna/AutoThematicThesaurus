#include "JsonPatternParser.h"

#include "utils/PathUtils.h"
using util::path::extractNumberFromPath;

#include <fstream>
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
            throw std::runtime_error("features: value for key '" + it.key() + "' must be a string");

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
            throw std::runtime_error("unknown feature token: " + token);
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

    try {
        for (const auto& [name, pat] : rawPatterns_) {
            if (!isEnabled(pat))
                continue;

            try {
                // buildModel может бросить (неизвестная роль/feature, цикл, битый body и т.д.)
                auto model = buildModel(name);
                manager->add(name, std::move(model));
                ++added;
            } catch (const std::exception& cause) {
                // точный контекст по имени паттерна
                const std::string msg = "failed build for '" + name + "': " + std::string(cause.what());
                Logger::log("JsonPatternParser", LogLevel::Error, msg);

                // откат уже добавленного, чтобы загрузка была атомарной
                manager->clear();

                // перебрасываем с сохранением первоначальной причины
                std::throw_with_nested(std::runtime_error(msg));
            }
        }

        manager->divide();
        Logger::log("JsonPatternParser", LogLevel::Info, "parseAll: added " + std::to_string(added) + " patterns");
    } catch (...) {
        // на случай исключений из divide()/log и пр.: откат и повторный throw
        manager->clear();
        throw;
    }
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

    struct VisitingGuard {
        std::unordered_set<std::string>& s;
        const std::string& k;

        ~VisitingGuard() {
            s.erase(k);
        }
    } guard{visiting_, name};

    // дальше можно смело бросать — guard очистит visiting_
    if (!pat.contains("body") || !pat.at("body").is_array()) {
        throw std::runtime_error("pattern '" + name + "' has no array 'body'");
    }

    Components comps = buildComponents(pat.at("body"), name);
    auto model = std::make_shared<Model>(name, comps);
    built_[name] = model;
    return model;
}

Components JsonPatternParser::buildComponents(const json& body, const std::string& ownerName) {
    Components out;
    out.reserve(body.size());

    size_t idx = 0;
    for (const auto& item : body) {
        const std::string where = "pattern '" + ownerName + "', body[" + std::to_string(idx) + "]";

        if (!item.is_object())
            throw std::runtime_error(where + ": body item must be an object");

        if (!item.contains("type") || !item.contains("role"))
            throw std::runtime_error(where + ": missing required fields 'type'/'role'");

        const auto type = item.at("type").get<std::string>();

        // === НОВОЕ: строгая валидация role ===
        if (!item.at("role").is_string())
            throw std::runtime_error(where + ": 'role' must be a string");
        const std::string roleStr = item.at("role").get<std::string>();
        if (roleStr != "head" && roleStr != "dependent" && roleStr != "independent")
            throw std::runtime_error(where + ": unknown role '" + roleStr + "' (allowed: head|dependent|independent)");

        if (type == "word") {
            if (!item.contains("pos") || !item.at("pos").is_string())
                throw std::runtime_error(where + ": word item must have string 'pos'");
            out.push_back(buildWordComp(item));
        } else if (type == "pattern") {
            if (!item.contains("pattern") || !item.at("pattern").is_string())
                throw std::runtime_error(where + ": pattern item must have string 'pattern'");
            out.push_back(buildPatternComp(item));
        } else {
            throw std::runtime_error(where + ": unknown body item type: " + type);
        }
        ++idx;
    }
    return out;
}

std::shared_ptr<WordComp> JsonPatternParser::buildWordComp(const json& item) {
    const auto role = roleFromString(item.at("role").get<std::string>());

    // здесь pos уже гарантирован и строка
    const std::string posStr = item.at("pos").get<std::string>();
    X::UniSPTag sp(posStr);

    GrammarPatternManager::GetManager()->addUsedSp(sp.toString(), role == SyntaxRole::Head);

    X::UniMorphTag tag = X::UniMorphTag::UNKN;
    if (item.contains("features") && item.at("features").is_object()) {
        tag = featuresFromJson(item.at("features"));
    }

    Additional add;
    if (item.contains("recursive") && item.at("recursive").is_boolean())
        add.m_rec = item.at("recursive").get<bool>();
    if (item.contains("exact_lexeme") && item.at("exact_lexeme").is_string())
        add.m_exLex = item.at("exact_lexeme").get<std::string>();

    Condition cond(role, tag, add);
    return std::make_shared<WordComp>(sp, cond);
}

std::shared_ptr<ModelComp> JsonPatternParser::buildPatternComp(const json& item) {
    // role уже провалидирован в buildComponents(...), но получить его всё равно нужно
    const auto role = roleFromString(item.at("role").get<std::string>());

    // Строгая проверка поля "pattern"
    if (!item.contains("pattern") || !item.at("pattern").is_string()) {
        throw std::runtime_error("pattern item must have string 'pattern'");
    }
    const std::string refName = item.at("pattern").get<std::string>();

    // Рекурсивная сборка референтного паттерна: бросит при unknown/cycle
    auto refModel = buildModel(refName);

    // features к группе (может бросить при неизвестном токене)
    X::UniMorphTag tag = X::UniMorphTag::UNKN;
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
    return std::make_shared<ModelComp>(refName, refModel->getComponents(), cond);
}

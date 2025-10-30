#ifndef JSON_PATTERN_PARSER_H
#define JSON_PATTERN_PARSER_H

#include <GrammarPatternManager.h>
#include <ModelComponent.h>

#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;
using json = nlohmann::json;

class JsonPatternParser {
public:
  // Ожидается файл с JSON-массивом шаблонов
  explicit JsonPatternParser(const fs::path& filePath);

  // Или напрямую готовый json-массив
  explicit JsonPatternParser(const json& arr);

  // Разобрать и добавить все включённые (enabled != false) шаблоны в менеджер
  void parseAll();

private:
  // Сырые json-описания по имени
  std::unordered_map<std::string, json> rawPatterns_;
  // Построенные модели (мемоизация)
  std::unordered_map<std::string, std::shared_ptr<Model>> built_;
  // Для обнаружения циклов
  std::unordered_set<std::string> visiting_;

  // Строительство по имени (с мемоизацией и защитой от циклов)
  std::shared_ptr<Model> buildModel(const std::string& name);

  // Строительство компонентов модели из body-массива
  Components buildComponents(const json& body);

  // Построить WordComp по json-элементу
  std::shared_ptr<WordComp> buildWordComp(const json& item);

  // Построить ModelComp (вложенный pattern) по json-элементу
  std::shared_ptr<ModelComp> buildPatternComp(const json& item);

  // Утилиты
  static SyntaxRole roleFromString(const std::string& s);
  static bool isEnabled(const json& pat);                    // default true
  static UniMorphTag featuresFromJson(const json& features); // аккумулирует маску UniMorphTag
};

#endif // JSON_PATTERN_PARSER_H

import json
import re

# Hyponym markers – phrases that typically introduce a subtype (hyponym)
HYPONYM_MARKERS = [
    "алгоритм",       # алгоритм классификации
    "метод",          # метод оптимизации
    "модель",         # модель обучения
    "архитектура",    # архитектура трансформера
    "подход",         # подход на основе графов
    "техника",        # техника повышения точности
    "процедура",      # процедура инициализации
    "операция",       # операция свёртки
    "механизм",       # механизм внимания
    "модификация",    # модификация алгоритма
    "вариант",        # вариант модели
    "стратегия",      # стратегия выбора
    "структура",      # структура данных
    "парадигма",      # парадигма программирования
    "алтернатива",    # альтернатива методу
    "реализация",     # реализация подхода
    "инстанция",      # инстанция модели
    "тип",            # тип обучения
]

# Related markers – phrases that are semantically related but not hierarchical
RELATED_MARKERS = [
    "автоматический выделение",
    "автоматический извлечение",
    "автоматический аннотация",
    "автоматический разметка",
    "автоматический классификация",
    "автоматический обработка",
    "выделение",
    "извлечение",
    "разметка",
    "аннотация",
    "обработка",
    "анализ",
    "построение",
    "идентификация",
    "распознавание",
    "предсказание",
    "оценка",
    "интерпретация",
    "генерация",
    "агрегация",
    "классификация",
    "кластеризация",
    "корреляция",
    "взвешивание",
    "фильтрация",
    "формализация",
    "теггинг",
    "лемматизация",
    "нормализация",
    "сегментация",
    "сопоставление",
    "сравнение",
    "поиск",
    "проверка",
    "сжатие",
    "рерайтинг",
    "автоматический выделение",
    "выделение",
    "извлечение",
    "автоматический извлечение",
    "задача",          # задача оптимизации
    "результат",       # результат анализа
    "сценарий",        # сценарий использования
    "пример",          # пример классификации
    "цель",            # цель обучения
    "объект",          # объект классификации
    "описание",        # описание модели
    "ошибка",          # ошибка предсказания
    "характеристика",  # характеристика алгоритма
    "ограничение",     # ограничение метода
    "применение",      # применение модели
    "использование",   # использование алгоритма
    "уровень",         # уровень шума
    "параметр",        # параметр модели
    "фактор",          # фактор влияния
    "область",         # область применения
    "аспект",          # аспект подхода
    "эффект",          # эффект применения
    "показатель",      # показатель точности
]

ALL_MARKERS = HYPONYM_MARKERS + RELATED_MARKERS

def normalize(phrase):
    """Normalize phrase: lowercase and collapse whitespace"""
    return re.sub(r"\s+", " ", phrase.strip().lower())

def starts_with_modifier(full_phrase, base_phrase):
    """
    Check if full_phrase equals "<modifier> base_phrase"
    """
    for marker in ALL_MARKERS:
        if normalize(full_phrase) == f"{marker} {normalize(base_phrase)}":
            return marker
    return None

def correct_relation(key, phrase, current_relation):
    """
    Determine corrected relation based on modifier patterns
    """
    norm_key = normalize(key)
    norm_phrase = normalize(phrase)

    # Case 1: key = "<modifier> phrase" → phrase is a hypernym
    marker = starts_with_modifier(norm_key, norm_phrase)
    if marker:
        return "hypernym" if marker in HYPONYM_MARKERS else "related"

    # Case 2: phrase = "<modifier> key" → phrase is a hyponym
    marker = starts_with_modifier(norm_phrase, norm_key)
    if marker:
        return "hyponym" if marker in HYPONYM_MARKERS else "related"

    # Otherwise, keep the original relation
    return current_relation

def correct_relations(data):
    """
    Apply relation correction to all entries in the dataset
    """
    changes = []
    for entry in data:
        key = entry["key"]
        for phrase_entry in entry["phrases"]:
            new_relation = correct_relation(key, phrase_entry["phrase"], phrase_entry["relation"])
            if new_relation != phrase_entry["relation"]:
                changes.append((key, phrase_entry["phrase"], phrase_entry["relation"], new_relation))
                phrase_entry["relation"] = new_relation
    return data, changes

# === Load and save ===
input_path = "marked_relations_manual.json"
output_path = "marked_relations_manual.json"

with open(input_path, "r", encoding="utf-8") as f:
    data = json.load(f)

corrected_data, log = correct_relations(data)

with open(output_path, "w", encoding="utf-8") as f:
    json.dump(corrected_data, f, ensure_ascii=False, indent=4)

# Print log of changes
print("=== Relation corrections ===")
for key, phrase, old, new in log:
    print(f"'{phrase}' -> '{key}': {old} → {new}")

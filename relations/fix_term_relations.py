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
RELATED_MARKERS = [ # todo: add relation related_action in future
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

# Adjectival modifiers that imply hyponym relation when prepended to a base phrase
HYPONYM_ADJECTIVES = [
    "автоматический",
    "нейросетевой",
    "глубинный",
    "статистический",
    "эмпирический",
    "обучаемый",
    "интерактивный",
    "адаптивный",
    "параллельный",
    "гибридный",
    "семантический",
    "синтаксический",
    "лексический",
    "морфологический",
    "корпусный",
    "графовый",
    "контекстный",
    "байесовский",
    "нелинейный"
]

SYNONYM_EQUIVALENTS = [
    ("данный", "датасет"),         # входной данный ≈ входной датасет
    ("классификация", "категоризация"),  # классификация текста ≈ категоризация текста
    ("задача", "проблема"),        # задача сегментация ≈ проблема сегментация
    ("цель", "задача"),            # цель обучение ≈ задача обучение
    ("ошибка", "погрешность"),     # ошибка предсказание ≈ погрешность предсказание
    ("предсказание", "прогнозирование"),  # предсказание значение ≈ прогнозирование значение
    ("анализ", "обработка"),       # анализ текст ≈ обработка текст
    ("предложение", "фраза"),      # сегментация предложение ≈ сегментация фраза
    ("аннотация", "разметка"),     # аннотация корпус ≈ разметка корпус
    ("семантика", "значение"),     # семантика слово ≈ значение слово
    ("текст", "документ"),         # анализ текст ≈ анализ документ
    ("ответ", "реакция"),          # ответ система ≈ реакция система
    ("обучение", "тренировка"),    # обучение модель ≈ тренировка модель
    ("оценка", "метрика"),         # оценка модель ≈ метрика модель
    ("вектор", "представление"),   # вектор слово ≈ представление слово
    ("взвешивание", "оценка"),     # взвешивание признак ≈ оценка признак
    ("файл", "документ"),          # обработка файл ≈ обработка документ
    ("схожесть", "сходство"),
    ("сходство", "близость"),
    ("схожесть", "близость"),
    ("текстовый данный", "текст"),
    ("эпоха", "период"),
    ("изучение", "анализ"),
    ("оценивание", "оценка"),
    ("ресурс", "источник"),
    ("обучаемый данный", "данный для обучение"),
    ("рассеянный", "многонаправленный")
]

NON_DISTINCTIVE_ADJECTIVES = [
    "релевантный",
    "точный",
    "эффективный",
    "популярный",
    "полезный",
    "качественный"
]

POSITIONAL_CONTEXT_ADJECTIVES = [
    "исходный",
    "входной",
    "выходной",
    "обучающий",
    "тестовый",
    "валидационный",
    "начальный",
    "конечный",
    "промежуточный",
    "первичный",
    "результирующий"
]

ACTION_NOUNS = [
    # обучение и адаптация
    "обучение",
    "дообучение",
    "переобучение",
    "самообучение",
    "адаптация",
    "обновление",

    # оценка и анализ
    "оценка",
    "оценивание",
    "анализ",
    "измерение",
    "расчёт",
    "подсчёт",
    "расчет",
    "подсчет",
    "наблюдение",

    # прогноз, вывод
    "предсказание",
    "прогнозирование",
    "вывод",
    "генерация",
    "ответ",
    "завершение",

    # обработка
    "обработка",
    "предобработка",
    "постобработка",
    "трансформация",
    "конвертация",
    "декодирование",
    "кодирование",
    "перевод",
    "токенизация",
    "нормализация",
    "лемматизация",
    "стемминг",
    "сжатие",

    # извлечение и выделение
    "выделение",
    "извлечение",
    "аннотация",
    "разметка",
    "теггинг",
    "сопоставление",
    "идентификация",

    # классификация и кластеризация
    "классификация",
    "категоризация",
    "кластеризация",
    "ранжирование",
    "поиск",
    "фильтрация",
    "регрессия",
    "отнесение",

    # обнаружение и распознавание
    "обнаружение",
    "распознавание",
    "определение",
    "локализация",
    "дискриминация",
    "проверка",

    # оптимизация и настройка
    "оптимизация",
    "регуляризация",
    "настройка",
    "калибровка",
    "параметризация",
    "селекция",

    # сравнение
    "сравнение",
    "сопоставление",
    "сверка",

    # интерпретация и объяснение
    "интерпретация",
    "объяснение",
    "инференция",
    "интерполяция",

    # агрегация и структурирование
    "агрегация",
    "структурирование",
    "упорядочивание",
    "группировка",
    "моделирование",
    "построение",

    # усиление и редактирование
    "аугментация",
    "расширение",
    "редактирование",
    "переформулирование",
    "перефразирование",
    "переименование",
    "удаление",

    # искажение и модификация
    "искажение",
    "модификация",
    "перестановка"
]

CONCRETIZATION_NOUNS = [
    # Основные
    "версия",
    "вариант",
    "реализация",
    "модификация",
    "экземпляр",
    "подтип",
    "вариация",
    "конфигурация",

    # Архитектурные
    "структура",
    "архитектура",
    "модель",
    "алгоритм",
    "подход",
    "механизм",

    # Иерархические / подклассы
    "тип",
    "вид",
    "форма",
    "категория",
    "модификация",
    "класс",
    "уровень",

    # Выпуски / итерации
    "издание",
    "ревизия",
    "выпуск",
    "редакция",
    "инстанция",

    # В программной/технической сфере
    "настройка",
    "объект",
    "модуль",
    "блок",
    "узел",
    "компонент",
    "сборка"
]



ALL_MARKERS = HYPONYM_MARKERS + RELATED_MARKERS

def normalize(phrase):
    """Normalize phrase: lowercase and collapse whitespace"""
    return re.sub(r"\s+", " ", phrase.strip().lower())

def action_applied_to_entity(p1, p2):
    norm1 = normalize(p1)
    norm2 = normalize(p2)
    for action in ACTION_NOUNS:
        if norm1.startswith(action + " ") and norm1[len(action)+1:] == norm2:
            return True
        if norm2.startswith(action + " ") and norm2[len(action)+1:] == norm1:
            return True
    return False

def phrase_has_concretization_noun(p1, p2):
    norm1 = normalize(p1)
    norm2 = normalize(p2)
    words1 = norm1.split()
    words2 = norm2.split()

    for noun in CONCRETIZATION_NOUNS:
        # p1 = noun + p2
        if words1[:1] == [noun] and " ".join(words1[1:]) == norm2:
            return "hypernym"
        if words2[:1] == [noun] and " ".join(words2[1:]) == norm1:
            return "hyponym"
    return None


def starts_with_modifier(full_phrase, base_phrase):
    """
    Check if full_phrase equals "<modifier> base_phrase"
    """
    for marker in ALL_MARKERS:
        if normalize(full_phrase) == f"{marker} {normalize(base_phrase)}":
            return marker
    return None

def has_adjectival_prefix(full_phrase, base_phrase):
    for adj in HYPONYM_ADJECTIVES:
        if normalize(full_phrase) == f"{adj} {normalize(base_phrase)}":
            return adj
    return None


def phrases_equivalent_synonyms(p1, p2):
    norm1 = normalize(p1)
    norm2 = normalize(p2)
    for a, b in SYNONYM_EQUIVALENTS:
        swapped1 = norm1.replace(a, b)
        swapped2 = norm1.replace(b, a)
        if norm2 == swapped1 or norm2 == swapped2:
            return True
    return False

def differs_by_single_nonessential_adj(p1, p2):
    """
    Check if p1 and p2 differ only by one non-distinctive adjective
    (regardless of position or total length).
    """
    words1 = set(normalize(p1).split())
    words2 = set(normalize(p2).split())

    diff1 = words1 - words2
    diff2 = words2 - words1

    # Only one word is different in either direction
    if len(diff1) + len(diff2) != 1:
        return False

    only_diff = list(diff1.union(diff2))[0]
    return only_diff in NON_DISTINCTIVE_ADJECTIVES

def differs_by_single_positional_adj(p1, p2):
    """
    Returns 'hyponym' or 'hypernym' if the phrases differ by exactly one word,
    and that word is in POSITIONAL_CONTEXT_ADJECTIVES.
    """
    words1 = set(normalize(p1).split())
    words2 = set(normalize(p2).split())

    diff1 = words1 - words2
    diff2 = words2 - words1

    # Check for one-word difference in either direction
    if len(diff1) == 1 and len(diff2) == 0:
        word = list(diff1)[0]
        if word in POSITIONAL_CONTEXT_ADJECTIVES:
            return "hypernym"
    elif len(diff2) == 1 and len(diff1) == 0:
        word = list(diff2)[0]
        if word in POSITIONAL_CONTEXT_ADJECTIVES:
            return "hyponym"
    return None


def correct_relation(key, phrase, current_relation):
    """
    Determine corrected relation based on modifier patterns
    """
    norm_key = normalize(key)
    norm_phrase = normalize(phrase)

    # # Case 1: key = "<modifier> phrase" → phrase is a hypernym
    # marker = starts_with_modifier(norm_key, norm_phrase)
    # if marker:
    #     return "hypernym" if marker in HYPONYM_MARKERS else "related"

    # # Case 2: phrase = "<modifier> key" → phrase is a hyponym
    # marker = starts_with_modifier(norm_phrase, norm_key)
    # if marker:
    #     return "hyponym" if marker in HYPONYM_MARKERS else "related"

    # # Case 3: key = "<adjective> phrase" → phrase is hypernym
    # adj = has_adjectival_prefix(key, phrase)
    # if adj:
    #     return "hypernym"

    # if phrases_equivalent_synonyms(key, phrase):
    #     return "synonym"

    # if differs_by_single_nonessential_adj(key, phrase):
    #     return "synonym"

    if action_applied_to_entity(key, phrase):
        return "related"

    relation = phrase_has_concretization_noun(key, phrase)
    if relation:
        return relation


    relation = differs_by_single_positional_adj(key, phrase)
    if relation:
        return relation

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

import json
import re

INPUT_PATH = "marked_relations.json"
OUTPUT_PATH = "marked_relations.json"

ANTONYM_EQUIVALENTS = [
    ("позитивный", "негативный"),
    ("правильный", "ошибочный"),
    ("корректный", "ошибочный"),
    ("включённый", "исключённый"),
    ("максимальный", "минимальный"),
    ("высокий", "низкий"),
    ("точный", "приблизительный"),
    ("точный", "грубый"),
    ("полный", "частичный"),
    ("всеобщий", "частный"),
    ("глобальный", "локальный"),
    ("общий", "специфический"),
    ("конкретный", "абстрактный"),
    ("сильный", "слабый"),
    ("стабильный", "переменный"),
    ("одинаковый", "разный"),
    ("одинаковый", "отличный"),
    ("одинаковый", "неоднородный"),
    ("совпадающий", "расходящийся"),
    ("наличие", "отсутствие"),
    ("присутствие", "отсутствие"),
    ("запуск", "остановка"),
    ("появление", "исчезновение"),
    ("усложнение", "упрощение"),
    ("расширение", "сжатие"),
    ("увеличение", "уменьшение"),
    ("повышение", "снижение"),
    ("рост", "падение"),
    ("начало", "конец"),
    ("активный", "пассивный"),
    ("прямой", "обратный"),
    ("прямая", "обратная"),
    ("прямая связь", "обратная связь"),
    ("вход", "выход"),
    ("входной", "выходной"),
    ("предсказание", "истина"),
    ("правда", "ложь"),
    ("истинный", "ложный"),
    ("разрешение", "запрет"),
    ("положительный", "отрицательный"),
    ("поверхностный", "глубокий"),
    ("настоящий", "синтетический"),
    ("реальный", "синтетический"),
    ("естественный", "сгенерированный"),
    ("достоверный", "поддельный"),
    ("согласованный", "противоречивый"),
    ("однозначный", "многозначный"),
    ("оптимальный", "неподходящий"),
    ("основной", "второстепенный"),
    ("главный", "побочный"),
    ("быстрый", "медленный"),
    ("автоматический", "ручной"),
    ("автоматизированный", "ручной"),
    ("упорядоченный", "хаотичный"),
    ("детерминированный", "случайный"),
    ("систематический", "случайный"),
    ("простой", "сложный"),
    ("плотный", "разреженный"),
    ("симметричный", "асимметричный"),
    ("двунаправленный", "односторонний"),
    ("большой", "малый"),
    ("включение", "исключение"),
    ("добавление", "удаление"),
    ("включение", "исключение"),
    ("равномерный", "неравномерный"),
    ("обязательный", "факультативный"),
    ("локальный", "глобальный"),
    ("встроенный", "внешний"),
    ("мягкий", "жесткий"),
    ("статический", "динамический"),
    ("жесткий", "гибкий"),
    ("долгосрочный", "краткосрочный"),
    ("централизованный", "распределённый"),
    ("внутренний", "внешний"),
    ("связанный", "независимый"),
    ("связный", "несвязный"),
    ("линейный", "нелинейный"),
    ("основной", "вспомогательный"),
    ("прямой", "косвенный"),
    ("интенсивный", "поверхностный"),
    ("качественный", "количественный"),
    ("фиксированный", "адаптивный"),
    ("универсальный", "специализированный"),
    ("простой", "комплексный"),
    ("полный", "фрагментарный"),
    ("избыточный", "недостаточный"),
    ("надежный", "ненадежный"),
    ("устойчивый", "чувствительный"),
    ("высокоуровневый", "низкоуровневый"),
    ("явный", "неявный"),
    ("обобщённый", "частный"),
    ("определённый", "неопределённый"),
    ("аккуратный", "шумный"),
    ("однородный", "разнородный"),
    ("объективный", "субъективный"),
    ("центральный", "периферийный"),
    ("непрерывный", "дискретный"),
    ("однонаправленный", "двунаправленный"),
    ("фокусированный", "рассеянный"),
    ("устный", "письменный"),
    ("буквальный", "переносный"),
    ("естественный", "искусственный")
]


def normalize(phrase):
    return " ".join(phrase.strip().lower().split())

def split_words(phrase):
    return normalize(phrase).split()

def is_negated_antonym(key, phrase):
    key_words = split_words(key)
    phrase_words = split_words(phrase)

    if len(key_words) < 2 or len(phrase_words) < 2:
        return False

    if phrase_words[0].startswith("не") and phrase_words[0][2:] == key_words[0]:
        if phrase_words[1:] == key_words[1:]:
            return True

    if key_words[0].startswith("не") and key_words[0][2:] == phrase_words[0]:
        if key_words[1:] == phrase_words[1:]:
            return True

    return False

def is_explicit_antonym_equivalent(key, phrase):
    norm1 = normalize(key)
    norm2 = normalize(phrase)

    for a, b in ANTONYM_EQUIVALENTS:
        swapped1 = norm1.replace(a, b)
        swapped2 = norm1.replace(b, a)
        if norm2 == swapped1 or norm2 == swapped2:
            return True
    return False

def ensure_entry(data, key, phrase, relation):
    norm_key = normalize(key)
    norm_phrase = normalize(phrase)
    for entry in data:
        if normalize(entry["key"]) == norm_key:
            for p in entry["phrases"]:
                if normalize(p["phrase"]) == norm_phrase:
                    if p["relation"] != relation:
                        p["relation"] = relation
                        return True
                    return False
            entry["phrases"].append({"phrase": phrase, "relation": relation})
            return True
    data.append({"key": key, "phrases": [{"phrase": phrase, "relation": relation}]})
    return True

with open(INPUT_PATH, "r", encoding="utf-8") as f:
    data = json.load(f)

changes = []

for entry in data:
    key = entry["key"]
    for p in entry["phrases"]:
        phrase = p["phrase"]
        if is_negated_antonym(key, phrase) or is_explicit_antonym_equivalent(key, phrase):
            if p["relation"] != "antonym":
                changes.append((phrase, key, p["relation"], "antonym"))
                p["relation"] = "antonym"
            changed = ensure_entry(data, phrase, key, "antonym")
            if changed:
                changes.append((key, phrase, "?", "antonym (reverse)"))

with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
    json.dump(data, f, ensure_ascii=False, indent=4)

print("=== Antonymic corrections ===")
for phrase, key, old, new in changes:
    print(f"'{phrase}' -> '{key}': {old} → {new}")

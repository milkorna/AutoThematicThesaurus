import json
from collections import defaultdict

# Загрузим исходные данные
with open("relations.json", "r", encoding="utf-8") as f:
    data = json.load(f)

# Построим индекс key -> phrases и phrase -> key
key_to_phrases = {entry["key"]: entry["phrases"] for entry in data}
phrase_to_keys = defaultdict(list)

for entry in data:
    key = entry["key"]
    for phrase_entry in entry["phrases"]:
        phrase_to_keys[phrase_entry["phrase"]].append((key, phrase_entry["relation"]))

# Обратим связи
for entry in data:
    key = entry["key"]
    existing_phrases = {p["phrase"]: p["relation"] for p in entry["phrases"]}

    # Найдём все "обратные" связи
    for phrase, rel in list(existing_phrases.items()):
        if phrase in key_to_phrases:
            inverse_rel = None
            if rel == "hypernym":
                inverse_rel = "hyponym"
            elif rel == "hyponym":
                inverse_rel = "hypernym"
            elif rel in {"synonym", "antonym", "related"}:
                inverse_rel = rel

            if inverse_rel:
                # Проверим, существует ли уже обратная связь
                reverse_entry = key_to_phrases[phrase]
                if not any(p["phrase"] == key and p["relation"] == inverse_rel for p in reverse_entry):
                    reverse_entry.append({"phrase": key, "relation": inverse_rel})

# Сохраняем обновлённые данные
with open("relations.json", "w", encoding="utf-8") as f:
    json.dump(
        [{"key": k, "phrases": v} for k, v in key_to_phrases.items()],
        f,
        indent=4,
        ensure_ascii=False
    )
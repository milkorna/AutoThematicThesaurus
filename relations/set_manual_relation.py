import json

INPUT_PATH = "marked_relations_manual.json"
OUTPUT_PATH = "marked_relations_manual.json"

key_phrase = "область семантический анализ".strip()
target_phrase = "область компьютерный анализ".strip()
new_relation = "related".strip()

assert new_relation in {"synonym", "hypernym", "hyponym", "related", "not_related"}, "Invalid relation type"

REVERSE_RELATION = {
    "synonym": "synonym",
    "hypernym": "hyponym",
    "hyponym": "hypernym",
    "related": "related",
    "not_related": "not_related"
}

def normalize(phrase):
    return " ".join(phrase.strip().lower().split())

norm_key = normalize(key_phrase)
norm_target = normalize(target_phrase)
reverse_relation = REVERSE_RELATION[new_relation]

with open(INPUT_PATH, "r", encoding="utf-8") as f:
    data = json.load(f)

def update_or_insert(key, phrase, relation):
    norm_key_local = normalize(key)
    norm_phrase_local = normalize(phrase)
    for entry in data:
        if normalize(entry["key"]) == norm_key_local:
            for p in entry["phrases"]:
                if normalize(p["phrase"]) == norm_phrase_local:
                    if p["relation"] != relation:
                        print(f"[UPDATE] '{phrase}' → '{key}': {p['relation']} → {relation}")
                        p["relation"] = relation
                        return True
                    else:
                        print(f"[SKIP] Отношение уже установлено: {p['relation']}")
                        return False
            # Если фраза не найдена
            print(f"[ADD] Добавлено новое отношение: '{phrase}' → '{key}' = {relation}")
            entry["phrases"].append({"phrase": phrase, "relation": relation})
            return True
    # Если ключ не найден — создаём новую запись
    print(f"[NEW] Создана новая запись: key = '{key}'")
    data.append({
        "key": key,
        "phrases": [{"phrase": phrase, "relation": relation}]
    })
    return True

changed_1 = update_or_insert(key_phrase, target_phrase, new_relation)
changed_2 = update_or_insert(target_phrase, key_phrase, reverse_relation)

if changed_1 or changed_2:
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=4)
    print(f"[OK] Изменения сохранены в {OUTPUT_PATH}")
else:
    print("[NO CHANGES] Ничего не изменилось.")

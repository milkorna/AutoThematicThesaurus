import json

def find_hypernym_hyponym_mismatches(file_path):
    """
    Проверяет симметрию гипероним-гипоним только для взаимных пар.

    Если A -> B (hypernym) и B -> A — тогда проверяем, что B -> A помечено как hyponym.
    И наоборот. Несогласованности выводятся.
    """
    # Загружаем JSON
    with open(file_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    # Словарь: ключ -> {фраза: тип отношения}
    relation_dict = {}
    for entry in data:
        key = entry["key"]
        relation_dict[key] = {}
        for phrase_data in entry["phrases"]:
            phrase = phrase_data["phrase"]
            relation = phrase_data.get("relation")
            if relation:
                relation_dict[key][phrase] = relation

    # Поиск взаимных несогласованных пар
    for key_A, phrases_A in relation_dict.items():
        for key_B, rel_A_B in phrases_A.items():
            # Проверка, только если key_B также имеет отношение к key_A
            if key_B in relation_dict and key_A in relation_dict[key_B]:
                rel_B_A = relation_dict[key_B][key_A]
                # Проверка пар hypernym ↔ hyponym
                if rel_A_B == "hypernym" and rel_B_A != "hyponym":
                    print(f"\nНесогласованность: {key_A} -> {key_B} (hypernym), но {key_B} -> {key_A} = {rel_B_A}")
                elif rel_A_B == "hyponym" and rel_B_A != "hypernym":
                    print(f"\nНесогласованность: {key_A} -> {key_B} (hyponym), но {key_B} -> {key_A} = {rel_B_A}")

if __name__ == "__main__":
    file_path = "marked_relations_manual.json"
    find_hypernym_hyponym_mismatches(file_path)

import json

# Параметры
INPUT_FILE = "mask_deep_pavlov.json"
OUTPUT_FILE = "synonyms_mask_deep_pavlov.json"
THRESHOLD = 0.87

def main():
    # 1. Чтение исходного JSON
    with open(INPUT_FILE, "r", encoding="utf-8") as f:
        data = json.load(f)

    # 2. Фильтрация и отбор непустых
    result = {}
    for key, entry in data.items():
        # Оставляем только синонимы с similarity > порога
        filtered_syns = [
            syn for syn in entry.get("phrases", [])
            if syn.get("similarity_for_masked_word", 0) > THRESHOLD
        ]
        # Если после фильтрации список пуст — пропускаем запись
        if not filtered_syns:
            continue

        result[key] = {
            "key": entry.get("key"),
            "is_term_manual": entry.get("is_term_manual"),
            "oof_prob_class": entry.get("oof_prob_class"),
            "phrases": filtered_syns
        }

    # 3. Запись в новый файл
    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=4)

if __name__ == "__main__":
    main()

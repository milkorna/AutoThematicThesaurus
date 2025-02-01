import pandas as pd
import re

# Пути к файлам
input_path = "/home/milkorna/Documents/AutoThematicThesaurus/one_word_terms.xlsx"
output_path = "/home/milkorna/Documents/AutoThematicThesaurus/one_word_terms_cleaned.xlsx"

# Загрузка данных
df = pd.read_excel(input_path)

# Преобразуем данные в словарь по корневым словам
word_groups = {}
pattern = re.compile(r'(а|и|ов)$')

for _, row in df.iterrows():
    word = row["noun"]
    base_word = re.sub(pattern, '', word)  # Убираем окончание

    if base_word not in word_groups:
        word_groups[base_word] = {
            "noun": base_word,
            "phrase_count": 0,
            "tf_idf": [],
            "prob_mean": [],
            "prob_max": [],
            "prob_min": [],
            "is_term": 0
        }

    word_groups[base_word]["phrase_count"] += row["phrase_count"]
    word_groups[base_word]["tf_idf"].append(row["tf_idf"])
    word_groups[base_word]["prob_mean"].append(row["prob_mean"])
    word_groups[base_word]["prob_max"].append(row["prob_max"])
    word_groups[base_word]["prob_min"].append(row["prob_min"])
    word_groups[base_word]["is_term"] = max(word_groups[base_word]["is_term"], row.get("is_term", 0))

# Усредняем числовые значения
filtered_data = []
for word, data in word_groups.items():
    data["tf_idf"] = sum(data["tf_idf"]) / len(data["tf_idf"])
    data["prob_mean"] = sum(data["prob_mean"]) / len(data["prob_mean"])
    data["prob_max"] = sum(data["prob_max"]) / len(data["prob_max"])
    data["prob_min"] = sum(data["prob_min"]) / len(data["prob_min"])
    filtered_data.append(data)

# Создание итогового DataFrame и сохранение
filtered_df = pd.DataFrame(filtered_data)
filtered_df.to_excel(output_path, index=False)

print(f"Очищенный файл сохранён в {output_path}")

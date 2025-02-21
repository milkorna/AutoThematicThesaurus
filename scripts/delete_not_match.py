import json
import pandas as pd

# Пути к файлам
json_path = r"/home/milkorna/Documents/AutoThematicThesaurus/my_data/final_relations_manual_check.json"
excel_path = r"/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"

# Загрузка данных из Excel
df = pd.read_excel(excel_path, engine='openpyxl')
valid_keys = set(df['key'].astype(str))  # Множество допустимых ключей

# Загрузка JSON-файла
with open(json_path, "r", encoding="utf-8") as f:
    data = json.load(f)

# Фильтрация данных
filtered_data = [entry for entry in data if entry["key"] in valid_keys]

# Сохранение отфильтрованных данных
output_path = json_path.replace(".json", "_filtered.json")
with open(output_path, "w", encoding="utf-8") as f:
    json.dump(filtered_data, f, ensure_ascii=False, indent=4)

print(f"Фильтрованный JSON сохранён в: {output_path}")
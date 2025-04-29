import pandas as pd
import json

excel_path = './data.xlsx'
json_path = './relations.json'

df = pd.read_excel(excel_path)
valid_phrases = set(df['phrase'].dropna().str.strip().str.lower())

with open(json_path, 'r', encoding='utf-8') as f:
    relations = json.load(f)

filtered_relations = []
for entry in relations:
    key = entry.get('key', '').strip().lower()
    phrases = entry.get('phrases', [])

    if key not in valid_phrases:
        continue

    filtered_phrases = [
        p for p in phrases
        if p.get('phrase', '').strip().lower() in valid_phrases
    ]

    if filtered_phrases:
        filtered_relations.append({
            'key': entry['key'],
            'phrases': filtered_phrases
        })

with open(json_path, 'w', encoding='utf-8') as f:
    json.dump(filtered_relations, f, ensure_ascii=False, indent=4)

print(f"Готово! Обновлённый файл сохранён в {json_path}")

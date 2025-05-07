import pandas as pd
import re
import os
import json

def slugify(phrase: str) -> str:
    phrase = phrase.lower()
    phrase = re.sub(r'[^a-z0-9а-яё0-9]+', '-', phrase)
    phrase = re.sub(r'-+', '-', phrase)
    phrase = phrase.strip('-')
    return phrase or "phrase"

# Функция для загрузки и обработки отношений из JSON-файла.
def load_relations(json_path: str, existing_phrases: set) -> dict:
    """
    Возвращает словарь вида:
    {
       "фраза": {
           "hypernym": { 'фраза1', 'фраза2', ... },
           "synonym": { ... },
           ...
       },
       ...
    }
    existing_phrases – множество фраз, которые есть в основном Excel-файле (для проверки, существуют ли страницы)
    """
    inverse_relation = {
        "hypernym": "hyponym",
        "hyponym": "hypernym",
        "synonym": "synonym",
        "antonym": "antonym",
        "related": "related"
    }
    relations_map = {}  # основной словарь отношений
    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except Exception as e:
        print(f"Ошибка при загрузке файла {json_path}: {e}")
        return relations_map

    for entry in data:
        main_phrase = entry.get("key", "").strip()
        if not main_phrase:
            continue
        # Инициализируем записи для главной фразы, если их ещё нет
        if main_phrase not in relations_map:
            relations_map[main_phrase] = {}

        for item in entry.get("phrases", []):
            related_phrase = item.get("phrase", "").strip()
            relation_type = item.get("relation", "").strip()
            if not related_phrase or not relation_type:
                continue
            # Добавляем отношение для главной фразы
            relations_map[main_phrase].setdefault(relation_type, set()).add(related_phrase)
            # Добавляем обратное отношение для связанной фразы
            inv = inverse_relation.get(relation_type, relation_type)
            relations_map.setdefault(related_phrase, {})
            relations_map[related_phrase].setdefault(inv, set()).add(main_phrase)
    return relations_map

# Словарь для отображения типов отношений на русский язык
relation_labels = {
    "synonym": "Синонимы",
    "hypernym": "Гиперонимы",
    "hyponym": "Гипонимы",
    "antonym": "Антонимы",
    "related": "Связанные"
}

def generate_context_pages(excel_path: str, output_folder: str, relations_json: str):
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    df = pd.read_excel(excel_path)

    # Проверяем наличие обязательных колонок
    required_columns = ['phrase', 'context', 'documents']
    for col in required_columns:
        if col not in df.columns:
            raise ValueError(f"Missing required column '{col}' in the Excel file.")

    # Подготовка отображаемых значений с использованием normalized_form, если он существует
    phrase2norm = {}
    has_normalized = ('normalized_form' in df.columns)
    for idx, row in df.iterrows():
        phrase_raw = str(row['phrase']).strip()
        if has_normalized and not pd.isna(row['normalized_form']):
            norm_val = str(row['normalized_form']).strip()
            phrase2norm[phrase_raw] = norm_val if norm_val else phrase_raw
        else:
            phrase2norm[phrase_raw] = phrase_raw

    # Множество фраз, для которых генерируются страницы (будем использовать для проверки наличия гиперссылки)
    existing_phrases = set(phrase2norm.keys())

    # Загружаем отношения из JSON-файла
    relations_map = load_relations(relations_json, existing_phrases)

    # Генерация HTML-страниц для каждой фразы
    for idx, row in df.iterrows():
        phrase = str(row['phrase']).strip()
        display_text = phrase2norm[phrase]

        context_data = str(row['context']).strip() if not pd.isna(row['context']) else ""
        contexts = [c.strip() for c in context_data.split('||') if c.strip()]

        documents_data = str(row['documents']).strip() if not pd.isna(row['documents']) else ""
        doc_ids = [doc.strip() for doc in documents_data.split(',') if doc.strip()]

        slug = slugify(phrase)

        html_content = []
        html_content.append(f"""<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <title>{display_text}</title>
    <link rel="stylesheet" href="../style.css">
</head>
<body>
<div class="container">
    <h1>{display_text}</h1>
""")

        # Вывод контекстов, если они заданы
        if contexts:
            html_content.append("<h2>Примеры употребления:</h2>")
            html_content.append("<ul class='context-list'>")
            n = min(len(contexts), len(doc_ids))
            for i in range(n):
                c = contexts[i]
                article_id = doc_ids[i]
                source_link = f"https://habr.com/ru/articles/{article_id}/"
                html_content.append(
                    f"<li class='context-item'>{c} "
                    f"(Источник: <a href='{source_link}' target='_blank'>{article_id}</a>)</li>"
                )
            if len(contexts) > n:
                for c in contexts[n:]:
                    html_content.append(f"<li class='context-item'>{c} (Источник не указан)</li>")
            html_content.append("</ul>")
        else:
            html_content.append("<p><em>Контекст не указан.</em></p>")

        # --- Блок вывода отношений ---
        if phrase in relations_map:
            html_content.append("<div class='relations'>")
            html_content.append("<hr>")
            html_content.append("<h2>Отношения:</h2>")
            for rel_type in ["synonym", "hypernym", "hyponym", "antonym", "related"]:
                if rel_type in relations_map[phrase]:
                    related_phrases = sorted(relations_map[phrase][rel_type])
                    label = relation_labels.get(rel_type, rel_type)
                    html_content.append(f"<h3>{label}:</h3>")
                    html_content.append("<ul class='relation-list'>")
                    for related in related_phrases:
                        # Используем normalized_form из data.xlsx, если такая фраза присутствует
                        display_related = phrase2norm.get(related, related)
                        if related in existing_phrases:
                            related_slug = slugify(related)
                            link = f"{related_slug}.html"
                            html_content.append(f"<li><a href='{link}'>{display_related}</a></li>")
                        else:
                            html_content.append(f"<li>{display_related}</li>")
                    html_content.append("</ul>")
            html_content.append("</div>")
        # --- Конец блока отношений ---

        # Ссылка для возврата к списку фраз
        html_content.append("""
    <a class="back-link" href="../thesaurus.html">Вернуться к списку фраз</a>
</div>
</body>
</html>
        """)

        filename = os.path.join(output_folder, f"{slug}.html")
        with open(filename, 'w', encoding='utf-8') as f:
            f.write("\n".join(html_content))

    print(f"Context pages successfully created in: {output_folder}")

if __name__ == "__main__":
    excel_path = "data.xlsx"
    output_folder = "pages"
    relations_json = "/home/milkorna/Documents/AutoThematicThesaurus/relations/marked_relations_manual.json"
    generate_context_pages(excel_path, output_folder, relations_json)

import pandas as pd
import re
import os

def slugify(phrase: str) -> str:
    phrase = phrase.lower()
    phrase = re.sub(r'[^a-z0-9а-яё0-9]+', '-', phrase)
    phrase = re.sub(r'-+', '-', phrase)
    phrase = phrase.strip('-')
    return phrase or "phrase"

def generate_context_pages(excel_path: str, output_folder: str):
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    df = pd.read_excel(excel_path)

    required_columns = ['phrase', 'context', 'documents']
    for col in required_columns:
        if col not in df.columns:
            raise ValueError(f"Missing required column '{col}' in the Excel file.")

    phrase2norm = {}
    has_normalized = ('normalized_form' in df.columns)

    for idx, row in df.iterrows():
        phrase_raw = str(row['phrase']).strip()
        if has_normalized and not pd.isna(row['normalized_form']):
            norm_val = str(row['normalized_form']).strip()
            phrase2norm[phrase_raw] = norm_val if norm_val else phrase_raw
        else:
            phrase2norm[phrase_raw] = phrase_raw

    df = pd.read_excel(excel_path)

    for col in required_columns:
        if col not in df.columns:
            raise ValueError(f"Missing required column '{col}' in the Excel file.")

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
    generate_context_pages(excel_path, output_folder)

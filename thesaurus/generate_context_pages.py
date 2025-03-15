import pandas as pd
import re
import os
import math

def slugify(phrase: str) -> str:
    """
    Generates a URL-friendly slug from a given phrase.
    - Converts the phrase to lowercase.
    - Replaces non-alphanumeric characters with hyphens ('-').
    - Reduces multiple consecutive hyphens to a single one.
    - Trims leading and trailing hyphens.
    Returns a cleaned string suitable for filenames or URLs.
    """
    phrase = phrase.lower()
    phrase = re.sub(r'[^a-z0-9а-яё0-9]+', '-', phrase) # Replace non-alphanumeric characters with '-'
    phrase = re.sub(r'-+', '-', phrase)                # Reduce multiple hyphens
    phrase = phrase.strip('-')                         # Remove leading and trailing hyphens
    return phrase or "phrase"                          # Ensure a valid filename if input is empty

def generate_context_pages(excel_path: str, output_folder: str):
    """
    Reads phrase data from an Excel file and generates HTML context pages for each phrase.
    - Extracts "phrase", "context", and "documents" columns.
    - Creates an output directory if it doesn't exist.
    - Generates an HTML file for each phrase, including its example contexts and source links.
    - Saves generated HTML files in the specified output folder.
    """
    # Ensure the output directory exists
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    # Load data from the Excel file
    df = pd.read_excel(excel_path)

    # Validate required columns
    required_columns = ['phrase', 'context', 'documents']
    for col in required_columns:
        if col not in df.columns:
            raise ValueError(f"Missing required column '{col}' in the Excel file.")

    # Process each phrase entry in the dataset
    for idx, row in df.iterrows():
        phrase = str(row['phrase']).strip()

        # Extract and clean context examples
        context_data = str(row['context']).strip() if not pd.isna(row['context']) else ""
        contexts = [c.strip() for c in context_data.split('||') if c.strip()]

        # Extract and clean document IDs
        documents_data = str(row['documents']).strip() if not pd.isna(row['documents']) else ""
        doc_ids = [doc.strip() for doc in documents_data.split(',') if doc.strip()]

        # Generate a filename-friendly slug for the phrase
        slug = slugify(phrase)

        # Construct the HTML content for the phrase
        html_content = []
        html_content.append(f"""<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <title>{phrase}</title>
    <!-- Путь к style.css предполагает, что style.css лежит на уровень выше папки pages/ -->
    <link rel="stylesheet" href="../style.css">
</head>
<body>
<div class="container">
    <h1>{phrase}</h1>
""")

        # Add context examples if available
        if contexts:
            html_content.append("<h2>Примеры употребления:</h2>")
            html_content.append("<ul class='context-list'>")

            # Ensure document IDs align with contexts
            n = min(len(contexts), len(doc_ids))

            # Generate list items with source links
            for i in range(n):
                c = contexts[i]
                article_id = doc_ids[i]
                source_link = f"https://habr.com/ru/articles/{article_id}/"
                html_content.append(
                    f"<li class='context-item'>{c} "
                    f"(Источник: <a href='{source_link}' target='_blank'>{article_id}</a>)</li>"
                )

            # Add remaining contexts without a source if extra examples exist
            if len(contexts) > n:
                for c in contexts[n:]:
                    html_content.append(f"<li class='context-item'>{c} (Источник не указан)</li>")

            html_content.append("</ul>")
        else:
            html_content.append("<p><em>Контекст не указан.</em></p>")

        # Add navigation link
        html_content.append("""
    <a class="back-link" href="../thesaurus.html">Вернуться к списку фраз</a>
</div>
</body>
</html>
        """)

        # Write the HTML content to a file
        filename = os.path.join(output_folder, f"{slug}.html")
        with open(filename, 'w', encoding='utf-8') as f:
            f.write("\n".join(html_content))

    print(f"Context pages successfully created in: {output_folder}")

if __name__ == "__main__":
    excel_path = "data.xlsx"
    output_folder = "pages"
    generate_context_pages(excel_path, output_folder)

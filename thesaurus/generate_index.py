import pandas as pd
import os
import re

def slugify(phrase: str) -> str:
    """
    Converts a phrase into a URL-friendly slug:
    - Converts to lowercase.
    - Replaces non-alphanumeric characters with hyphens ('-').
    - Collapses multiple hyphens into a single one.
    - Strips leading and trailing hyphens.
    Returns a valid filename or URL-friendly string.
    """
    phrase = phrase.lower()
    phrase = re.sub(r'[^a-z0-9а-яё0-9]+', '-', phrase)
    phrase = re.sub(r'-+', '-', phrase)
    phrase = phrase.strip('-')
    return phrase or "phrase"

def generate_index(excel_path: str, output_html: str, pages_folder: str):
    """
    Generates an HTML thesaurus index page from an Excel file.
    - Reads phrases from the Excel file.
    - Identifies hierarchical relationships between phrases (parent-child).
    - Creates an HTML page with links to individual phrase context pages.
    - Saves the generated index page to the specified output path.
    """
    # Load data from Excel
    df = pd.read_excel(excel_path)

    # Ensure the required column exists
    if 'phrase' not in df.columns:
        raise ValueError("В файле Excel нет колонки 'phrase'.")

    # Extract unique phrases, ensuring no duplicates or empty values
    phrases = sorted(set(str(x).strip() for x in df['phrase'].dropna() if str(x).strip()))

    # Mapping parent phrases to child phrases (phrases that contain the parent as a substring)
    children_map = {p: set() for p in phrases}
    is_child = {p: False for p in phrases}

    # Identify hierarchical relationships:
    # If phrase p is a substring of phrase q (p != q), q is considered a child of p.
    for p in phrases:
        for q in phrases:
            if p != q and p in q:
                children_map[p].add(q)
                is_child[q] = True

    # Construct the HTML content for the thesaurus index
    html_content = []
    html_content.append("""<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <title>Thesaurus</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
    <h1>Тезаурус по компьютерной лингвистике</h1>
    <ul class="phrase-list">
""")

    # Display only top-level phrases (those that are not marked as children)
    for p in phrases:
        if not is_child[p]: # Ensure the phrase is not a child of another phrase
            slug_p = slugify(p)
            link_p = f"{pages_folder}/{slug_p}.html"

            # Add parent phrase to the list
            html_content.append(f'        <li class="phrase-item"><a href="{link_p}">{p}</a></li>')

            # If the phrase has associated child phrases, list them as sub-items
            children = sorted(children_map[p])
            if children:
                html_content.append('        <ul class="sub-phrase-list">')
                for child in children:
                    slug_child = slugify(child)
                    link_child = f"{pages_folder}/{slug_child}.html"
                    html_content.append(
                        f'            <li class="sub-phrase-item">'
                        f'<a href="{link_child}">{child}</a>'
                        f'</li>'
                    )
                html_content.append('        </ul>')

    # Close the HTML structure
    html_content.append("""    </ul>
</div>
</body>
</html>
""")

    # Save the generated HTML to a file
    with open(output_html, 'w', encoding='utf-8') as f:
        f.write("\n".join(html_content))

    print(f"Thesaurus index page successfully created: {output_html}")

if __name__ == "__main__":
    excel_path = "data.xlsx"
    output_html = "thesaurus.html"
    pages_folder = "pages"
    generate_index(excel_path, output_html, pages_folder)

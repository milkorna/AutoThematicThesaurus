import pandas as pd
import re

def slugify(phrase: str) -> str:
    phrase = phrase.lower()
    phrase = re.sub(r'[^a-z0-9а-яё0-9]+', '-', phrase)
    phrase = re.sub(r'-+', '-', phrase)
    phrase = phrase.strip('-')
    return phrase or "phrase"

def is_subphrase(parent: str, candidate: str) -> bool:
    """
    Определяет, встречается ли parent как последовательность слов внутри candidate,
    игнорируя окончания слов (2 последних символа).
    """
    def stem(word: str, cut: int = 2) -> str:
        return word[:-cut] if len(word) > cut else word

    p_tokens = [stem(word) for word in parent.split()]
    c_tokens = [stem(word) for word in candidate.split()]

    if len(p_tokens) > len(c_tokens):
        return False

    for start_idx in range(len(c_tokens) - len(p_tokens) + 1):
        segment = c_tokens[start_idx : start_idx + len(p_tokens)]
        if segment == p_tokens:
            return True

    return False


def generate_index(excel_path: str, output_html: str, pages_folder: str):
    df = pd.read_excel(excel_path)

    if 'phrase' not in df.columns:
        raise ValueError("В файле Excel нет колонки 'phrase'.")

    phrase2norm = {}
    if 'normalized_form' in df.columns:
        for _, row in df.iterrows():
            p_val = str(row['phrase']).strip()
            if not pd.isna(row['normalized_form']):
                norm_val = str(row['normalized_form']).strip()
                phrase2norm[p_val] = norm_val if norm_val else p_val
            else:
                phrase2norm[p_val] = p_val
    else:
        for _, row in df.iterrows():
            p_val = str(row['phrase']).strip()
            phrase2norm[p_val] = p_val

    phrases = sorted(set(str(x).strip() for x in df['phrase'].dropna() if str(x).strip()))

    children_map = {p: set() for p in phrases}
    is_child = {p: False for p in phrases}

    for p in phrases:
        for q in phrases:
            if p != q and is_subphrase(p, q):
                children_map[p].add(q)
                is_child[q] = True

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

    for p in phrases:
        if not is_child[p]:
            slug_p = slugify(p)
            link_p = f"{pages_folder}/{slug_p}.html"

            display_text = phrase2norm[p]
            html_content.append(f'        <li class="phrase-item"><a href="{link_p}">{display_text}</a></li>')

            children = sorted(children_map[p])
            if children:
                html_content.append('        <ul class="sub-phrase-list">')
                for child in children:
                    slug_child = slugify(child)
                    link_child = f"{pages_folder}/{slug_child}.html"
                    display_text_child = phrase2norm[child]
                    html_content.append(
                        f'            <li class="sub-phrase-item">'
                        f'<a href="{link_child}">{display_text_child}</a>'
                        f'</li>'
                    )
                html_content.append('        </ul>')

    html_content.append("""    </ul>
</div>
</body>
</html>
""")

    with open(output_html, 'w', encoding='utf-8') as f:
        f.write("\n".join(html_content))

    print(f"Thesaurus index page successfully created: {output_html}")

if __name__ == "__main__":
    excel_path = "data.xlsx"
    output_html = "thesaurus.html"
    pages_folder = "pages"
    generate_index(excel_path, output_html, pages_folder)

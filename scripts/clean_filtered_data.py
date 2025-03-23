import pandas as pd

from core.paths import PROJECT_ROOT, PATH_DATA_WITH_OFF

# Path to the output file
PATH_OUT = PROJECT_ROOT / "data_with_oof_cleaned.xlsx"

# List of columns to merge using MAXIMUM
COL_MAX = ["is_term_manual"]
# List of columns to merge using SUM
COL_SUM = ["frequency"]
# List of columns to merge using AVERAGE
COL_MEAN = ["topic_relevance", "centrality_score", "inverse_rarity", "oof_prob_class"]
# List of columns to merge using CONCATENATION
COL_CONCAT = ["context"]

def parse_phrase_into_words(phrase):
    """Splits a phrase into a list of words (by spaces)."""
    return phrase.strip().split()

def join_words_into_phrase(words):
    """Joins a list of words back into a string (phrase)."""
    return " ".join(words)

def unify_rows(rowA, rowB, canonical_phrase):
    """
    Merges data from two rows (dictionaries) into one according to specified rules:
      - COL_MAX: maximum value
      - COL_SUM: sum
      - COL_MEAN: average
      - COL_CONCAT: concatenation with " | "
    Other columns (e.g., tag_match, model_name, label, ...)
      - taken "as is" from the canonical row.

    :param rowA: dictionary of values for phrase A
    :param rowB: dictionary of values for phrase B
    :param canonical_phrase: string 'A' or 'B' or the actual canonical phrase
                             used to determine which phrase is canonical.
    :return: dictionary row (final result)
    """
    # Determine which row is canonical
    prefer_row = rowA if rowA["_phrase"] == canonical_phrase else rowB
    other_row = rowB if prefer_row is rowA else rowA

    # Copy everything from prefer_row, then update necessary fields
    row = dict(prefer_row)  # base on the canonical row

    # COL_MAX
    for c in COL_MAX:
        valA = rowA.get(c, 0.0)
        valB = rowB.get(c, 0.0)
        row[c] = max(valA, valB)

    # COL_SUM
    for c in COL_SUM:
        valA = rowA.get(c, 0.0)
        valB = rowB.get(c, 0.0)
        row[c] = valA + valB

    # COL_MEAN
    for c in COL_MEAN:
        valA = float(rowA.get(c, 0.0))
        valB = float(rowB.get(c, 0.0))
        row[c] = (valA + valB) / 2.0

    # COL_CONCAT
    for c in COL_CONCAT:
        strA = rowA.get(c, "")
        strB = rowB.get(c, "")
        if strA and strB:
            row[c] = strA + " | " + strB
        else:
            row[c] = strA or strB

    return row

def is_same_except_one_word(phraseA, phraseB, patterns):
    """
    Checks if phraseA and phraseB differ by exactly one word,
    and whether that difference matches a known replacement pattern
    (e.g., -ой <-> -ый, -а <-> '', -и <-> '', -ов <-> '').
    Returns (True, <canonical_phrase>) if they match, otherwise (False, None).
    """
    wordsA = parse_phrase_into_words(phraseA)
    wordsB = parse_phrase_into_words(phraseB)
    if len(wordsA) != len(wordsB):
        return (False, None)

    diff_count = 0
    diff_index = -1

    # Find the position where words differ
    for i in range(len(wordsA)):
        if wordsA[i] != wordsB[i]:
            diff_count += 1
            diff_index = i
            if diff_count > 1:
                return (False, None)
    if diff_count == 0:
        # Completely identical
        return (False, None)

    wA = wordsA[diff_index]
    wB = wordsB[diff_index]

    for (suffix1, suffix2, prefer) in patterns:
        if wA.endswith(suffix1) and wB.endswith(suffix2):
            rootA = wA[: -len(suffix1)]
            rootB = wB[: -len(suffix2)]
            if rootA == rootB:
                if prefer == suffix1:
                    canonical_word = rootA + suffix1
                elif prefer == suffix2:
                    canonical_word = rootA + suffix2
                else:
                    canonical_word = rootA

                if canonical_word == wA:
                    return (True, phraseA)
                elif canonical_word == wB:
                    return (True, phraseB)
                else:
                    canonical_words = wordsA[:]
                    canonical_words[diff_index] = canonical_word
                    new_phrase = join_words_into_phrase(canonical_words)
                    return (True, new_phrase)

        if wA.endswith(suffix2) and wB.endswith(suffix1):
            rootA = wA[: -len(suffix2)]
            rootB = wB[: -len(suffix1)]
            if rootA == rootB:
                if prefer == suffix1:
                    canonical_word = rootB + suffix1
                elif prefer == suffix2:
                    canonical_word = rootA + suffix2
                else:
                    canonical_word = rootA

                if canonical_word == wA:
                    return (True, phraseA)
                elif canonical_word == wB:
                    return (True, phraseB)
                else:
                    canonical_words = wordsA[:]
                    canonical_words[diff_index] = canonical_word
                    new_phrase = join_words_into_phrase(canonical_words)
                    return (True, new_phrase)

    return (False, None)


def main():
    # Load DataFrame
    df = pd.read_excel(PATH_DATA_WITH_OFF)
    print(f"[INFO] Loaded {df.shape[0]} rows.")

    # Convert DataFrame to dictionary: phrase -> row (dictionary of all columns)
    phrase2row = {}
    for i, r in df.iterrows():
        phrase = str(r.get("key", "")).strip()
        if not phrase:
            continue
        d = r.to_dict()
        d["_phrase"] = phrase
        phrase2row[phrase] = d

    # Define patterns
    patterns = [
        ("ой", "ый", "ой"),   # if "языковой" vs "языковый", prefer "ой"
        ("а",  "",    ""),    # "эмбеддинга" vs "эмбеддинг", no suffix
        ("и",  "",    ""),    # "эмбеддинги" vs "эмбеддинг"
        ("ов", "",    ""),    # "эмбеддингов" vs "эмбеддинг"
    ]

    # Merge similar phrases
    changed = True
    while changed:
        changed = False
        all_phrases = list(phrase2row.keys())

        for phraseA in all_phrases:
            if phraseA not in phrase2row:
                continue
            rowA = phrase2row[phraseA]
            for phraseB in all_phrases:
                if phraseB == phraseA:
                    continue
                if phraseB not in phrase2row:
                    continue
                rowB = phrase2row[phraseB]
                ok, canonical_phrase = is_same_except_one_word(phraseA, phraseB, patterns)
                if not ok:
                    continue
                new_row = unify_rows(rowA, rowB, canonical_phrase)

                phrase2row[canonical_phrase] = new_row
                if phraseA != canonical_phrase and phraseA in phrase2row:
                    del phrase2row[phraseA]
                if phraseB != canonical_phrase and phraseB in phrase2row:
                    del phrase2row[phraseB]

                changed = True
                break
            if changed:
                break

    # Convert back to DataFrame and save
    cleaned_rows = []
    for ph, rowdict in phrase2row.items():
        cleaned_rows.append(rowdict)

    df_cleaned = pd.DataFrame(cleaned_rows)
    if "_phrase" in df_cleaned.columns:
        df_cleaned.drop(columns=["_phrase"], inplace=True)

    print(f"[INFO] Final shape: {df_cleaned.shape}")
    df_cleaned.to_excel(PATH_OUT, index=False)
    print(f"[INFO] Saved cleaned data to {PATH_OUT}")


if __name__ == "__main__":
    main()

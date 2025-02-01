import json
import os
import pandas as pd
import numpy as np
import torch
import inspect
import pymorphy2

from transformers import AutoTokenizer, AutoModelForMaskedLM

# Патчим pymorphy2 для совместимости с Python 3.12+
if not hasattr(inspect, "getargspec"):
    def getargspec_patched(func):
        spec = inspect.getfullargspec(func)
        return spec.args, spec.varargs, spec.varkw, spec.defaults  # Совместимый формат
    inspect.getargspec = getargspec_patched


########################################
# 1) Настройки
########################################

# Пути к файлам
PATH_FILTERED = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"
PATH_BIGDATA = "/home/milkorna/Documents/AutoThematicThesaurus/data_with_oof.xlsx"
PATH_OUT_JSON = "/home/milkorna/Documents/AutoThematicThesaurus/synonyms_analysis/synonyms_mask_deep_pavlov.json"

# Параметры для поиска
MODEL_NAME = "DeepPavlov/rubert-base-cased"
TOP_N = 20  # Сколько кандидатов выдаёт модель для [MASK]
STOP_WORD = "данный"  # синонимы, содержащие это слово, будут удаляться


########################################
# 2) Инициализация токенизатора и модели
########################################

print("[INFO] Loading morphological analyzer and model...")
morph = pymorphy2.MorphAnalyzer()
tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
model = AutoModelForMaskedLM.from_pretrained(MODEL_NAME)
model.eval()  # переводим в режим inference
print("[INFO] Model loaded successfully.")

########################################
# 3) Функции вспомогательные
########################################

def get_pos_tag(word):
    """
    Возвращает часть речи (POS) слова через pymorphy2, например NOUN, ADJF и т.п.
    """
    parsed_word = morph.parse(word)[0]
    return parsed_word.tag.POS

def lemmatize_word(word):
    """
    Лемматизация слова через pymorphy2.
    """
    parsed_word = morph.parse(word)[0]
    return parsed_word.normal_form

def find_synonyms_in_set(phrase, phrases_set, top_n=TOP_N):
    """
    Аналог вашей старой логики:
    - Для каждого слова в phrase подставляем [MASK], модель выдаёт top_n кандидатов
    - Фильтруем по части речи
    - Составляем новые варианты фразы
    - Оставляем только те варианты, которые есть в phrases_set (точное совпадение).
    Возвращаем список (строки), которые потенциально считаем «синонимами» из набора.
    """
    words = phrase.split()  # Разбиваем фразу на слова
    generated_phrases = set()  # Храним все возможные варианты

    # POS для каждого слова
    pos_tags = [get_pos_tag(w) for w in words]

    for i, (original_word, original_pos) in enumerate(zip(words, pos_tags)):
        # Маскируем i-й токен
        masked_sentence = words.copy()
        masked_sentence[i] = "[MASK]"
        # Придумываем фейковый контекст модели (можно менять)
        input_text = f"{' '.join(masked_sentence)} может быть заменено на [MASK]."

        # Токенизация
        input_ids = tokenizer.encode(input_text, return_tensors="pt")
        mask_token_index = torch.where(input_ids == tokenizer.mask_token_id)[1]
        # Предсказание
        with torch.no_grad():
            output = model(input_ids)
            logits = output.logits

        mask_token_logits = logits[0, mask_token_index, :]
        top_tokens = torch.topk(mask_token_logits, top_n, dim=1).indices[0].tolist()

        # Получаем слова-кандидаты
        synonyms_candidates = []
        for token in top_tokens:
            cand_word = tokenizer.decode([token]).strip().lower()
            # Лемматизируем, чтобы убрать формы
            cand_word_lemma = lemmatize_word(cand_word)
            synonyms_candidates.append(cand_word_lemma)

        # Собираем фразы с заменой
        for syn in synonyms_candidates:
            # Проверяем, совпадает ли POS
            syn_pos = get_pos_tag(syn)
            if syn_pos == original_pos:
                new_phrase_words = words.copy()
                new_phrase_words[i] = syn
                new_phrase = " ".join(new_phrase_words)
                if new_phrase != phrase:
                    generated_phrases.add(new_phrase)

    # Сопоставляем с реальными фразами из big-data
    matched_phrases = [p for p in generated_phrases if p in phrases_set]

    # Лог, если не нашли ни одного совпадения
    if not matched_phrases:
        with open("script_logs.txt", "a", encoding="utf-8") as log_file:
            log_file.write(f"No matches in set for '{phrase}'. Generated: {generated_phrases}\n")

    return matched_phrases

def ensure_mutual_synonyms_and_cleanup(results, stop_word=STOP_WORD):
    """
    - Удаляем из synonyms элементы, где "key" содержит stop_word (e.g. "данный").
    - Для каждой пары (A -> B), если B -> A нет, добавляем.
    - Если в итоге у A synonyms пуст => удалим A из results (или пусть остаётся).
    """
    # Превратим список ключей (чтобы итерироваться по копии)
    all_phrases = list(results.keys())

    for phrase in all_phrases:
        item = results[phrase]
        synonyms = item.get("synonyms", [])

        # 1) Удаляем синонимы содержащие stop_word
        synonyms = [syn for syn in synonyms if stop_word not in syn["key"]]
        item["synonyms"] = synonyms

    # 2) Взаимное дополнение:
    #    Если A содержит B, то B тоже должен содержать A
    for phrase in all_phrases:
        if phrase not in results:
            continue
        item = results[phrase]
        synonyms = item.get("synonyms", [])

        for syn_obj in synonyms:
            syn_phrase = syn_obj["key"]
            if syn_phrase in results:
                # проверим, есть ли phrase в synonyms того syn_phrase
                syn_list_of_synonyms = results[syn_phrase].get("synonyms", [])
                # Ищем, есть ли уже "phrase" там
                already_present = any(sub["key"] == phrase for sub in syn_list_of_synonyms)
                if not already_present:
                    # Добавляем
                    syn_list_of_synonyms.append({
                        "key": phrase,
                        "is_term_manual": item["is_term_manual"],
                        "oof_prob_class": item["oof_prob_class"]
                    })
                    results[syn_phrase]["synonyms"] = syn_list_of_synonyms

    # 3) Удаляем из словаря те фразы, у которых synonyms оказался пуст
    #    (т.к. пользователь попросил «не перечислять те элементы, для которых не найдены синонимы»)
    to_remove = []
    for phrase in results:
        if not results[phrase]["synonyms"]:
            to_remove.append(phrase)

    for phrase in to_remove:
        del results[phrase]

    return results

########################################
# 4) Основной скрипт
########################################

def main():
    print("[INFO] Reading filtered data from:", PATH_FILTERED)
    df_filtered = pd.read_excel(PATH_FILTERED)

    print("[INFO] Reading big data from:", PATH_BIGDATA)
    df_big = pd.read_excel(PATH_BIGDATA)

    # Убедимся, что в df_big есть нужные колонки: 'key', 'is_term_manual', 'oof_prob_class'
    required_cols = {'key', 'is_term_manual', 'oof_prob_class'}
    if not required_cols.issubset(df_big.columns):
        print(f"[ERROR] df_big missing required columns: {required_cols - set(df_big.columns)}")
        return

    # Превратим df_big в словарь { phrase -> (is_term_manual, oof_prob_class) }
    # и также в set для быстрого поиска
    big_dict = {}
    for idx, row in df_big.iterrows():
        phrase_key = str(row['key']).strip()
        is_term = int(row['is_term_manual'])  # assuming 0/1
        prob = float(row['oof_prob_class'])
        big_dict[phrase_key] = (is_term, prob)
    phrases_set = set(big_dict.keys())
    results = {}

    # Для каждой строки df_filtered (где key, is_term_manual, etc.)
    # ищем синонимы
    for idx, row in df_filtered.iterrows():
        phrase_key = str(row['key']).strip()
        if phrase_key not in big_dict:
            # Если phrase_key нет в big_dict, значит нет info is_term_manual / oof_prob_class
            # Можно пропустить (тогда не найдём синонимы) или задать 0/0.0
            # Пока пропустим
            continue
        is_term, prob = big_dict[phrase_key]

        synonyms_found = find_synonyms_in_set(phrase_key, phrases_set, top_n=TOP_N)
        if not synonyms_found:
            # Если вообще нет кандидатов, мы пока всё равно включим item в results
            # но после ensure_mutual_synonyms_and_cleanup он может быть удалён
            results[phrase_key] = {
                "key": phrase_key,
                "is_term_manual": is_term,
                "oof_prob_class": prob,
                "synonyms": []
            }
        else:
            # Создаём список dict'ов
            syn_list = []
            for syn_phrase in synonyms_found:
                syn_is_term, syn_prob = big_dict[syn_phrase]
                syn_list.append({
                    "key": syn_phrase,
                    "is_term_manual": syn_is_term,
                    "oof_prob_class": syn_prob
                })

            results[phrase_key] = {
                "key": phrase_key,
                "is_term_manual": is_term,
                "oof_prob_class": prob,
                "synonyms": syn_list
            }

    # Постобработка: убрать stop_word, взаимно дополнить, убрать пустые
    results = ensure_mutual_synonyms_and_cleanup(results, stop_word=STOP_WORD)

    # Сохраняем финальный JSON
    if not results:
        print("[INFO] No items with synonyms found. The result is empty.")
    else:
        print(f"[INFO] Found {len(results)} phrases with synonyms after cleanup.")

    print(f"[INFO] Saving synonyms to {PATH_OUT_JSON}")
    with open(PATH_OUT_JSON, "w", encoding="utf-8") as f:
        json.dump(results, f, ensure_ascii=False, indent=4)

    print("[INFO] Done.")


if __name__ == "__main__":
    main()

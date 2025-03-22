import os
import re
from scripts.core.paths import CORPUS_DIR

input_folder = CORPUS_DIR / 'texts'
output_file = CORPUS_DIR / 'combined_corpus'

def process_text_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as infile:
        text = infile.read()
        sentences = re.split(r'(?<=[.!?;:])\s+', text)
        processed_sentences = [sentence for sentence in sentences if len(sentence) > 25]
    return processed_sentences

def process_title_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as infile:
        title = infile.readline().strip()
        if not title.endswith('.'):
            title += '.'
        if len(title) > 25:
            return title
    return None

with open(output_file, 'w', encoding='utf-8') as outfile:
    for filename in os.listdir(input_folder):
        if re.match(r'art\d+_text\.txt', filename):
            file_path = os.path.join(input_folder, filename)
            processed_sentences = process_text_file(file_path)
            for sentence in processed_sentences:
                outfile.write(sentence + '\n')
        elif re.match(r'art\d+_title\.txt', filename):
            file_path = os.path.join(input_folder, filename)
            processed_title = process_title_file(file_path)
            if processed_title:
                outfile.write(processed_title + '\n')

import os
import re
import chardet
from scripts.core.paths import CORPUS_DIR

input_folder = CORPUS_DIR / 'texts'

def process_links(line):
    # Define a pattern to match URLs
    link_pattern = r'https?://[^\s/$.?#].[^\s]*'
    if re.match(link_pattern, line.strip()):
        # If the entire line is just a URL with punctuation, return None to remove it
        if re.fullmatch(f'{link_pattern}[.,!?;:]*', line.strip()):
            return None
        else:
            # If the line contains text and a URL, replace the URL with 'link' and preserve trailing punctuation
            return re.sub(f'({link_pattern})([.,!?;:]*)', r'link\2', line)
    # If a URL is at the end of the line, replace it with 'link'
    line = re.sub(f'({link_pattern})([.,!?;:]*)$', r'link\2', line)
    return line

def clean_punctuation(text):
    # Replace unwanted punctuation patterns with a single dot or appropriate punctuation
    text = re.sub(r'\.{2,}', '.', text)  # Replace sequences of dots with a single dot
    text = re.sub(r',+', ',', text)      # Replace sequences of commas with a single comma
    text = re.sub(r';+', ';', text)      # Replace sequences of semicolons with a single semicolon
    text = re.sub(r':+', ':', text)      # Replace sequences of colons with a single colon
    text = re.sub(r' ,', ',', text)      # Replace " ," with ","
    text = re.sub(r' ([.!?,;:])', r'\1', text)  # Remove space before punctuation
    text = re.sub(r'([!?;:])\.', r'\1', text)   # Replace "!.", ":.", "?.", ";." with "!", ":", "?", ";"
    text = re.sub(r',\.', '.', text)            # Replace ",." with "."
    return text

def process_text(text):
    lines = text.split('\n')
    processed_lines = []

    for line in lines:
        # Process each line to handle URLs
        line = process_links(line)
        line = clean_punctuation(line)
        if line is None:
            # Skip the line if it should be removed
            continue

        stripped_line = line.strip()
        if stripped_line:
            # If the previous line is not empty and doesn't end with a punctuation mark, add a dot
            if stripped_line and not stripped_line.endswith(('.', '!', '?', ':', ';')):
                line = stripped_line + '.'
        processed_lines.append(line.strip())

    non_empty_lines = [line for line in processed_lines if line]
    processed_text = '\n'.join(non_empty_lines)
    return processed_text

def read_file(filepath):
    with open(filepath, 'rb') as file:
        raw_data = file.read()
        # Detect the file encoding
        result = chardet.detect(raw_data)
        encoding = result['encoding']

    # Use 'utf-8' if encoding cannot be determined
    if not encoding:
        encoding = 'utf-8'

    # Convert the file to 'utf-8' if it's not already
    if encoding.lower() != 'utf-8':
        raw_data = raw_data.decode(encoding).encode('utf-8')
        with open(filepath, 'wb') as file:
            file.write(raw_data)
        return raw_data.decode('utf-8')
    else:
        with open(filepath, 'r', encoding='utf-8') as file:
            return file.read()

def process_files_in_directory(directory):
    for filename in os.listdir(directory):
        # Match filenames that start with 'art' followed by digits and '_text.txt'
        if re.match(r'art\d+_text\.txt', filename):
            filepath = os.path.join(directory, filename)
            # Read the file
            text = read_file(filepath)
            # Process the text
            processed_text = process_text(text)
            # Write the processed text back to the file
            with open(filepath, 'w', encoding='utf-8') as file:
                file.write(processed_text)

process_files_in_directory(input_folder)

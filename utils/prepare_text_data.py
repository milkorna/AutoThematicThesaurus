import os
import re
import chardet

def process_links(line):
    # Çàìåíÿåì ññûëêè íà "link" åñëè â ñòðîêå åñòü òåêñò, èíà÷å óäàëÿåì ñòðîêó
    link_pattern = r'https?://[^\s/$.?#].[^\s]*'
    if re.match(link_pattern, line.strip()):
        # Åñëè ñòðîêà ñîñòîèò òîëüêî èç ññûëêè è çíàêîâ ïðåïèíàíèÿ, óäàëÿåì å¸
        if re.fullmatch(f'{link_pattern}[.,!?;:]*', line.strip()):
            return None
        else:
            # Åñëè â ñòðîêå åñòü òåêñò, çàìåíÿåì ññûëêó íà "link"
            return re.sub(link_pattern, 'link', line)
    return line

def process_text(text):
    text = re.sub(r'\.{2,}', '.', text)
    text = re.sub(r',\.', '.', text)
    text = re.sub(r' \.', '.', text)
    text = re.sub(r' \,', ',', text)

    lines = text.split('\n')
    processed_lines = []

    for line in lines:

        line = process_links(line)
        if line is None:
            continue

        stripped_line = line.strip()
        if stripped_line:  # åñëè ñòðîêà íå ïóñòàÿ
            if processed_lines and not stripped_line.endswith(('.', '!', '?', ':', ';')):
                # äîáàâëÿåì òî÷êó, åñëè ïðåäûäóùàÿ ñòðîêà áûëà òåêñòîì è òåêóùàÿ ñòðîêà íå çàêàí÷èâàåòñÿ çíàêîì ïðåïèíàíèÿ
                processed_lines[-1] += '.'
            processed_lines.append(line)

    # Óäàëÿåì âñå ïóñòûå ñòðîêè ìåæäó ñòðîêàìè ñ òåêñòîì
    non_empty_lines = []
    for i in range(len(processed_lines)):
        if processed_lines[i].strip() or (i > 0 and processed_lines[i-1].strip()):
            non_empty_lines.append(processed_lines[i])

    # Óäàëÿåì âñå îñòàâøèåñÿ ïóñòûå ñòðîêè â êîíöå òåêñòà
    while non_empty_lines and not non_empty_lines[-1].strip():
        non_empty_lines.pop()

    processed_text = '\n'.join(non_empty_lines)
    return processed_text

def read_file(filepath):
    with open(filepath, 'rb') as file:
        raw_data = file.read()
        result = chardet.detect(raw_data)
        encoding = result['encoding']
    
    if not encoding:
        encoding = 'utf-8'
    
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
        if re.match(r'art\d+_text\.txt', filename):
            filepath = os.path.join(directory, filename)
            text = read_file(filepath)
            
            processed_text = process_text(text)
            
            with open(filepath, 'w', encoding='utf-8') as file:
                file.write(processed_text)

directory = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/texts'
process_files_in_directory(directory)

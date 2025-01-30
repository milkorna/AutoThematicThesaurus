def process_file(input_file, output_file):
    unique_lines = set()

    with open(input_file, 'r', encoding='utf-8') as infile:
        for line in infile:
            stripped_line = line.strip().replace(" ", "")
            if stripped_line:
                unique_lines.add(stripped_line)

    sorted_lines = sorted(unique_lines)

    with open(output_file, 'w', encoding='utf-8') as outfile:
        for line in sorted_lines:
            outfile.write(line + '\n')

input_file = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/stop_words_unordered.txt'
output_file = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/stop_words.txt'
process_file(input_file, output_file)

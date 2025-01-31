import pandas as pd
from collections import Counter

# Load dataset
data_file = "/home/milkorna/Documents/AutoThematicThesaurus/data.xlsx"
data = pd.read_excel(data_file)

# Split phrases into words
data['words'] = data['key'].apply(lambda x: set(x.split()))

# Count the occurrences of each word across all phrases
all_words = Counter(word for words in data['words'] for word in words)

# Calculate inverse rarity for each phrase
def calculate_inverse_rarity(phrase_words):
    return sum(1 / all_words[word] for word in phrase_words)

data['inverse_rarity'] = data['words'].apply(calculate_inverse_rarity)

# Remove the temporary 'words' column
data = data.drop(columns=['words'])

# Move the new column to the desired position
columns = list(data.columns)
columns.insert(columns.index('tag_match'), 'inverse_rarity')
data = data[columns]

# Save the updated dataset
data.to_excel("/home/milkorna/Documents/AutoThematicThesaurus/updated_dataset.xlsx", index=False)
print("Inverse rarity coefficient has been added and saved in 'updated_dataset.xlsx'.")

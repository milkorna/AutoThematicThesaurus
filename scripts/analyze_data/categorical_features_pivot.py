import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from core.paths import PATH_DATA

"""
Script Purpose:
This script performs an exploratory data analysis (EDA) on the given dataset to investigate the distribution of terms and non-terms across two key features: 'label' and 'model_name'. The script:

1. Creates pivot tables to summarize the counts of terms ('is_term_manual' = 1) and non-terms ('is_term_manual' = 0).
2. Saves these pivot tables for further analysis.
3. Generates stacked bar charts to visualize the proportion of terms and non-terms for each 'label' and 'model_name'.
"""

# Load the dataset
df = pd.read_excel(PATH_DATA)

# Create a pivot table summarizing 'label'
label_pivot = df.pivot_table(
    index='label',
    columns='is_term_manual',
    values='context',
    aggfunc='count',
    fill_value=0
)
label_pivot.columns = ['Not Term', 'Term']
label_pivot['Total'] = label_pivot['Not Term'] + label_pivot['Term']
label_pivot = label_pivot.sort_values(by='Total', ascending=False)

# Save the pivot table for 'label'
label_pivot.to_csv('label_pivot.csv')

# Visualize the distribution of terms and non-terms for 'label'
label_pivot_normalized = label_pivot[['Not Term', 'Term']].div(label_pivot['Total'], axis=0)
label_pivot_normalized.plot(kind='bar', stacked=True, figsize=(12, 6), color=['skyblue', 'lightgreen'])
plt.title('Distribution of Terms and Non-Terms by Label', fontsize=16)
plt.xlabel('Label', fontsize=12)
plt.ylabel('Proportion', fontsize=12)
plt.xticks(rotation=45, ha='right')
plt.tight_layout()
plt.savefig('label_distribution.png')
plt.close()

# Create a pivot table summarizing 'model_name'
model_name_pivot = df.pivot_table(
    index='model_name',
    columns='is_term_manual',
    values='context',
    aggfunc='count',
    fill_value=0
)
model_name_pivot.columns = ['Not Term', 'Term']
model_name_pivot['Total'] = model_name_pivot['Not Term'] + model_name_pivot['Term']
model_name_pivot = model_name_pivot.sort_values(by='Total', ascending=False)

# Save the pivot table for 'model_name'
model_name_pivot.to_excel('model_name_pivot.xlsx')

# Visualize the distribution of terms and non-terms for 'model_name'
model_name_pivot_normalized = model_name_pivot[['Not Term', 'Term']].div(model_name_pivot['Total'], axis=0)
model_name_pivot_normalized.plot(kind='bar', stacked=True, figsize=(12, 6), color=['skyblue', 'lightgreen'])
plt.title('Distribution of Terms and Non-Terms by Model Name', fontsize=16)
plt.xlabel('Model Name', fontsize=12)
plt.ylabel('Proportion', fontsize=12)
plt.xticks(rotation=45, ha='right')
plt.tight_layout()
plt.savefig('model_name_distribution.png')
plt.close()

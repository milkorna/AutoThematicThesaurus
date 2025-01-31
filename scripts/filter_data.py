import pandas as pd

# Load dataset
file_path = "/home/milkorna/Documents/AutoThematicThesaurus/data_with_oof.xlsx"
df = pd.read_excel(file_path)

# Define excluded values for model_name and label
excluded_model_names = ["(Прич + С) + СоюзИ + (Прич + С)",
                        "(Прич + С) + Предл + (Прич + С)",
                        "(Прич + С) + СоюзИ + (Прил + С)",
                        "Прич и (Прич + С)",
                        "(Прич + С) + (Прич + С)рд",
                        "Прич и (Прил + С)",
                        "(Прил + С) + Предл + (Прич + С)",
                        "(Прич + С) + СоюзИ + С"]
excluded_labels = ["everyday expression", "colloquial phrase"]

# Filter dataset
filtered_df = df[
    (
        ((df['is_term_manual'] == 1) & (df['oof_prob_class'] >= 0.05)) |
        ((df['is_term_manual'] == 0) & (df['oof_prob_class'] > 0.34))
    ) &
    (~df['model_name'].isin(excluded_model_names)) &
    (~df['label'].isin(excluded_labels))
]

# Save filtered dataset
output_file = "filtered_data.xlsx"
filtered_df.to_excel(output_file, index=False)

print(f"Filtering completed. Data saved to {output_file}")
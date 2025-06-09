import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys

# --- DADOS BRUTOS DOS TESTES ---
# ATENÇÃO: Preencha esta estrutura de dicionário com os SEUS DADOS BRUTOS de Oleom!
# As chaves 'trace_firstfit', 'trace_bestfit', etc., referem-se ao TIPO DE TRACE USADO.
# Os valores associados a cada trace são dicionários com os resultados brutos (lista de 5 inteiros)
# para cada ALGORITMO TESTADO.
#
# Exemplo de como preencher (valores fictícios, SUBSTITUA pelos seus):
# DADOS_BRUTOS_DOS_TESTES = {
#     "trace_firstfit": {
#         "First-Fit": [10, 12, 11, 9, 13],  # 5 resultados de Oleom para FF com trace First-Fit
#         "Next-Fit": [15, 14, 16, 15, 17], # 5 resultados de Oleom para NF com trace First-Fit
#         "Best-Fit": [20, 19, 21, 20, 18],
#         "Worst-Fit": [25, 24, 26, 25, 23],
#     },
#     "trace_bestfit": {
#         "First-Fit": [22, 20, 23, 21, 24],
#         "Next-Fit": [28, 27, 29, 28, 30],
#         "Best-Fit": [8, 9, 7, 8, 10], # Best-Fit indo bem com seu próprio trace
#         "Worst-Fit": [35, 33, 36, 34, 37],
#     },
#     # ... preencha para os outros traces ...
# }

# EXEMPO ILUSTRATIVO (SUBSTITUA PELOS SEUS DADOS REAIS E COMPLETOS!)
DADOS_BRUTOS_DOS_TESTES = {
    "trace_firstfit": {
        "First-Fit": [106, 103, 86, 100, 94],
        "Next-Fit":  [106, 105, 86,  98, 95],
        "Best-Fit":  [ 91,  86, 74,  82, 75],
        "Worst-Fit": [ 89,  91, 76,  83, 77],
    },
    "trace_bestfit": {
        "First-Fit": [369, 374, 359, 368, 353],
        "Next-Fit":  [373, 381, 362, 370, 354],
        "Best-Fit":  [321, 339, 314, 321, 315],
        "Worst-Fit": [341, 348, 327, 336, 327],
    },
    "trace_worstfit": {
        "First-Fit": [0, 0, 0, 0, 0],
        "Next-Fit":  [0, 0, 0, 0, 0],
        "Best-Fit":  [0, 0, 0, 0, 0],
        "Worst-Fit": [0, 0, 0, 0, 0],
    },
    "trace_nextfit": {
        "First-Fit": [93, 91, 108, 71, 93],
        "Next-Fit": [105, 93, 109, 73, 93],
        "Best-Fit": [91, 75, 94, 65, 78],
        "Worst-Fit": [94, 73, 96, 65, 84],
    },
}

# Nomes mais amigáveis para os traces e algoritmos nos gráficos
TRACE_LABELS = {
    "trace_firstfit": "Trace Gerado para First-Fit",
    "trace_bestfit": "Trace Gerado para Best-Fit",
    "trace_worstfit": "Trace Gerado para Worst-Fit",
    "trace_nextfit": "Trace Gerado para Next-Fit",
}

ALGORITHM_LABELS = {
    "First-Fit": "First-Fit",
    "Next-Fit": "Next-Fit",
    "Best-Fit": "Best-Fit",
    "Worst-Fit": "Worst-Fit",
}

def process_raw_data(raw_data):
    """
    Processa os dados brutos para calcular médias e desvios padrão.
    Retorna uma nova estrutura de dicionário com (media, desvio_padrao).
    """
    processed_data = {}
    for trace_type, algorithms_data in raw_data.items():
        processed_data[trace_type] = {}
        for algo, oleom_list in algorithms_data.items():
            mean_oleom = np.mean(oleom_list)
            std_dev_oleom = np.std(oleom_list) # np.std calcula desvio padrão populacional por padrão
            processed_data[trace_type][algo] = (mean_oleom, std_dev_oleom)
    return processed_data

def generate_detailed_tables(raw_data, processed_data):
    """Gera e imprime as tabelas detalhadas com dados brutos, média e DP."""
    print("### Tabelas Detalhadas de Alocacoes Falhas (Oleom) ###\n")

    for trace_key, trace_label in TRACE_LABELS.items():
        print(f"--- Resultados para: {trace_label} ---\n")
        
        table_data = []
        for algo in ALGORITHM_LABELS.keys():
            raw_results = raw_data[trace_key][algo]
            mean, std = processed_data[trace_key][algo]
            
            row = [ALGORITHM_LABELS[algo]] + raw_results + [mean, std]
            table_data.append(row)

        df_detailed = pd.DataFrame(table_data, columns=[
            "Algoritmo Testado", "Rodada 1", "Rodada 2", "Rodada 3", 
            "Rodada 4", "Rodada 5", "MEDIA", "DESVIO PADRAO"
        ])
        
        print(df_detailed.to_markdown(floatfmt=".2f"))
        print("\n")

def generate_summary_table(processed_data):
    """Gera e imprime a tabela resumo de médias de Oleom."""
    print("### Tabela Resumo: Media de Alocacoes Falhas (Oleom) ###")
    
    summary_data = {
        algo: {trace_label: processed_data[trace_key][algo][0]
               for trace_key, trace_label in TRACE_LABELS.items()}
        for algo in ALGORITHM_LABELS.keys()
    }
    df_summary = pd.DataFrame(summary_data).T 
    
    # Renomear colunas para os rótulos amigáveis do trace
    df_summary.columns = [
        TRACE_LABELS[col.replace('Trace Gerado para ', 'trace_').replace('-', '').replace(' ', '_').lower()] 
        for col in df_summary.columns
    ]
    
    print(df_summary.to_markdown(floatfmt=".2f"))
    print("\n")


def generate_bar_charts(processed_data):
    """Gera os gráficos de barras agrupadas."""
    print("Gerando gráficos de barras...")

    # Configurações de estilo para Matplotlib
    plt.style.use('seaborn-v0_8-darkgrid') 
    plt.rcParams.update({'font.size': 10})
    plt.rcParams['figure.dpi'] = 100 

    for trace_key, trace_label in TRACE_LABELS.items():
        data_for_chart = processed_data[trace_key]
        
        algorithms = list(data_for_chart.keys())
        means = [data_for_chart[algo][0] for algo in algorithms]
        stds = [data_for_chart[algo][1] for algo in algorithms]

        fig, ax = plt.subplots(figsize=(8, 6)) 
        
        bars = ax.bar(algorithms, means, yerr=stds, capsize=5, 
                      color=['skyblue', 'lightcoral', 'lightgreen', 'gold']) 

        ax.set_title(f"Media de Alocacoes Falhas por Algoritmo\ncom {trace_label}", fontsize=14, pad=20)
        ax.set_xlabel("Algoritmo Testado", fontsize=12)
        ax.set_ylabel("Media de Alocacoes Falhas (Oleom)", fontsize=12)
        
        # Ajustar limites do Y para melhor visualização
        min_y = min(means) - max(stds) if means else 0
        if min_y < 0: min_y = 0 
        ax.set_ylim(bottom=min_y * 0.9, top=max(means) * 1.2)

        # Adicionar valores nas barras
        for bar in bars:
            yval = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2, yval + (ax.get_ylim()[1] * 0.03), # Posição relativa ao limite Y
                    f'{yval:.2f}', # Formata para 2 casas decimais
                    ha='center', va='bottom', fontsize=9)

        filename = f"grafico_{trace_key.replace('trace_', '')}.png" # Nome do arquivo mais limpo
        plt.tight_layout() 
        plt.savefig(filename, dpi=300) 
        print(f"Gráfico '{filename}' gerado com sucesso!")
        plt.close(fig) 

if __name__ == "__main__":
    # Verifica se os dados brutos foram preenchidos
    if not DADOS_BRUTOS_DOS_TESTES:
        print("ERRO: A estrutura 'DADOS_BRUTOS_DOS_TESTES' esta vazia. Por favor, preencha-a com seus resultados.")
        sys.exit(1)
    
    # Verifica se todos os traces e algoritmos têm 5 resultados
    for trace_type, alg_data in DADOS_BRUTOS_DOS_TESTES.items():
        for algo, results_list in alg_data.items():
            if not isinstance(results_list, list) or len(results_list) != 5:
                print(f"ERRO: Os dados brutos para '{algo}' no trace '{trace_type}' devem ser uma lista com EXATAMENTE 5 resultados.")
                print(f"Encontrado: {results_list}")
                sys.exit(1)

    # Processa os dados brutos para obter médias e DPs
    PROCESSED_DATA = process_raw_data(DADOS_BRUTOS_DOS_TESTES)

    generate_detailed_tables(DADOS_BRUTOS_DOS_TESTES, PROCESSED_DATA)
    generate_summary_table(PROCESSED_DATA)
    generate_bar_charts(PROCESSED_DATA)
    print("\nProcesso concluido. Verifique os arquivos PNG gerados e a saida do console para as tabelas.")

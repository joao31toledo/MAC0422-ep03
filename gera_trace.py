import sys
import random

# Constantes para a simulação de memória (baseadas nas do seu EP)
TOTAL_PIXELS = 65536
MAX_ALLOC_SIZE = 256 # Do PDF: 1 <= m <= 256
MIN_ALLOC_SIZE = 1

# Número de operações (linhas) no arquivo de trace gerado. Ajuste conforme necessário.
NUM_OPERATIONS = 700

def generate_first_fit_trace(filename):
    """
    Gera um arquivo de trace que favorece o algoritmo First-Fit.
    O First-Fit se destaca pela velocidade, pois aloca no primeiro bloco que serve.
    Um cenário ideal para ele tem muitos espaços livres úteis no início da memória.
    NÃO INCLUI COMANDOS COMPACTAR.
    """
    print(f"Gerando trace para First-Fit: {filename} (SEM COMPACTAR)")
    with open(filename, 'w') as f:
        line_num = 1
        for _ in range(NUM_OPERATIONS):
            # Gera alocações de tamanho pequeno a médio.
            # Isso simula requisições que o First-Fit encontraria rapidamente no início da memória,
            # assumindo que o PGM inicial tem espaço lá.
            size = random.randint(10, MAX_ALLOC_SIZE // 2) # Tamanhos mais comuns
            f.write(f"{line_num} {size}\n")
            line_num += 1

def generate_best_fit_trace(filename):
    """
    Gera um arquivo de trace que favorece o algoritmo Best-Fit.
    O Best-Fit busca o menor bloco livre que serve, minimizando a fragmentação interna.
    Um cenário ideal para ele envolve a presença de buracos de tamanhos variados.
    NÃO INCLUI COMANDOS COMPACTAR.
    """
    print(f"Gerando trace para Best-Fit: {filename} (SEM COMPACTAR)")
    with open(filename, 'w') as f:
        line_num = 1
        
        # Fase 1: Cria uma sequência de alocações que podem gerar buracos de tamanhos diversos.
        # (Isso complementaria um PGM inicial que também tenha fragmentação).
        f.write(f"{line_num} {MAX_ALLOC_SIZE}\n") # Bloco grande
        line_num += 1
        f.write(f"{line_num} {MAX_ALLOC_SIZE // 4}\n") # Bloco pequeno
        line_num += 1
        f.write(f"{line_num} {MAX_ALLOC_SIZE // 2}\n") # Bloco médio
        line_num += 1
        f.write(f"{line_num} {MAX_ALLOC_SIZE - 50}\n") # Outro grande, mas com sobra diferente
        line_num += 1
        
        # Fase 2: Gera requisições que forçariam o Best-Fit a "pensar" e escolher o ajuste mais justo.
        # Geramos tamanhos variados que podem se encaixar em diferentes buracos,
        # incentivando a escolha do "melhor" (o que deixa a menor sobra).
        for _ in range(NUM_OPERATIONS - 4): # Operações restantes
            size = random.randint(MIN_ALLOC_SIZE, MAX_ALLOC_SIZE)
            f.write(f"{line_num} {size}\n")
            line_num += 1

def generate_worst_fit_trace(filename):
    """
    Gera um arquivo de trace que favorece o algoritmo Worst-Fit.
    O Worst-Fit busca o maior bloco livre disponível para alocar,
    com a intenção de deixar um grande remanescente.
    NÃO INCLUI COMANDOS COMPACTAR.
    """
    print(f"Gerando trace para Worst-Fit: {filename} (SEM COMPACTAR)")
    with open(filename, 'w') as f:
        line_num = 1
        
        # Fase 1: Cria algumas alocações para garantir que haja buracos grandes e variados.
        # (Assumindo que o PGM inicial é majoritariamente vazio ou com grandes blocos livres).
        f.write(f"{line_num} {MAX_ALLOC_SIZE // 2}\n") # Bloco médio
        line_num += 1
        f.write(f"{line_num} {MAX_ALLOC_SIZE // 4}\n") # Bloco pequeno
        line_num += 1
        f.write(f"{line_num} {MAX_ALLOC_SIZE - 100}\n") # Bloco quase grande
        line_num += 1
        f.write(f"{line_num} {MAX_ALLOC_SIZE // 8}\n") # Bloco muito pequeno
        line_num += 1

        # Fase 2: Gera alocações pequenas.
        # O Worst-Fit deveria pegar os maiores buracos para essas alocações pequenas,
        # deixando grandes remanescentes. Isso é o que ele tenta otimizar.
        for _ in range(NUM_OPERATIONS - 5):
            size = random.randint(MIN_ALLOC_SIZE, MAX_ALLOC_SIZE // 4) # Principalmente alocações pequenas
            f.write(f"{line_num} {size}\n")
            line_num += 1

def generate_next_fit_trace(filename):
    """
    Gera um arquivo de trace que favorece o algoritmo Next-Fit.
    O Next-Fit se destaca por distribuir as alocações e por sua busca circular.
    Um cenário ideal mostra essa distribuição e o "dar a volta" na memória.
    NÃO INCLUI COMANDOS COMPACTAR.
    """
    print(f"Gerando trace para Next-Fit: {filename} (SEM COMPACTAR)")
    with open(filename, 'w') as f:
        line_num = 1
        
        # Fase 1: Preenche uma parte significativa da memória sequencialmente.
        # Isso força o 'inicio_next_fit' a se mover para o meio/fim da memória.
        for _ in range(NUM_OPERATIONS // 2):
            size = random.randint(10, MAX_ALLOC_SIZE // 2)
            f.write(f"{line_num} {size}\n")
            line_num += 1
        
        # Fase 2: Continua alocando.
        # As alocações aqui podem fazer o Next-Fit usar espaços no final,
        # e depois, se a memória ficar cheia nessa região, forçá-lo a circular
        # e encontrar espaços no início (que estariam livres após a compactação).
        for _ in range(NUM_OPERATIONS // 2):
            size = random.randint(MIN_ALLOC_SIZE, MAX_ALLOC_SIZE // 4) # Alocações menores
            f.write(f"{line_num} {size}\n")
            line_num += 1

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Uso: python3 generate_trace.py <tipo_algoritmo>")
        print("  <tipo_algoritmo>: 1=First-Fit, 2=Best-Fit, 3=Worst-Fit, 4=Next-Fit")
        sys.exit(1)

    try:
        algorithm_type = int(sys.argv[1])
        if not (1 <= algorithm_type <= 4):
            raise ValueError
    except ValueError:
        print("Erro: <tipo_algoritmo> deve ser um numero inteiro entre 1 e 4.")
        sys.exit(1)

    trace_generators = {
        1: ("trace-firstfit5.txt", generate_first_fit_trace),
        2: ("trace-bestfit.txt", generate_best_fit_trace),
        3: ("trace-worstfit.txt", generate_worst_fit_trace),
        4: ("trace-nextfit.txt", generate_next_fit_trace),
    }

    filename, generator_func = trace_generators[algorithm_type]
    generator_func(filename)
    print(f"Arquivo de trace '{filename}' gerado com sucesso!")

import sys
import random

def gerar_pgm_entrada(nome_arquivo="entrada.pgm", largura=256, altura=256, max_val=255, pixels_por_linha_arquivo=16, tipo_entrada=1):
    """
    Gera um arquivo PGM (Portable Graymap) para ser usado como entrada para o simulador.
    
    Args:
        nome_arquivo (str): Nome do arquivo PGM a ser gerado.
        largura (int): Largura da imagem em pixels.
        altura (int): Altura da imagem em pixels.
        max_val (int): Valor máximo de intensidade do pixel (255 para preto/branco).
        pixels_por_linha_arquivo (int): Quantidade de pixels por linha no arquivo de texto PGM.
        tipo_entrada (int):
            1: Entrada aleatória com blocos contíguos de 0s e 255s (fragmentada de forma variada).
            2: Entrada totalmente em branco (todos 255s - ideal para First/Next Fit iniciais).
    """
    total_pixels = largura * altura
    
    with open(nome_arquivo, "w") as f:
        # Escreve o cabeçalho fixo (P2, 256 256, 255)
        f.write("P2\n")
        f.write(f"{largura} {altura}\n")
        f.write(f"{max_val}\n")
        
        pixels_gerados = 0 # Contador de pixels já escritos
        
        if tipo_entrada == 2:
            # Tipo 2: Totalmente em branco (todos 255s)
            for i in range(total_pixels):
                pixel_value = max_val
                # Formatação da escrita: 3 caracteres, espaço entre pixels, quebra de linha
                if i % pixels_por_linha_arquivo == 0:
                    f.write(f"{pixel_value:3d}")
                elif (i + 1) % pixels_por_linha_arquivo == 0:
                    f.write(f" {pixel_value:3d}\n")
                else:
                    f.write(f" {pixel_value:3d}")
        
        elif tipo_entrada == 1:
            # Tipo 1: Aleatório com blocos contíguos de 0s e 255s (para criar fragmentação variada)
            # Inicia com um valor aleatório (preto ou branco)
            current_value = random.choice([0, max_val]) 
            
            while pixels_gerados < total_pixels:
                # Determina o comprimento do bloco atual.
                # Tendência a blocos brancos (livres) serem maiores.
                if current_value == max_val: # Bloco branco (livre)
                    block_length = random.randint(20, 200) # Comprimento maior para blocos brancos
                else: # Bloco preto (ocupado)
                    block_length = random.randint(5, 80) # Comprimento menor para blocos pretos
                
                # Garante que o bloco não ultrapasse o total de pixels restantes
                block_length = min(block_length, total_pixels - pixels_gerados)
                
                # Escreve os pixels do bloco atual
                for i in range(block_length):
                    pixel_value = current_value
                    
                    # Calcula o índice global do pixel para a formatação da linha
                    global_pixel_index = pixels_gerados + i
                    
                    if global_pixel_index % pixels_por_linha_arquivo == 0:
                        f.write(f"{pixel_value:3d}")
                    elif (global_pixel_index + 1) % pixels_por_linha_arquivo == 0:
                        f.write(f" {pixel_value:3d}\n")
                    else:
                        f.write(f" {pixel_value:3d}")
                
                pixels_gerados += block_length # Atualiza o contador de pixels gerados
                
                # Inverte o valor para o próximo bloco (alterna entre preto e branco)
                current_value = max_val if current_value == 0 else 0

# --- Chamada da função para gerar o arquivo ---
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Uso: python3 gerar_pgm_entrada.py <tipo_entrada>")
        print("  <tipo_entrada>: 1 = aleatoria com blocos contiguos (fragmentada), 2 = totalmente em branco")
        sys.exit(1)

    try:
        tipo_entrada_arg = int(sys.argv[1])
        if tipo_entrada_arg not in [1, 2]:
            raise ValueError
    except ValueError:
        print("Erro: <tipo_entrada> deve ser 1 ou 2.")
        sys.exit(1)

    output_filename = "entrada.pgm"
    gerar_pgm_entrada(output_filename, tipo_entrada=tipo_entrada_arg)
    
    if tipo_entrada_arg == 1:
        print(f"Arquivo '{output_filename}' (256x256, aleatorio com blocos contiguos) gerado com sucesso!")
    elif tipo_entrada_arg == 2:
        print(f"Arquivo '{output_filename}' (256x256, totalmente em branco) gerado com sucesso!")


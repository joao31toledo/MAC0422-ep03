# gerar_pgm_aleatorio.py
import random

def gerar_pgm_aleatorio(nome_arquivo="in.pgm", largura=256, altura=256, max_val=255, pixels_por_linha_arquivo=16):
    """
    Gera um arquivo PGM (Portable Graymap) com pixels aleatórios (0 ou 255),
    formatado com um número fixo de pixels por linha no arquivo de texto,
    conforme as especificações do EP.
    """
    total_pixels = largura * altura
    
    with open(nome_arquivo, "w") as f:
        # Escreve o cabeçalho fixo (P2, 256 256, 255)
        f.write("P2\n")
        f.write(f"{largura} {altura}\n")
        f.write(f"{max_val}\n")
        
        # Escreve os dados dos pixels
        for i in range(total_pixels):
            # Gera um valor aleatório: 0 (preto) ou 255 (branco)
            # random.randint(0, 1) retorna 0 ou 1. Multiplicamos por 255.
            pixel_value = random.randint(0, 1) * max_val 
            
            if i % pixels_por_linha_arquivo == 0:
                # Primeiro pixel da linha do arquivo (não tem espaço antes)
                f.write(f"{pixel_value:3d}") # Formata com 3 caracteres, preenchido com espaços
            elif (i + 1) % pixels_por_linha_arquivo == 0:
                # Último pixel da linha do arquivo (tem espaço antes e quebra de linha depois)
                f.write(f" {pixel_value:3d}\n")
            else:
                # Pixels intermediários na linha do arquivo (tem espaço antes)
                f.write(f" {pixel_value:3d}")

# --- Chamada da função para gerar o arquivo ---
if __name__ == "__main__":
    gerar_pgm_aleatorio("entrada.pgm")
    print("Arquivo 'entrada.pgm' (256x256, pixels aleatórios) gerado com sucesso!")
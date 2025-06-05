import sys

def parse_pgm_header(f):
    """Lê o cabeçalho PGM (P2, largura, altura, valor_max) de um objeto de arquivo."""
    p_type = f.readline().strip()
    if p_type != "P2":
        raise ValueError(f"Formato PGM inválido. Esperado 'P2', encontrado '{p_type}'")

    # Pula linhas de comentário, se houver
    line = f.readline().strip()
    while line.startswith('#'):
        line = f.readline().strip()

    width, height = map(int, line.split())
    max_val = int(f.readline().strip())
    
    return p_type, width, height, max_val

def read_pgm_pixels(f, width, height):
    """Lê os dados de pixel de um objeto de arquivo PGM."""
    pixels = []
    # Lê todas as linhas restantes (dados de pixels), junta-as e depois as divide por espaços
    all_pixel_data_str = f.read()
    pixel_values_str = all_pixel_data_str.split()
    
    for p_str in pixel_values_str:
        try:
            pixels.append(int(p_str))
        except ValueError:
            # Ignora strings vazias ou dados malformados, embora o padrão PGM deva ser limpo
            pass
            
    if len(pixels) != width * height:
        raise ValueError(f"Número de pixels lidos ({len(pixels)}) não corresponde ao esperado ({width * height}). "
                         f"Verifique se o arquivo está completo ou formatado corretamente.")
        
    return pixels

def compare_pgm_files(file1_path, file2_path):
    """Compara dois arquivos PGM pixel a pixel."""
    print(f"Comparando '{file1_path}' com '{file2_path}'...")
    
    try:
        with open(file1_path, 'r') as f1, open(file2_path, 'r') as f2:
            # Lê os cabeçalhos de ambos os arquivos
            p_type1, width1, height1, max_val1 = parse_pgm_header(f1)
            p_type2, width2, height2, max_val2 = parse_pgm_header(f2)

            # Compara os cabeçalhos primeiro
            if not (p_type1 == p_type2 == "P2" and width1 == width2 and \
                    height1 == height2 and max_val1 == max_val2):
                print("\nERRO: Os cabeçalhos dos arquivos PGM são diferentes. Não é possível comparar pixel a pixel.")
                print(f"  '{file1_path}' Cabeçalho: Tipo={p_type1}, Dim={width1}x{height1}, MaxVal={max_val1}")
                print(f"  '{file2_path}' Cabeçalho: Tipo={p_type2}, Dim={width2}x{height2}, MaxVal={max_val2}")
                return False

            # Lê os pixels de ambos os arquivos
            pixels1 = read_pgm_pixels(f1, width1, height1)
            pixels2 = read_pgm_pixels(f2, width2, height2)

            # Compara os pixels
            diff_count = 0
            for i in range(len(pixels1)):
                if pixels1[i] != pixels2[i]:
                    diff_count += 1
                    # Reporta as primeiras diferenças para dar uma ideia
                    if diff_count <= 10: # Limita a saída a 10 diferenças para brevidade
                        row = i // width1
                        col = i % width1
                        print(f"DIFERENÇA no pixel ({row},{col}) [índice global {i}]:")
                        print(f"  '{file1_path}': {pixels1[i]:3d}")
                        print(f"  '{file2_path}': {pixels2[i]:3d}")
            
            if diff_count == 0:
                print(f"\nOs arquivos '{file1_path}' e '{file2_path}' são IDÊNTICOS pixel a pixel.")
                return True
            else:
                print(f"\nOs arquivos '{file1_path}' e '{file2_path}' são DIFERENTES.")
                print(f"Total de {diff_count} pixels diferentes encontrados.")
                return False

    except FileNotFoundError:
        print(f"\nERRO: Um dos arquivos não foi encontrado.")
        print(f"Verifique se '{file1_path}' e '{file2_path}' existem e os caminhos estão corretos.")
        return False
    except ValueError as e:
        print(f"\nERRO ao processar arquivo PGM: {e}")
        print("Verifique se os arquivos PGM estão no formato P2 correto.")
        return False
    except Exception as e:
        print(f"\nOcorreu um erro inesperado: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Uso: python3 compare_pgm.py <arquivo1.pgm> <arquivo2.pgm>")
        print("Exemplo: python3 compare_pgm.py saida_firstfit.pgm saida_bestfit.pgm")
        sys.exit(1)

    file1 = sys.argv[1]
    file2 = sys.argv[2]

    compare_pgm_files(file1, file2)


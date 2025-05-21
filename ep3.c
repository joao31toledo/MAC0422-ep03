#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 100
#define TOTAL_PIXELS 65536
#define TAMANHO_PIXEL 3 // ANTIGO PIXEL_CHAR_COUNT
#define PIXELS_POR_LINHA 16 // ANTIGO PIXELS_PER_FILE_LINE
#define BYTES_POR_LINHA (PIXELS_POR_LINHA * TAMANHO_PIXEL + (PIXELS_POR_LINHA - 1) + 1) // ANTIGO LINE_FILE_BYTE_COUNT

FILE *saida_file = NULL; 
long header_size = 0;
FILE *trace_file = NULL;


void copia_conteudo(FILE *arq_entrada) {
    char buffer_header[MAX_CHARS];
    int pixel_value;

    fgets(buffer_header, sizeof(buffer_header), arq_entrada);
    fgets(buffer_header, sizeof(buffer_header), arq_entrada);
    fgets(buffer_header, sizeof(buffer_header), arq_entrada);

    fprintf(saida_file, "P2\n");
    fprintf(saida_file, "256 256\n");
    fprintf(saida_file, "255\n");
    

    for (int i = 0; i < TOTAL_PIXELS; i++) {
        if (fscanf(arq_entrada, "%d", &pixel_value) != 1) {
            fprintf(stderr, "ERRO (copia_conteudo): Falha ao ler pixel de entrada no índice %d. Abortando cópia de pixels.\n", i);
            break; // Sai do loop em caso de erro
        }

        if (i % PIXELS_POR_LINHA == 0) {
            if (fprintf(saida_file, "%*d", TAMANHO_PIXEL, pixel_value) < 0) {
                perror("ERRO (copia_conteudo): Falha ao escrever o primeiro pixel de uma linha no arquivo de saída");
                break;
            }
        } else if ((i + 1) % PIXELS_POR_LINHA == 0) {
            if (fprintf(saida_file, " %*d\n", TAMANHO_PIXEL, pixel_value) < 0) {
                perror("ERRO (copia_conteudo): Falha ao escrever o último pixel de uma linha no arquivo de saída");
                break;
            }
        } else {
            if (fprintf(saida_file, " %*d", TAMANHO_PIXEL, pixel_value) < 0) {
                perror("ERRO (copia_conteudo): Falha ao escrever um pixel intermediário na linha do arquivo de saída");
                break;
            }
        }
    }

    fclose(arq_entrada);
}

long obter_tamanho_cabecalho() {
    long initial_pos = ftell(saida_file);
    fseek(saida_file, 0, SEEK_SET);

    char local_buffer[MAX_CHARS];

    fgets(local_buffer, sizeof(local_buffer), saida_file);
    fgets(local_buffer, sizeof(local_buffer), saida_file);
    fgets(local_buffer, sizeof(local_buffer), saida_file);

    long tam_cabecalho = ftell(saida_file);
    fseek(saida_file, initial_pos, SEEK_SET);
    return tam_cabecalho;
}

int main(int argc, char *argv[]) {
    const char *entrada_path = argv[2];
    const char *trace_path = argv[3];
    const char *saida_path = argv[4];

    FILE *entrada_file = fopen(entrada_path, "r");
    saida_file = fopen(saida_path, "w"); 

    copia_conteudo(entrada_file);

    fflush(saida_file);
    fclose(saida_file);
    saida_file = NULL;

    printf("Cópia do PGM concluída.\n");

    saida_file = fopen(saida_path, "r+");

    header_size = obter_tamanho_cabecalho(); 
    printf("Tamanho do cabeçalho PGM (contado dinamicamente): %ld bytes.\n", header_size);

    trace_file = fopen(trace_path, "r");
    printf("Arquivo de trace '%s' aberto com sucesso.\n", trace_path);

    char line_buffer[50];
    int line_num;
    int mem_size;
    char command_str[20];

    while (fgets(line_buffer, sizeof(line_buffer), trace_file) != NULL) {
        if (sscanf(line_buffer, "%d %d", &line_num, &mem_size) == 2) {
            printf("Linha lida: '%s' -> Tipo: ALOCAR, Linha: %d, Tamanho: %d\n", line_buffer, line_num, mem_size);
            // Futuramente: chamar alocar(line_num, mem_size); que usará as globais
        }
        else if (sscanf(line_buffer, "%d %s", &line_num, command_str) == 2 &&
                 strcmp(command_str, "COMPACTAR") == 0) {
            printf("Linha lida: '%s' -> Tipo: COMPACTAR, Linha: %d\n", line_buffer, line_num);
            // Futuramente: chamar compactar(line_num); que usará as globais
        }
    }

    fclose(saida_file);
    fclose(trace_file);


    return 0;
}
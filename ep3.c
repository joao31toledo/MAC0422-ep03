#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void copia_conteudo(FILE *entrada_file, FILE *saida_file) {
    char caracter;

    while ((caracter = fgetc(entrada_file)) != EOF) {
        fputc(caracter, saida_file);
    }

    fclose(entrada_file);
}

int main(int argc, char *argv[]) {

    const char *entrada_nome = "entrada.pgm";
    const char *trace_nome = "trace.txt";
    const char *saida_nome = "saida.pgm";
    FILE *entrada_file = fopen(entrada_nome, "r");
    FILE *trace_file = fopen(trace_nome, "r");
    FILE *saida_file = fopen(saida_nome, "w");


    char line_buffer[50]; // Buffer para armazenar cada linha lida do trace
    int line_num;         // Para o <numero da linha>
    int mem_size;         // Para o <tamanho da memória>
    char command_str[20]; // Para a string "COMPACTAR" ou outros comandos futuros

    while (fgets(line_buffer, sizeof(line_buffer), trace_file) != NULL) {

        if (sscanf(line_buffer, "%d %d", &line_num, &mem_size) == 2) {
            printf("Linha lida: '%s' -> Tipo: ALOCAR, Linha: %d, Tamanho: %d\n", line_buffer, line_num, mem_size);
            // FUTURAMENTE: Chamar a função de alocação aqui: alocar(line_num, mem_size, memoria_pgm_file);
        }
        else if (sscanf(line_buffer, "%d %s", &line_num, command_str) == 2 &&
                 strcmp(command_str, "COMPACTAR") == 0) {
            printf("Linha lida: '%s' -> Tipo: COMPACTAR, Linha: %d\n", line_buffer, line_num);
            // FUTURAMENTE: Chamar a função de compactação aqui: compactar(line_num, memoria_pgm_file);
        }
    }


    copia_conteudo(entrada_file, saida_file);
    fclose(trace_file);
    fclose(saida_file);
    return 0;
}
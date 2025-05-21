#include <stdio.h>
#include <stdlib.h>

void copia_conteudo(FILE *entrada_file, FILE *saida_file) {
    char caracter;

    while ((caracter = fgetc(entrada_file)) != EOF) {
        fputc(caracter, saida_file);
    }

    fclose(entrada_file);
}

int main(int argc, char *argv[]) {

    const char *entrada_nome = "entrada.pgm";
    const char *saida_nome = "saida.pgm";
    FILE *entrada_file = fopen(entrada_nome, "r");
    FILE *saida_file = fopen(saida_nome, "w");


    copia_conteudo(entrada_file, saida_file);
    fclose(saida_file);
    return 0;
}
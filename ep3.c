#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 100
#define TOTAL_PIXELS 65536
#define TAMANHO_PIXEL 3
#define PIXELS_POR_LINHA 16
#define BYTES_POR_LINHA (PIXELS_POR_LINHA * TAMANHO_PIXEL + (PIXELS_POR_LINHA - 1) + 1)

FILE *saida_file = NULL; 
long header_size = 0;
static int inicio_next_fit = 0; 
FILE *trace_file = NULL;
int alocacoes_falhas = 0;


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
            break;
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

long calcula_offset(int posicao) {
    // Validação básica da posição para evitar acessos fora dos limites
    if (posicao < 0 || posicao >= TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (calcula_offset): Posicao de pixel invalida: %d. Retornando -1.\n", posicao);
        return -1; // Retorna um valor inválido para indicar erro
    }

    int linha_arquivo = posicao / PIXELS_POR_LINHA;
    int coluna_na_linha = posicao % PIXELS_POR_LINHA;

    long offset_linhas_anteriores = (long)linha_arquivo * BYTES_POR_LINHA;

    long offset_na_linha = (long)coluna_na_linha * TAMANHO_PIXEL;
    if (coluna_na_linha > 0) {
        offset_na_linha += coluna_na_linha;
    }

    long offset_total = header_size + offset_linhas_anteriores + offset_na_linha;

    return offset_total;
}

int ler_pixel(int posicao) {
    int valor_pixel = -1; 

    if (posicao < 0 || posicao >= TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (ler_pixel): Posicao de pixel invalida: %d. Retornando -1.\n", posicao);
        return -1;
    }

    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (ler_pixel): Arquivo de saida (memoria simulada) nao esta aberto. Abortando leitura.\n");
        return -1;
    }

    long offset = calcula_offset(posicao);
    if (offset == -1) {
        fprintf(stderr, "ERRO (ler_pixel): Falha ao calcular offset para posicao %d.\n", posicao);
        return -1;
    }

    long current_pos = ftell(saida_file);
    if (current_pos == -1L) {
        perror("ERRO (ler_pixel): Falha ao obter a posicao atual do ponteiro do arquivo");
        return -1;
    }

    if (fseek(saida_file, offset, SEEK_SET) != 0) {
        perror("ERRO (ler_pixel): Falha ao mover o ponteiro do arquivo para ler o pixel");
        return -1;
    }

    if (fscanf(saida_file, "%d", &valor_pixel) != 1) {
        perror("ERRO (ler_pixel): Falha ao ler o valor do pixel");
        valor_pixel = -1;
    }

    if (fseek(saida_file, current_pos, SEEK_SET) != 0) {
        perror("ERRO (ler_pixel): Falha ao restaurar a posicao do ponteiro do arquivo apos a leitura");
    }

    return valor_pixel;
}

void escreve_pixel(int posicao, int valor_pixel) {
    if (posicao < 0 || posicao >= TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (escreve_pixel): Posicao de pixel invalida: %d. Nenhuma escrita realizada.\n", posicao);
        return;
    }
    if (valor_pixel != 0 && valor_pixel != 255) {
        fprintf(stderr, "ERRO (escreve_pixel): Valor de pixel invalido: %d. Apenas 0 ou 255 sao permitidos.\n", valor_pixel);
        return;
    }
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (escreve_pixel): Arquivo de saida (memoria simulada) nao esta aberto. Abortando escrita.\n");
        return;
    }

    long offset = calcula_offset(posicao);
    if (offset == -1) {
        fprintf(stderr, "ERRO (escreve_pixel): Falha ao calcular offset para posicao %d. Abortando escrita.\n", posicao);
        return;
    }

    long current_pos = ftell(saida_file);
    if (current_pos == -1L) {
        perror("ERRO (escreve_pixel): Falha ao obter a posicao atual do ponteiro do arquivo");
        return;
    }

    if (fseek(saida_file, offset, SEEK_SET) != 0) {
        perror("ERRO (escreve_pixel): Falha ao mover o ponteiro do arquivo para escrever o pixel");
        if (fseek(saida_file, current_pos, SEEK_SET) != 0) {
             perror("ERRO (escreve_pixel): Falha ao restaurar a posicao do ponteiro APOS falha no fseek de escrita");
        }
        return;
    }

    if (fprintf(saida_file, "%*d", TAMANHO_PIXEL, valor_pixel) < 0) {
        perror("ERRO (escreve_pixel): Falha ao escrever o valor do pixel");
        if (fseek(saida_file, current_pos, SEEK_SET) != 0) {
            perror("ERRO (escreve_pixel): Falha ao restaurar a posicao do ponteiro APOS falha na escrita");
        }
        return;
    }

    if (fflush(saida_file) != 0) {
        perror("ALERTA (escreve_pixel): Falha ao forcar a escrita para o disco (fflush)");
    }

    if (fseek(saida_file, current_pos, SEEK_SET) != 0) {
        perror("ERRO (escreve_pixel): Falha ao restaurar a posicao do ponteiro do arquivo apos a escrita");
    }
}

void compacta_memoria() {
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (compacta_memoria): Arquivo de saida (memoria simulada) nao esta aberto. Abortando compactacao.\n");
        return;
    }

    int ponteiro_leitura = 0;
    int ponteiro_escrita = 0;
    int valor_pixel;

    while (ponteiro_leitura < TOTAL_PIXELS) {
        valor_pixel = ler_pixel(ponteiro_leitura);

        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (compacta_memoria): Falha ao ler pixel na posicao %d. Abortando compactacao.\n", ponteiro_leitura);
            return;
        }

        if (valor_pixel == 0) { 
            escreve_pixel(ponteiro_escrita, 0); 
            ponteiro_escrita++; 
        }

        ponteiro_leitura++; 
    }

    while (ponteiro_escrita < TOTAL_PIXELS) {
        escreve_pixel(ponteiro_escrita, 255);
        ponteiro_escrita++; 
    }
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

int alocar_first_fit(int tamanho_processo) {
    if (tamanho_processo <= 0 || tamanho_processo > TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (alocar_first_fit): Tamanho de processo invalido: %d. Deve ser entre 1 e %d.\n",
                tamanho_processo, TOTAL_PIXELS);
        alocacoes_falhas++;
        return -1;
    }
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar_first_fit): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        alocacoes_falhas++;
        return -1;
    }

    int posicao_inicial_bloco_livre = -1;
    int tamanho_bloco_livre_atual = 0; 
    int valor_pixel; 

    for (int i = 0; i < TOTAL_PIXELS; i++) {
        valor_pixel = ler_pixel(i);

        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_first_fit): Falha ao ler pixel na posicao %d. Abortando busca por bloco.\n", i);
            alocacoes_falhas++;
            return -1;
        }

        if (valor_pixel == 255) {
            if (posicao_inicial_bloco_livre == -1) {
                posicao_inicial_bloco_livre = i;
            }
            tamanho_bloco_livre_atual++;
        } else {
            if (posicao_inicial_bloco_livre != -1 && 
                tamanho_bloco_livre_atual >= tamanho_processo &&
                (posicao_inicial_bloco_livre + tamanho_processo <= TOTAL_PIXELS)) {
                for (int j = 0; j < tamanho_processo; j++) {
                    int target_pos = posicao_inicial_bloco_livre + j;
                    if (target_pos >= TOTAL_PIXELS) {
                        fprintf(stderr, "ERRO INTERNO GRAVE (alocar_first_fit): Posicao de escrita %d excede TOTAL_PIXELS %d (j=%d)! ISSO NAO DEVERIA ACONTECER!\n",
                                target_pos, TOTAL_PIXELS, j);
                        alocacoes_falhas++;
                        return -1;
                    }
                    escreve_pixel(target_pos, 0);
                }

                return posicao_inicial_bloco_livre; 
            }
            posicao_inicial_bloco_livre = -1;
            tamanho_bloco_livre_atual = 0;
        }
    }

    if (posicao_inicial_bloco_livre != -1 && 
        tamanho_bloco_livre_atual >= tamanho_processo &&
        (posicao_inicial_bloco_livre + tamanho_processo <= TOTAL_PIXELS)) { 
        
        for (int j = 0; j < tamanho_processo; j++) {
            int target_pos = posicao_inicial_bloco_livre + j;
            if (target_pos >= TOTAL_PIXELS) {
                fprintf(stderr, "ERRO INTERNO GRAVE (alocar_first_fit): Posicao de escrita %d excede TOTAL_PIXELS %d (j=%d) APOS CHECAGEM FINAL! ISSO NAO DEVERIA ACONTECER!\n",
                                target_pos, TOTAL_PIXELS, j);
                alocacoes_falhas++;
                return -1; 
            }
            escreve_pixel(target_pos, 0);
        }

        return posicao_inicial_bloco_livre;
    }

    alocacoes_falhas++;
    return -1;
}

int alocar_next_fit(int tamanho_processo) {
    if (tamanho_processo <= 0 || tamanho_processo > TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (alocar_next_fit): Tamanho de processo invalido: %d. Deve ser entre 1 e %d.\n",
                tamanho_processo, TOTAL_PIXELS);
        alocacoes_falhas++;
        return -1;
    }
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar_next_fit): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        alocacoes_falhas++;
        return -1;
    }

    int posicao_inicial_bloco_livre_encontrado = -1;
    int tamanho_bloco_livre_encontrado = 0;
    int valor_pixel;

    for (int i = inicio_next_fit; i < TOTAL_PIXELS; i++) {
        valor_pixel = ler_pixel(i);

        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_next_fit): Falha ao ler pixel na posicao %d. Abortando busca na primeira passagem.\n", i);
            alocacoes_falhas++;
            return -1;
        }

        if (valor_pixel == 255) {
            if (tamanho_bloco_livre_encontrado == 0) {
                posicao_inicial_bloco_livre_encontrado = i;
            }
            tamanho_bloco_livre_encontrado++;

            if (tamanho_bloco_livre_encontrado >= tamanho_processo) {
                for (int j = 0; j < tamanho_processo; j++) {
                    escreve_pixel(posicao_inicial_bloco_livre_encontrado + j, 0); 
                }
                inicio_next_fit = (posicao_inicial_bloco_livre_encontrado + tamanho_processo) % TOTAL_PIXELS;
                
                return posicao_inicial_bloco_livre_encontrado;
            }
        } else {
            posicao_inicial_bloco_livre_encontrado = -1;
            tamanho_bloco_livre_encontrado = 0;
        }
    }

    tamanho_bloco_livre_encontrado = 0;
    posicao_inicial_bloco_livre_encontrado = -1;

    for (int i = 0; i < inicio_next_fit; i++) { 
        valor_pixel = ler_pixel(i); 

        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_next_fit): Falha ao ler pixel na posicao %d. Abortando busca na segunda passagem.\n", i);
            alocacoes_falhas++;
            return -1;
        }

        if (valor_pixel == 255) {
            if (tamanho_bloco_livre_encontrado == 0) { 
                posicao_inicial_bloco_livre_encontrado = i;
            }
            tamanho_bloco_livre_encontrado++; 

            if (tamanho_bloco_livre_encontrado >= tamanho_processo) {
                for (int j = 0; j < tamanho_processo; j++) {
                    escreve_pixel(posicao_inicial_bloco_livre_encontrado + j, 0); 
                }
                inicio_next_fit = (posicao_inicial_bloco_livre_encontrado + tamanho_processo) % TOTAL_PIXELS;
                
                return posicao_inicial_bloco_livre_encontrado;
            }
        } else {
            posicao_inicial_bloco_livre_encontrado = -1;
            tamanho_bloco_livre_encontrado = 0;
        }
    }

    alocacoes_falhas++;
    return -1;
}

int alocar_best_fit(int tamanho_processo) {
    if (tamanho_processo <= 0 || tamanho_processo > TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (alocar_best_fit): Tamanho de processo invalido: %d. Deve ser entre 1 e %d.\n",
                tamanho_processo, TOTAL_PIXELS);
        return -1;
    }
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar_best_fit): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        return -1;
    }

    int best_fit_posicao_inicial = -1;
    int best_fit_tamanho = TOTAL_PIXELS + 1;

    int posicao_inicial_bloco_livre_atual = -1;
    int tamanho_bloco_livre_atual = 0;
    int valor_pixel;

    for (int i = 0; i < TOTAL_PIXELS; i++) {
        valor_pixel = ler_pixel(i);

        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_best_fit): Falha ao ler pixel na posicao %d. Abortando busca por bloco.\n", i);
            return -1;
        }

        if (valor_pixel == 255) {
            if (posicao_inicial_bloco_livre_atual == -1) {
                posicao_inicial_bloco_livre_atual = i;
            }
            tamanho_bloco_livre_atual++;
        } else {
            if (posicao_inicial_bloco_livre_atual != -1 && tamanho_bloco_livre_atual >= tamanho_processo) {
                if (tamanho_bloco_livre_atual < best_fit_tamanho) {
                    best_fit_tamanho = tamanho_bloco_livre_atual;
                    best_fit_posicao_inicial = posicao_inicial_bloco_livre_atual;
                }
            }
            posicao_inicial_bloco_livre_atual = -1;
            tamanho_bloco_livre_atual = 0;
        }
    }

    if (posicao_inicial_bloco_livre_atual != -1 && tamanho_bloco_livre_atual >= tamanho_processo) {
        if (tamanho_bloco_livre_atual < best_fit_tamanho) {
            best_fit_tamanho = tamanho_bloco_livre_atual;
            best_fit_posicao_inicial = posicao_inicial_bloco_livre_atual;
        }
    }

    if (best_fit_posicao_inicial == -1) {
        alocacoes_falhas++;
        return -1;
    } else {
        for (int j = 0; j < tamanho_processo; j++) {
            escreve_pixel(best_fit_posicao_inicial + j, 0);
        }
        return best_fit_posicao_inicial;
    }
}

int alocar_worst_fit(int tamanho_processo) {

    if (tamanho_processo <= 0 || tamanho_processo > TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (alocar_worst_fit): Tamanho de processo invalido: %d. Deve ser entre 1 e %d.\n",
                tamanho_processo, TOTAL_PIXELS);
        return -1;
    }
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar_worst_fit): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        return -1;
    }
    int worst_fit_posicao_inicial = -1;
    int worst_fit_tamanho = 0;

    int posicao_inicial_bloco_livre_atual = -1;
    int tamanho_bloco_livre_atual = 0;
    int valor_pixel;

    for (int i = 0; i < TOTAL_PIXELS; i++) {
        valor_pixel = ler_pixel(i);
        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_worst_fit): Falha ao ler pixel na posicao %d. Abortando busca por bloco.\n", i);
            return -1;
        }

        if (valor_pixel == 255) {
            if (posicao_inicial_bloco_livre_atual == -1) {
                posicao_inicial_bloco_livre_atual = i;
            }
            tamanho_bloco_livre_atual++; 
        } else {
            if (posicao_inicial_bloco_livre_atual != -1 && tamanho_bloco_livre_atual >= tamanho_processo) {
                if (tamanho_bloco_livre_atual > worst_fit_tamanho) {
                    worst_fit_tamanho = tamanho_bloco_livre_atual;
                    worst_fit_posicao_inicial = posicao_inicial_bloco_livre_atual;
                }
            }
            posicao_inicial_bloco_livre_atual = -1;
            tamanho_bloco_livre_atual = 0;
        }
    }

    if (posicao_inicial_bloco_livre_atual != -1 && tamanho_bloco_livre_atual >= tamanho_processo) {
        if (tamanho_bloco_livre_atual > worst_fit_tamanho) {
            worst_fit_tamanho = tamanho_bloco_livre_atual;
            worst_fit_posicao_inicial = posicao_inicial_bloco_livre_atual;
        }
    }

    if (worst_fit_posicao_inicial == -1) {
        alocacoes_falhas++;
        return -1; // Falha na alocação
    } else {
        for (int j = 0; j < tamanho_processo; j++) {
            escreve_pixel(worst_fit_posicao_inicial + j, 0); 
        }


        return worst_fit_posicao_inicial;
    }
}

int alocar(int tipo_algoritmo, int tamanho_processo) {
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        return -1;
    }

    // O switch/case para chamar o algoritmo correto
    switch (tipo_algoritmo) {
        case 1:
            return alocar_first_fit(tamanho_processo);
        case 2:
            return alocar_next_fit(tamanho_processo);
        case 3:
            return alocar_best_fit(tamanho_processo);
        case 4:
            return alocar_worst_fit(tamanho_processo);
        default:
            fprintf(stderr, "ERRO (alocar): Tipo de algoritmo invalido: %d. Esperado 1 a 4.\n", tipo_algoritmo);
            return -1;
    }
}

int main(int argc, char *argv[]) {
    int tipo_algoritmo = atoi(argv[1]);
    const char *entrada_path = argv[2];
    const char *trace_path = argv[3];
    const char *saida_path = argv[4];

    FILE *entrada_file = fopen(entrada_path, "r");
    saida_file = fopen(saida_path, "w"); 

    copia_conteudo(entrada_file);

    fflush(saida_file);
    fclose(saida_file);
    saida_file = NULL;
    saida_file = fopen(saida_path, "r+");

    header_size = obter_tamanho_cabecalho(); 

    trace_file = fopen(trace_path, "r");

    char line_buffer[50];
    int line_num;
    int mem_size;
    char command_str[20];

    while (fgets(line_buffer, sizeof(line_buffer), trace_file) != NULL) {
        if (sscanf(line_buffer, "%d %d", &line_num, &mem_size) == 2) {
            alocar(tipo_algoritmo, mem_size);
        }
        else if (sscanf(line_buffer, "%d %s", &line_num, command_str) == 2 && strcmp(command_str, "COMPACTAR") == 0) {
            compacta_memoria();
        }
    }


    printf("%d\n", alocacoes_falhas);
    fclose(saida_file);
    fclose(trace_file);


    return 0;
}
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

long calcula_offset(int posicao) {
    // Validação básica da posição para evitar acessos fora dos limites
    if (posicao < 0 || posicao >= TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (calcula_offset): Posicao de pixel invalida: %d. Retornando -1.\n", posicao);
        return -1; // Retorna um valor inválido para indicar erro
    }

    // 1. Calcular a linha e coluna do pixel no layout do arquivo (16 pixels por linha)
    int linha_arquivo = posicao / PIXELS_POR_LINHA;      // Índice da linha de 16 pixels no arquivo
    int coluna_na_linha = posicao % PIXELS_POR_LINHA;    // Índice da coluna dentro dessa linha (0 a 15)

    // 2. Calcular o offset das linhas completas que vêm antes da linha do pixel atual
    // Cada linha de 16 pixels ocupa LINE_FILE_BYTE_COUNT bytes.
    long offset_linhas_anteriores = (long)linha_arquivo * BYTES_POR_LINHA;

    // 3. Calcular o offset dentro da linha atual para a coluna do pixel
    // Cada pixel ocupa PIXEL_CHAR_COUNT (3) caracteres.
    // Há um espaço ANTES de cada pixel, EXCETO o primeiro de cada linha (coluna_na_linha == 0).
    // O número de espaços a adicionar é igual à coluna_na_linha.
    long offset_na_linha = (long)coluna_na_linha * TAMANHO_PIXEL;
    if (coluna_na_linha > 0) {
        offset_na_linha += coluna_na_linha; // Adiciona os bytes dos espaços
    }

    // 4. Somar o offset do cabeçalho, das linhas anteriores e da posição na linha
    long offset_total = header_size + offset_linhas_anteriores + offset_na_linha;

    // Mensagem de debug para rastrear os cálculos (útil para testar)
    fprintf(stderr, "DEBUG (calcula_offset): Posicao linear %d -> Linha_arquivo %d, Coluna_na_linha %d -> Offset total: %ld bytes\n",
            posicao, linha_arquivo, coluna_na_linha, offset_total);

    return offset_total;
}

int ler_pixel(int posicao) {
    int valor_pixel = -1; // Valor padrão de erro

    // 1. Validação básica da posição
    if (posicao < 0 || posicao >= TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (ler_pixel): Posicao de pixel invalida: %d. Retornando -1.\n", posicao);
        return -1;
    }

    // Verifica se o arquivo de saída está aberto e pronto para leitura
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (ler_pixel): Arquivo de saida (memoria simulada) nao esta aberto. Abortando leitura.\n");
        return -1;
    }

    // 2. Calcular o offset em bytes do pixel
    long offset = calcula_offset(posicao);
    if (offset == -1) { // Verifica se calcula_offset retornou erro
        fprintf(stderr, "ERRO (ler_pixel): Falha ao calcular offset para posicao %d.\n", posicao);
        return -1;
    }

    // 3. Salvar a posição atual do ponteiro do arquivo
    long current_pos = ftell(saida_file);
    if (current_pos == -1L) {
        perror("ERRO (ler_pixel): Falha ao obter a posicao atual do ponteiro do arquivo");
        return -1;
    }

    // 4. Mover o ponteiro do arquivo para o offset do pixel
    if (fseek(saida_file, offset, SEEK_SET) != 0) {
        perror("ERRO (ler_pixel): Falha ao mover o ponteiro do arquivo para ler o pixel");
        return -1;
    }

    // 5. Ler o valor do pixel
    if (fscanf(saida_file, "%d", &valor_pixel) != 1) {
        perror("ERRO (ler_pixel): Falha ao ler o valor do pixel");
        valor_pixel = -1; // Garante que o retorno seja -1 em caso de erro na leitura
    }

    // 6. Restaurar a posição original do ponteiro do arquivo
    if (fseek(saida_file, current_pos, SEEK_SET) != 0) {
        perror("ERRO (ler_pixel): Falha ao restaurar a posicao do ponteiro do arquivo apos a leitura");
        // Este erro não impede a função de retornar o valor lido (se a leitura foi bem-sucedida),
        // mas é importante ser logado para debug.
    }

    fprintf(stderr, "DEBUG (ler_pixel): Pixel na posicao %d lido: %d\n", posicao, valor_pixel);
    return valor_pixel;
}

void escreve_pixel(int posicao, int valor_pixel) {
    // 1. Validação dos parâmetros de entrada
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

    // 2. Calcular o offset em bytes do pixel
    long offset = calcula_offset(posicao);
    if (offset == -1) { // Verifica se calcula_offset retornou erro
        fprintf(stderr, "ERRO (escreve_pixel): Falha ao calcular offset para posicao %d. Abortando escrita.\n", posicao);
        return;
    }

    // 3. Salvar a posição atual do ponteiro do arquivo
    long current_pos = ftell(saida_file);
    if (current_pos == -1L) {
        perror("ERRO (escreve_pixel): Falha ao obter a posicao atual do ponteiro do arquivo");
        return;
    }

    // 4. Mover o ponteiro do arquivo para o offset do pixel
    if (fseek(saida_file, offset, SEEK_SET) != 0) {
        perror("ERRO (escreve_pixel): Falha ao mover o ponteiro do arquivo para escrever o pixel");
        // Tentamos restaurar a posição original mesmo em caso de falha no fseek de escrita
        if (fseek(saida_file, current_pos, SEEK_SET) != 0) {
             perror("ERRO (escreve_pixel): Falha ao restaurar a posicao do ponteiro APOS falha no fseek de escrita");
        }
        return;
    }

    // 5. Escrever o novo valor do pixel, garantindo a formatação exata (3 caracteres)
    // O "%*d" usa o valor de TAMANHO_PIXEL para preencher com espaços à esquerda se necessário.
    if (fprintf(saida_file, "%*d", TAMANHO_PIXEL, valor_pixel) < 0) {
        perror("ERRO (escreve_pixel): Falha ao escrever o valor do pixel");
        // Tentamos restaurar a posição original mesmo em caso de falha no fprintf
        if (fseek(saida_file, current_pos, SEEK_SET) != 0) {
            perror("ERRO (escreve_pixel): Falha ao restaurar a posicao do ponteiro APOS falha na escrita");
        }
        return;
    }

    // 6. Forçar a escrita para o disco (opcional, mas recomendado para segurança)
    if (fflush(saida_file) != 0) {
        perror("ALERTA (escreve_pixel): Falha ao forcar a escrita para o disco (fflush)");
        // Não é um erro crítico que impeça a função de continuar, mas é bom saber.
    }

    // 7. Restaurar a posição original do ponteiro do arquivo
    if (fseek(saida_file, current_pos, SEEK_SET) != 0) {
        perror("ERRO (escreve_pixel): Falha ao restaurar a posicao do ponteiro do arquivo apos a escrita");
        // Este é um erro importante de ser logado.
    }

    fprintf(stderr, "DEBUG (escreve_pixel): Pixel na posicao %d escrito com valor: %d\n", posicao, valor_pixel);
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
    saida_file = fopen(saida_path, "r+");

    header_size = obter_tamanho_cabecalho(); 

    trace_file = fopen(trace_path, "r");

    char line_buffer[50];
    int line_num;
    int mem_size;
    char command_str[20];

    while (fgets(line_buffer, sizeof(line_buffer), trace_file) != NULL) {
        if (sscanf(line_buffer, "%d %d", &line_num, &mem_size) == 2) {
            // Futuramente: chamar alocar(line_num, mem_size); que usará as globais
        }
        else if (sscanf(line_buffer, "%d %s", &line_num, command_str) == 2 &&
                 strcmp(command_str, "COMPACTAR") == 0) {
            // Futuramente: chamar compactar(line_num); que usará as globais
        }
    }

    int valor_lido;

    // Teste 1: Ler o pixel 0 (primeiro pixel)
    valor_lido = ler_pixel(0);
    fprintf(stderr, "Valor do pixel na posicao 0: (esperado = 0) %d\n", valor_lido);

    // Teste 2: Ler o pixel 15 (ultimo da primeira linha)
    valor_lido = ler_pixel(15);
    fprintf(stderr, "Valor do pixel na posicao 15: (esperado = 0) %d\n", valor_lido);

    // Teste 3: Ler o pixel 16 (primeiro da segunda linha)
    valor_lido = ler_pixel(16);
    fprintf(stderr, "Valor do pixel na posicao 16: (esperado = 0) %d\n", valor_lido);

    // Teste 4: Ler um pixel no "meio" (ex: 65534)
    valor_lido = ler_pixel(65534);
    fprintf(stderr, "Valor do pixel na posicao 65534: (esperado = 255) %d\n", valor_lido);

    // Teste 5: Ler o ultimo pixel (65535)
    valor_lido = ler_pixel(65535);
    fprintf(stderr, "Valor do pixel na posicao 65535: (esperado = 255) %d\n", valor_lido);

    /////////////////////////////////////////////////////////////////////////////////////////////
    escreve_pixel(0, 255);
    valor_lido = ler_pixel(0);
    fprintf(stderr, "Valor do pixel na posicao 0 após escrita: (esperado = 255) %d\n", valor_lido);

    escreve_pixel(15,255);
    valor_lido = ler_pixel(15);
    fprintf(stderr, "Valor do pixel na posicao 15 após escrita: (esperado = 255) %d\n", valor_lido);

    escreve_pixel(16,255);
    valor_lido = ler_pixel(16);
    fprintf(stderr, "Valor do pixel na posicao 16 após escrita: (esperado = 255) %d\n", valor_lido);
    
    escreve_pixel(65534,0);
    valor_lido = ler_pixel(65534);
    fprintf(stderr, "Valor do pixel na posicao 65534 após escrita: (esperado = 0) %d\n", valor_lido);

    escreve_pixel(65535,0);
    valor_lido = ler_pixel(65535);
    fprintf(stderr, "Valor do pixel na posicao 65535 após escrita: (esperado = 0) %d\n", valor_lido);

    fclose(saida_file);
    fclose(trace_file);


    return 0;
}
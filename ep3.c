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
    fprintf(stderr, "DEBUG (ler_pixel): Lendo posicao %d, offset calculado: %ld\n", posicao, offset); // ADDED DEBUG

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
    fprintf(stderr, "DEBUG (escreve_pixel): Escrevendo posicao %d, offset calculado: %ld\n", posicao, offset); // ADDED DEBUG

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
}


void compacta_memoria() {

    // Validação inicial: verifica se o arquivo de memória está aberto
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (compacta_memoria): Arquivo de saida (memoria simulada) nao esta aberto. Abortando compactacao.\n");
        return;
    }

    int ponteiro_leitura = 0; // Varre toda a memoria para encontrar pixels ocupados
    int ponteiro_escrita = 0; // Aponta para a proxima posicao onde um pixel ocupado deve ser escrito
    int valor_pixel;          // Armazena o valor do pixel lido

    // --- Primeira Passagem: Mover blocos ocupados para o início ---
    // O ponteiro_leitura avanca por todas as posicoes
    while (ponteiro_leitura < TOTAL_PIXELS) {
        valor_pixel = ler_pixel(ponteiro_leitura); // Le o pixel na posicao atual de leitura

        // Se ler_pixel retornar -1, indica um erro grave.
        // Nao podemos continuar com a compactacao se nao conseguimos ler a memoria.
        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (compacta_memoria): Falha ao ler pixel na posicao %d. Abortando compactacao.\n", ponteiro_leitura);
            return;
        }

        if (valor_pixel == 0) { // Se o pixel lido estiver OCUPADO (preto)
            // Escrevemos este pixel preto na posicao atual de escrita.
            // (Se ponteiro_leitura == ponteiro_escrita, eh uma escrita "no lugar")
            escreve_pixel(ponteiro_escrita, 0); 
            ponteiro_escrita++; // Avanca o ponteiro de escrita para a proxima vaga para um ocupado
        }
        // Se valor_pixel for 255 (branco), nao fazemos nada com ele nesta fase.
        // Apenas continuamos a procurar por pixels ocupados.

        ponteiro_leitura++; // Sempre avanca o ponteiro de leitura
    }

    // --- Segunda Passagem: Preencher o restante com blocos livres ---
    // Apos a primeira passagem, 'ponteiro_escrita' indica a primeira posicao
    // que deve ser preenchida com um pixel branco (livre).
    while (ponteiro_escrita < TOTAL_PIXELS) {
        escreve_pixel(ponteiro_escrita, 255); // Escreve 255 (branco/livre)
        ponteiro_escrita++; // Avanca para a proxima posicao
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
    fprintf(stderr, "DEBUG (alocar_first_fit): Tentando alocar %d unidades usando First-Fit.\n", tamanho_processo);

    // 1. Validação inicial dos parâmetros
    if (tamanho_processo <= 0 || tamanho_processo > TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (alocar_first_fit): Tamanho de processo invalido: %d. Deve ser entre 1 e %d.\n",
                tamanho_processo, TOTAL_PIXELS);
        alocacoes_falhas++; // Incrementa contador de falhas
        return -1;
    }
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar_first_fit): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        alocacoes_falhas++; // Incrementa contador de falhas
        return -1;
    }

    int posicao_inicial_bloco_livre = -1; // Guarda o início do bloco livre atual
    int tamanho_bloco_livre_atual = 0;    // Conta o tamanho do bloco livre atual
    int valor_pixel;                      // Armazena o valor do pixel lido

    // 2. Loop principal: Percorrer a memória pixel por pixel para encontrar um bloco livre
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        valor_pixel = ler_pixel(i); // Lê o estado do pixel na posição 'i'

        // Verifica se houve erro na leitura do pixel. Se sim, aborta.
        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_first_fit): Falha ao ler pixel na posicao %d. Abortando busca por bloco.\n", i);
            alocacoes_falhas++; // Incrementa contador de falhas
            return -1;
        }

        if (valor_pixel == 255) { // Se o pixel atual é BRANCO (LIVRE)
            if (posicao_inicial_bloco_livre == -1) {
                // Se estamos começando um novo bloco livre
                posicao_inicial_bloco_livre = i;
            }
            tamanho_bloco_livre_atual++; // Incrementa o contador do tamanho do bloco livre atual
        } else { // Se o pixel atual é PRETO (OCUPADO)
            // O bloco livre atual foi interrompido.
            // Verifica se o bloco livre encontrado ANTES do pixel preto era grande o suficiente
            // E SE ELE CABE LINEARMENTE ATÉ O FINAL DA MEMÓRIA
            if (posicao_inicial_bloco_livre != -1 && 
                tamanho_bloco_livre_atual >= tamanho_processo &&
                (posicao_inicial_bloco_livre + tamanho_processo <= TOTAL_PIXELS)) { // Check if allocation fits within bounds
                
                fprintf(stderr, "DEBUG (alocar_first_fit - ENCONTRADO 1): Posicao inicial livre: %d, Tam livre: %d, Tam processo: %d\n",
                        posicao_inicial_bloco_livre, tamanho_bloco_livre_atual, tamanho_processo);
                fprintf(stderr, "DEBUG (alocar_first_fit - ENCONTRADO 1): Fim teorico da alocacao: %d (TOTAL_PIXELS: %d)\n",
                        posicao_inicial_bloco_livre + tamanho_processo, TOTAL_PIXELS);

                // 3. Alocar o processo: marcar os pixels como ocupados (preto)
                for (int j = 0; j < tamanho_processo; j++) {
                    int target_pos = posicao_inicial_bloco_livre + j;
                    // Defesa extra: embora a condição acima deva impedir,
                    // esta checagem garante que NENHUMA escrita ocorra fora dos limites.
                    if (target_pos >= TOTAL_PIXELS) {
                        fprintf(stderr, "ERRO INTERNO GRAVE (alocar_first_fit): Posicao de escrita %d excede TOTAL_PIXELS %d (j=%d)! ISSO NAO DEVERIA ACONTECER!\n",
                                target_pos, TOTAL_PIXELS, j);
                        alocacoes_falhas++;
                        return -1; // Retorna imediatamente em caso de erro crítico inesperado
                    }
                    escreve_pixel(target_pos, 0); // Marca como ocupado
                }

                fprintf(stderr, "DEBUG (alocar_first_fit): Alocado processo de tamanho %d na posicao %d.\n",
                        tamanho_processo, posicao_inicial_bloco_livre);
                return posicao_inicial_bloco_livre; // Retorna a posicao inicial da alocacao
            }
            // Se o bloco livre encontrado não foi grande o suficiente, ou se não havia bloco livre,
            // resetamos as variáveis para procurar o próximo bloco livre.
            posicao_inicial_bloco_livre = -1;
            tamanho_bloco_livre_atual = 0;
        }
    }

    // 4. Verificação final: Após o loop, se chegamos ao fim da memória,
    // o último bloco livre (se houver) ainda precisa ser avaliado.
    // E SE ELE CABE LINEARMENTE ATÉ O FINAL DA MEMÓRIA
    if (posicao_inicial_bloco_livre != -1 && 
        tamanho_bloco_livre_atual >= tamanho_processo &&
        (posicao_inicial_bloco_livre + tamanho_processo <= TOTAL_PIXELS)) { // Check if allocation fits within bounds
        
        fprintf(stderr, "DEBUG (alocar_first_fit - ENCONTRADO FINAL): Posicao inicial livre: %d, Tam livre: %d, Tam processo: %d\n",
                        posicao_inicial_bloco_livre, tamanho_bloco_livre_atual, tamanho_processo);
        fprintf(stderr, "DEBUG (alocar_first_fit - ENCONTRADO FINAL): Fim teorico da alocacao: %d (TOTAL_PIXELS: %d)\n",
                        posicao_inicial_bloco_livre + tamanho_processo, TOTAL_PIXELS);

        for (int j = 0; j < tamanho_processo; j++) {
            int target_pos = posicao_inicial_bloco_livre + j;
            // Defesa extra: embora a condição acima deva impedir,
            // esta checagem garante que NENHUMA escrita ocorra fora dos limites.
            if (target_pos >= TOTAL_PIXELS) {
                fprintf(stderr, "ERRO INTERNO GRAVE (alocar_first_fit): Posicao de escrita %d excede TOTAL_PIXELS %d (j=%d) APOS CHECAGEM FINAL! ISSO NAO DEVERIA ACONTECER!\n",
                                target_pos, TOTAL_PIXELS, j);
                alocacoes_falhas++;
                return -1; // Retorna imediatamente em caso de erro crítico inesperado
            }
            escreve_pixel(target_pos, 0); // Marca como ocupado
        }
        fprintf(stderr, "DEBUG (alocar_first_fit): Alocado processo de tamanho %d na posicao %d (fim da memoria).\n",
                tamanho_processo, posicao_inicial_bloco_livre);
        return posicao_inicial_bloco_livre;
    }

    // Se o loop terminou e nenhum bloco adequado foi encontrado
    fprintf(stderr, "ERRO (alocar_first_fit): Nao foi possivel alocar processo de tamanho %d. Memoria cheia ou muito fragmentada.\n",
            tamanho_processo);
    alocacoes_falhas++; // Incrementa contador de falhas
    return -1; // Falha na alocação
}


int alocar_next_fit(int tamanho_processo) {
    fprintf(stderr, "DEBUG (alocar_next_fit): Tentando alocar %d unidades usando Next-Fit. (Inicio da busca: %d)\n",
            tamanho_processo, inicio_next_fit);

    // 1. Validação inicial dos parâmetros
    if (tamanho_processo <= 0 || tamanho_processo > TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (alocar_next_fit): Tamanho de processo invalido: %d. Deve ser entre 1 e %d.\n",
                tamanho_processo, TOTAL_PIXELS);
        alocacoes_falhas++; // Incrementa contador de falhas
        return -1;
    }
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar_next_fit): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        alocacoes_falhas++; // Incrementa contador de falhas
        return -1;
    }

    int posicao_inicial_bloco_livre_encontrado = -1; // Guarda o início do bloco livre atual
    int tamanho_bloco_livre_encontrado = 0;    // Conta o tamanho do bloco livre atual
    int valor_pixel;                      // Armazena o valor do pixel lido

    // --- PRIMEIRA PASSAGEM: Busca do 'inicio_next_fit' até o final da memória ---
    // Percorre a memória a partir do último ponto de alocação até o fim
    for (int i = inicio_next_fit; i < TOTAL_PIXELS; i++) {
        valor_pixel = ler_pixel(i); // Lê o estado do pixel na posição 'i'

        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_next_fit): Falha ao ler pixel na posicao %d. Abortando busca na primeira passagem.\n", i);
            alocacoes_falhas++; // Incrementa contador de falhas
            return -1;
        }

        if (valor_pixel == 255) { // Se o pixel atual é BRANCO (LIVRE)
            if (tamanho_bloco_livre_encontrado == 0) { // Se é o início de um novo bloco livre
                posicao_inicial_bloco_livre_encontrado = i;
            }
            tamanho_bloco_livre_encontrado++; // Incrementa o contador do tamanho do bloco livre atual

            // Verifica se o bloco livre atual é grande o suficiente E se ele cabe linearmente
            if (tamanho_bloco_livre_encontrado >= tamanho_processo) {
                // Encontramos o bloco! Aloca e retorna.
                for (int j = 0; j < tamanho_processo; j++) {
                    escreve_pixel(posicao_inicial_bloco_livre_encontrado + j, 0); // Marca como ocupado
                }
                // Atualiza a posição de início para a PRÓXIMA busca
                inicio_next_fit = (posicao_inicial_bloco_livre_encontrado + tamanho_processo) % TOTAL_PIXELS;
                
                fprintf(stderr, "DEBUG (alocar_next_fit): Alocado processo de tamanho %d na posicao %d (Primeira Passagem). Proxima busca iniciara em %d.\n",
                        tamanho_processo, posicao_inicial_bloco_livre_encontrado, inicio_next_fit);
                return posicao_inicial_bloco_livre_encontrado; // Retorna a posicao inicial da alocacao
            }
        } else { // Se o pixel atual é PRETO (OCUPADO), o bloco livre atual foi interrompido
            // Resetamos as variáveis para procurar o próximo bloco livre
            posicao_inicial_bloco_livre_encontrado = -1;
            tamanho_bloco_livre_encontrado = 0;
        }
    }

    // --- SEGUNDA PASSAGEM: Busca do início da memória (0) até 'inicio_next_fit - 1' ---
    // Se não encontrou na primeira passagem, reinicia a busca do começo da memória
    // até a posição onde a primeira passagem começou (excluindo-a).
    
    // Reinicia as variáveis de bloco livre para esta nova busca
    tamanho_bloco_livre_encontrado = 0;
    posicao_inicial_bloco_livre_encontrado = -1;

    for (int i = 0; i < inicio_next_fit; i++) { // Vai até (mas não inclui) inicio_next_fit
        valor_pixel = ler_pixel(i); // Lê o estado do pixel na posição 'i'

        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_next_fit): Falha ao ler pixel na posicao %d. Abortando busca na segunda passagem.\n", i);
            alocacoes_falhas++; // Incrementa contador de falhas
            return -1;
        }

        if (valor_pixel == 255) { // Se o pixel atual é BRANCO (LIVRE)
            if (tamanho_bloco_livre_encontrado == 0) { // Se é o início de um novo bloco livre
                posicao_inicial_bloco_livre_encontrado = i;
            }
            tamanho_bloco_livre_encontrado++; // Incrementa o contador do tamanho do bloco livre atual

            // Verifica se o bloco livre atual é grande o suficiente E se ele cabe linearmente
            if (tamanho_bloco_livre_encontrado >= tamanho_processo) {
                // Encontramos o bloco! Aloca e retorna.
                for (int j = 0; j < tamanho_processo; j++) {
                    escreve_pixel(posicao_inicial_bloco_livre_encontrado + j, 0); // Marca como ocupado
                }
                // Atualiza a posição de início para a PRÓXIMA busca
                inicio_next_fit = (posicao_inicial_bloco_livre_encontrado + tamanho_processo) % TOTAL_PIXELS;
                
                fprintf(stderr, "DEBUG (alocar_next_fit): Alocado processo de tamanho %d na posicao %d (Segunda Passagem). Proxima busca iniciara em %d.\n",
                        tamanho_processo, posicao_inicial_bloco_livre_encontrado, inicio_next_fit);
                return posicao_inicial_bloco_livre_encontrado; // Retorna a posicao inicial da alocacao
            }
        } else { // Se o pixel atual é PRETO (OCUPADO), o bloco livre atual foi interrompido
            // Resetamos as variáveis para procurar o próximo bloco livre
            posicao_inicial_bloco_livre_encontrado = -1;
            tamanho_bloco_livre_encontrado = 0;
        }
    }

    // Se o loop terminou e nenhum bloco adequado foi encontrado em nenhuma das passagens
    fprintf(stderr, "ERRO (alocar_next_fit): Nao foi possivel alocar processo de tamanho %d. Memoria cheia ou muito fragmentada.\n",
            tamanho_processo);
    alocacoes_falhas++; // Incrementa contador de falhas
    return -1; // Falha na alocação
}

int alocar_best_fit(int tamanho_processo) {
    // 1. Validação inicial dos parâmetros
    if (tamanho_processo <= 0 || tamanho_processo > TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (alocar_best_fit): Tamanho de processo invalido: %d. Deve ser entre 1 e %d.\n",
                tamanho_processo, TOTAL_PIXELS);
        return -1;
    }
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar_best_fit): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        return -1;
    }

    // 2. Variáveis para acompanhar o MELHOR bloco encontrado até agora
    int best_fit_posicao_inicial = -1;
    int best_fit_tamanho = TOTAL_PIXELS + 1; // Inicializa com o maior valor possível para int

    // Variáveis temporárias para o bloco livre atual que está sendo analisado
    int posicao_inicial_bloco_livre_atual = -1;
    int tamanho_bloco_livre_atual = 0;
    int valor_pixel;

    // 3. Loop principal: Percorrer a memória pixel por pixel para encontrar todos os blocos livres
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        valor_pixel = ler_pixel(i); // Lê o estado do pixel na posição 'i'

        // Verifica se houve erro na leitura do pixel. Se sim, aborta.
        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_best_fit): Falha ao ler pixel na posicao %d. Abortando busca por bloco.\n", i);
            return -1;
        }

        if (valor_pixel == 255) { // Se o pixel atual é BRANCO (LIVRE)
            if (posicao_inicial_bloco_livre_atual == -1) {
                // Se estamos começando um novo bloco livre contíguo
                posicao_inicial_bloco_livre_atual = i;
            }
            tamanho_bloco_livre_atual++; // Incrementa o contador do tamanho do bloco livre atual
        } else { // Se o pixel atual é PRETO (OCUPADO), o bloco livre atual foi interrompido
            // Avalia o bloco livre que ACABOU de ser encontrado
            if (posicao_inicial_bloco_livre_atual != -1 && tamanho_bloco_livre_atual >= tamanho_processo) {
                // Se este bloco livre serve E é MENOR que o melhor encontrado até agora
                if (tamanho_bloco_livre_atual < best_fit_tamanho) {
                    best_fit_tamanho = tamanho_bloco_livre_atual;
                    best_fit_posicao_inicial = posicao_inicial_bloco_livre_atual;
                }
            }
            // Reseta as variáveis para procurar o próximo bloco livre
            posicao_inicial_bloco_livre_atual = -1;
            tamanho_bloco_livre_atual = 0;
        }
    }

    // 4. Verificação final: Após o loop, o último bloco livre (se houver) ainda precisa ser avaliado.
    // A lógica dentro do 'else' (pixel preto) só é ativada quando um bloco livre é interrompido.
    // Se a memória termina com um bloco livre, ele não teria sido avaliado.
    if (posicao_inicial_bloco_livre_atual != -1 && tamanho_bloco_livre_atual >= tamanho_processo) {
        if (tamanho_bloco_livre_atual < best_fit_tamanho) {
            best_fit_tamanho = tamanho_bloco_livre_atual;
            best_fit_posicao_inicial = posicao_inicial_bloco_livre_atual;
        }
    }

    // 5. Alocação (se um bloco adequado foi encontrado ao final da busca)
    if (best_fit_posicao_inicial == -1) {
        alocacoes_falhas++;
        return -1; // Falha na alocação
    } else {
        // Encontramos o MELHOR AJUSTE! Agora, marcamos os pixels como ocupados.
        for (int j = 0; j < tamanho_processo; j++) {
            escreve_pixel(best_fit_posicao_inicial + j, 0); // Marca como ocupado (preto)
        }
        return best_fit_posicao_inicial; // Retorna a posicao inicial da alocacao
    }
}

int alocar_worst_fit(int tamanho_processo) {

    // 1. Validação inicial dos parâmetros
    if (tamanho_processo <= 0 || tamanho_processo > TOTAL_PIXELS) {
        fprintf(stderr, "ERRO (alocar_worst_fit): Tamanho de processo invalido: %d. Deve ser entre 1 e %d.\n",
                tamanho_processo, TOTAL_PIXELS);
        return -1;
    }
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar_worst_fit): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        return -1;
    }

    // 2. Variáveis para acompanhar o MELHOR bloco (ou seja, o MAIOR que serve) encontrado até agora
    int worst_fit_posicao_inicial = -1;
    int worst_fit_tamanho = 0; // Inicializa com o menor valor possível para um tamanho (0)

    // Variáveis temporárias para o bloco livre atual que está sendo analisado
    int posicao_inicial_bloco_livre_atual = -1;
    int tamanho_bloco_livre_atual = 0;
    int valor_pixel;

    // 3. Loop principal: Percorrer a memória pixel por pixel para encontrar todos os blocos livres
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        valor_pixel = ler_pixel(i); // Lê o estado do pixel na posição 'i'

        // Verifica se houve erro na leitura do pixel. Se sim, aborta.
        if (valor_pixel == -1) {
            fprintf(stderr, "ERRO (alocar_worst_fit): Falha ao ler pixel na posicao %d. Abortando busca por bloco.\n", i);
            return -1;
        }

        if (valor_pixel == 255) { // Se o pixel atual é BRANCO (LIVRE)
            if (posicao_inicial_bloco_livre_atual == -1) {
                // Se estamos começando um novo bloco livre contíguo
                posicao_inicial_bloco_livre_atual = i;
            }
            tamanho_bloco_livre_atual++; // Incrementa o contador do tamanho do bloco livre atual
        } else { // Se o pixel atual é PRETO (OCUPADO), o bloco livre atual foi interrompido
            // Avalia o bloco livre que ACABOU de ser encontrado
            if (posicao_inicial_bloco_livre_atual != -1 && tamanho_bloco_livre_atual >= tamanho_processo) {
                // Se este bloco livre serve E é MAIOR que o maior encontrado até agora
                if (tamanho_bloco_livre_atual > worst_fit_tamanho) {
                    worst_fit_tamanho = tamanho_bloco_livre_atual;
                    worst_fit_posicao_inicial = posicao_inicial_bloco_livre_atual;
                }
            }
            // Reseta as variáveis para procurar o próximo bloco livre
            posicao_inicial_bloco_livre_atual = -1;
            tamanho_bloco_livre_atual = 0;
        }
    }

    // 4. Verificação final: Após o loop, o último bloco livre (se houver) ainda precisa ser avaliado.
    // A lógica dentro do 'else' (pixel preto) só é ativada quando um bloco livre é interrompido.
    // Se a memória termina com um bloco livre, ele não teria sido avaliado.
    if (posicao_inicial_bloco_livre_atual != -1 && tamanho_bloco_livre_atual >= tamanho_processo) {
        if (tamanho_bloco_livre_atual > worst_fit_tamanho) {
            worst_fit_tamanho = tamanho_bloco_livre_atual;
            worst_fit_posicao_inicial = posicao_inicial_bloco_livre_atual;
        }
    }

    // 5. Alocação (se um bloco adequado foi encontrado ao final da busca)
    if (worst_fit_posicao_inicial == -1) {
        alocacoes_falhas++;
        return -1; // Falha na alocação
    } else {
        // Encontramos o PIOR AJUSTE! Agora, marcamos os pixels como ocupados.
        for (int j = 0; j < tamanho_processo; j++) {
            escreve_pixel(worst_fit_posicao_inicial + j, 0); // Marca como ocupado (preto)
        }


        return worst_fit_posicao_inicial; // Retorna a posicao inicial da alocacao
    }
}

int alocar(int tipo_algoritmo, int tamanho_processo) {
    // Validações básicas (se necessário, adicione mais, como tamanho_processo > 0)
    if (saida_file == NULL) {
        fprintf(stderr, "ERRO (alocar): Arquivo de saida (memoria simulada) nao esta aberto. Abortando alocacao.\n");
        return -1;
    }

    // O switch/case para chamar o algoritmo correto
    switch (tipo_algoritmo) {
        case 1:
            return alocar_first_fit(tamanho_processo);
        case 2:
            return alocar_best_fit(tamanho_processo);
        case 3:
            return alocar_worst_fit(tamanho_processo);
        case 4:
            return alocar_next_fit(tamanho_processo);
        default:
            fprintf(stderr, "ERRO (alocar): Tipo de algoritmo invalido: %d. Esperado 1 a 4.\n", tipo_algoritmo);
            return -1; // Sinaliza erro por tipo de algoritmo inválido
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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>

// Definições de cores ANSI para terminal
#define RED    "\x1b[31m"
#define YELLOW "\x1b[33m"
#define GREEN  "\x1b[32m"
#define RESET  "\x1b[0m"

#define MAX 100
#define MAX_HIST 500 // Limite de registros no histórico
#define LIMITE_CRITICO 3
#define LIMITE_ATENCAO 7

typedef struct {
    int codigo;
    char nome[50];
    char categoria[30];
    float peso;
    int quantidade;
    float preco;
    int dia, mes, ano;
    int dias_restantes;
    int prioridade;
} Alimento;

// Nova estrutura para o histórico
typedef struct {
    int codigo_produto;
    char tipo_acao[15]; // Ex: "CADASTRO", "ENTRADA", "SAÍDA", "EXCLUSÃO"
    int quantidade;
    char data_hora[25]; // Formato: DD/MM/AAAA HH:MM:SS
} Movimentacao;

// Função para calcular dias restantes até a validade
int calcular_dias(int dia, int mes, int ano) {
    time_t agora = time(NULL);
    struct tm t = {0};
    t.tm_mday = dia;
    t.tm_mon = mes - 1;
    t.tm_year = ano - 1900;
    time_t validade = mktime(&t);
    double diff = difftime(validade, agora);
    return (int)(diff / 86400);
}

// Função para classificar a prioridade do alimento
void classificar(Alimento *a) {
    int dias = calcular_dias(a->dia, a->mes, a->ano);
    a->dias_restantes = dias;
    if(dias <= 0) a->prioridade = 0; // Vencido
    else if(dias <= LIMITE_CRITICO) a->prioridade = 1; // Crítico
    else if(dias <= LIMITE_ATENCAO) a->prioridade = 2; // Atenção
    else a->prioridade = 3; // OK
}

// NOVA FUNÇÃO: Registrar a movimentação
void registrar_movimentacao(Movimentacao hist[], int *total_mov, int cod, const char *tipo, int qtd) {
    if (*total_mov >= MAX_HIST) return; // Evita estourar o limite

    Movimentacao nova_mov;
    nova_mov.codigo_produto = cod;
    strcpy(nova_mov.tipo_acao, tipo);
    nova_mov.quantidade = qtd;

    // Captura data e hora atuais
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf(nova_mov.data_hora, "%02d/%02d/%04d %02d:%02d:%02d", 
            tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    hist[*total_mov] = nova_mov;
    (*total_mov)++;
}

// Função para cadastrar um novo alimento (Alterada para receber o histórico)
void cadastrar_alimento(Alimento estoque[], int *total, Movimentacao hist[], int *total_mov) {
    if (*total >= MAX) {
        printf("\n[ERRO] Estoque cheio!\n");
        return;
    }

    Alimento novo;
    novo.codigo = *total + 1;

    printf("\n--- Cadastro de Alimento (ID: %d) ---\n", novo.codigo);
    printf("Nome: ");
    scanf(" %[^\n]", novo.nome);
    printf("Categoria: ");
    scanf(" %[^\n]", novo.categoria);
    printf("Peso (kg): ");
    scanf("%f", &novo.peso);
    while (getchar() != '\n'); // limpa o buffer
    printf("Quantidade em estoque: ");
    scanf("%d", &novo.quantidade);
    while (getchar() != '\n'); // limpa o buffer
    printf("Preco unitario (R$): ");
    scanf("%f", &novo.preco);
    while (getchar() != '\n'); // limpa o buffer
    printf("Data de validade (DD MM AAAA): ");
    scanf("%d %d %d", &novo.dia, &novo.mes, &novo.ano);
    while (getchar() != '\n'); // limpa o buffer

    classificar(&novo);
    estoque[*total] = novo;
    (*total)++;
    
    // Registra a ação no histórico
    registrar_movimentacao(hist, total_mov, novo.codigo, "CADASTRO", novo.quantidade);
    
    printf("\n[SUCESSO] Produto '%s' adicionado!\n", novo.nome);
}

// Função para listar todos os produtos com cores no terminal
void listar(Alimento estoque[], int total) {
    if (total == 0) {
        printf("\n[AVISO] Nenhum alimento cadastrado.\n");
        return;
    }

    char *status[] = {"VENCIDO", "CRÍTICO", "ATENÇÃO", "OK"};

    printf("\n%-4s %-15s %-12s %-10s %-12s %-15s %-10s %s\n",
           "ID", "Nome", "Categoria", "Qtd", "Preco(R$)", "Validade", "Status", "Dias");
    printf("--------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < total; i++) {
        Alimento a = estoque[i];
        const char *color;
        switch (a.prioridade) {
            case 0: color = RED; break;
            case 1: color = YELLOW; break;
            case 2: color = YELLOW; break;
            case 3: color = GREEN; break;
            default: color = RESET;
        }
        printf("%-4d %-15s %-12s %-10d %-12.2f %02d/%02d/%04d [%s%-7s" RESET "] %d dias\n",
               a.codigo, a.nome, a.categoria, a.quantidade, a.preco,
               a.dia, a.mes, a.ano, color, status[a.prioridade], a.dias_restantes);
    }
}

// Função para buscar um produto
void buscar_produto(Alimento estoque[], int total) {
    if (total == 0) {
        printf("\n[AVISO] O estoque está vazio! Cadastre algo primeiro.\n");
        return;
    }

    int escolha;
    printf("\n--- LOCALIZAR PRODUTO ---");
    printf("\n1. Buscar por CODIGO");
    printf("\n2. Buscar por NOME");
    printf("\nEscolha: ");
    scanf("%d", &escolha);
    while (getchar() != '\n'); // limpa o buffer

    if (escolha == 1) {
        int codigo_busca;
        printf("Digite o codigo: ");
        scanf("%d", &codigo_busca);
        while (getchar() != '\n'); // limpa o buffer
        for (int i = 0; i < total; i++) {
            if (estoque[i].codigo == codigo_busca) {
                printf("\n[ACHOU!] O produto '%s' (R$ %.2f) está na categoria: [%s]\n",
                       estoque[i].nome, estoque[i].preco, estoque[i].categoria);
                return;
            }
        }
    } else if (escolha == 2) {
        char nome_busca[50];
        printf("Digite o nome exato: ");
        scanf(" %[^\n]", nome_busca);
        for (int i = 0; i < total; i++) {
            if (strcmp(estoque[i].nome, nome_busca) == 0) {
                printf("\n[ACHOU!] O produto ID %d (R$ %.2f) está na categoria: [%s]\n",
                       estoque[i].codigo, estoque[i].preco, estoque[i].categoria);
                return;
            }
        }
    }
    printf("\n[ERRO] Produto não localizado no sistema.\n");
}

// Função para salvar em TXT
void salvar_estoque(Alimento estoque[], int total) {
    FILE *arquivo = fopen("estoque.txt", "w");
    if (arquivo == NULL) {
        printf("\n[Erro] Não foi possível abrir o arquivo.\n");
        return;
    }
    
    // Escreve o cabeçalho no formato correto (separado por vírgulas)
    fprintf(arquivo, "ID,Nome,Categoria,Peso,Qtd,Preco,Validade,Dias,Prioridade\n");

    // Escreve cada alimento (Adicionado o a.peso)
    for (int i = 0; i < total; i++) {
        Alimento a = estoque[i];
        fprintf(arquivo, "%d,%s,%s,%.2f,%d,%.2f,%02d/%02d/%04d,%d,%d\n",
                a.codigo, a.nome, a.categoria, a.peso, a.quantidade, a.preco,
                a.dia, a.mes, a.ano, a.dias_restantes, a.prioridade);
    }
    
    fclose(arquivo);
    printf("\n[SUCESSO] Arquivo salvo com sucesso em 'estoque.txt'.\n");
}

// função para excluir produtos (Alterada para receber o histórico)
void excluir_produtos(Alimento estoque[], int *total, Movimentacao hist[], int *total_mov){
    if(*total == 0){
        printf("\n O estoque esta vazio! Nada para excluir.\n");
        return;
    }
    int codigo;
    printf("\n Digite o código do produto a ser excluido: ");
    scanf("%d", &codigo);
    while (getchar() != '\n');

    for(int i = 0; i < *total; i++){
        if (estoque[i].codigo == codigo){
            int qtd_excluida = estoque[i].quantidade; // Guarda a qtd antes de apagar

            // Desloca todos os elementos após o encontrado
            for(int j = i; j < *total - 1; j++){
                estoque[j] = estoque[j + 1];
            }
            (*total)--;
            
            // Registra a exclusão no histórico
            registrar_movimentacao(hist, total_mov, codigo, "EXCLUSÃO", qtd_excluida);
            
            printf("\n Produto com ID %d foi excluido. \n", codigo);
            return;
        }
    }
    printf("\n produto com ID %d não encontrado. \n", codigo);
}

// NOVA FUNÇÃO: Dar entrada ou saída de estoque de um produto existente
void atualizar_estoque(Alimento estoque[], int total, Movimentacao hist[], int *total_mov) {
    if (total == 0) {
        printf("\n[AVISO] O estoque está vazio! Cadastre um produto primeiro.\n");
        return;
    }

    int cod, qtd, escolha, indice = -1;
    printf("\n--- MOVIMENTAR ESTOQUE (ENTRADA/SAÍDA) ---\n");
    printf("Digite o ID do produto: ");
    scanf("%d", &cod);
    while (getchar() != '\n');

    // Busca o índice do produto
    for (int i = 0; i < total; i++) {
        if (estoque[i].codigo == cod) {
            indice = i;
            break;
        }
    }

    if (indice == -1) {
        printf("[ERRO] Produto com ID %d não encontrado.\n", cod);
        return;
    }

    printf("Produto: %s | Estoque atual: %d\n", estoque[indice].nome, estoque[indice].quantidade);
    printf("1. Registrar ENTRADA (+)\n");
    printf("2. Registrar SAIDA (-)\n");
    printf("Escolha: ");
    scanf("%d", &escolha);
    printf("Quantidade: ");
    scanf("%d", &qtd);
    while (getchar() != '\n');

    if (escolha == 1) {
        estoque[indice].quantidade += qtd;
        registrar_movimentacao(hist, total_mov, cod, "ENTRADA", qtd);
        printf("[SUCESSO] Entrada registrada! Novo saldo: %d\n", estoque[indice].quantidade);
    } else if (escolha == 2) {
        if (qtd > estoque[indice].quantidade) {
            printf("[ERRO] Quantidade insuficiente em estoque!\n");
        } else {
            estoque[indice].quantidade -= qtd;
            registrar_movimentacao(hist, total_mov, cod, "SAÍDA", qtd);
            printf("[SUCESSO] Saída registrada! Novo saldo: %d\n", estoque[indice].quantidade);
        }
    } else {
        printf("[ERRO] Opção inválida.\n");
    }
}

// NOVA FUNÇÃO: Consultar auditoria
void consultar_historico(Movimentacao hist[], int total_mov) {
    if (total_mov == 0) {
        printf("\n[AVISO] Nenhuma movimentação registrada no histórico.\n");
        return;
    }

    printf("\n================= HISTÓRICO DE MOVIMENTAÇÕES (AUDITORIA) =================\n");
    printf("%-22s | %-10s | %-12s | %-5s\n", "Data/Hora", "ID Produto", "Ação", "Qtd");
    printf("--------------------------------------------------------------------------\n");
    for (int i = 0; i < total_mov; i++) {
        printf("%-22s | %-10d | %-12s | %-5d\n", 
               hist[i].data_hora, hist[i].codigo_produto, hist[i].tipo_acao, hist[i].quantidade);
    }
    printf("==========================================================================\n");
}

// função para carregar o estoque (Corrigido o ponteiro *total)
void carregar_estoque(Alimento estoque[], int *total) {
    FILE *arquivo = fopen("estoque.txt", "r"); 
    
    if (arquivo == NULL) {
        printf("\n[AVISO] Arquivo 'estoque.txt' não encontrado. Iniciando estoque vazio.\n");
        return;
    }

    char cabecalho[256];
    // Lê e ignora a primeira linha (cabeçalho)
    if (fgets(cabecalho, sizeof(cabecalho), arquivo) == NULL) {
        fclose(arquivo);
        return;
    }

    *total = 0; 
    Alimento a;

    // O fscanf agora lê as 11 variáveis exatas que foram salvas (incluindo o %f do peso)
    while (fscanf(arquivo, "%d,%[^,],%[^,],%f,%d,%f,%d/%d/%d,%d,%d\n",
                  &a.codigo, a.nome, a.categoria, &a.peso, &a.quantidade, &a.preco,
                  &a.dia, &a.mes, &a.ano, &a.dias_restantes, &a.prioridade) == 11) {
        
        estoque[*total] = a;
        (*total)++;
        
        if (*total >= MAX) {
            break;
        }
    }

    fclose(arquivo);
    printf("\n[SUCESSO] Estoque carregado! %d produtos encontrados.\n", *total);
}
// Função principal
int main() {
    setlocale(LC_ALL, "portuguese");
    Alimento lista_estoque[MAX];
    int total_produtos = 0;
    
    // Variáveis do Histórico
    Movimentacao historico[MAX_HIST];
    int total_movimentacoes = 0;

    carregar_estoque(lista_estoque, &total_produtos);
    int opcao;

    do {
        printf("\n===== CONTROLE DE ESTOQUE MENU =====\n");
        printf("1. Cadastrar Produto\n");
        printf("2. Listar Todos os Produtos\n");
        printf("3. Localizar Produto (Busca)\n");
        printf("4. Salvar arquivo em .TXT\n");
        printf("5. Excluir Produtos\n");
        printf("6. Movimentar Estoque (Entrada/Saída)\n"); // NOVO
        printf("7. Consultar Histórico (Auditoria)\n"); // NOVO
        printf("0. Sair\n");
        printf("Opcao: ");
        scanf("%d", &opcao);
        while (getchar() != '\n'); // limpa o buffer

        if (opcao == 1) cadastrar_alimento(lista_estoque, &total_produtos, historico, &total_movimentacoes);
        else if (opcao == 2) listar(lista_estoque, total_produtos);
        else if (opcao == 3) buscar_produto(lista_estoque, total_produtos);
        else if (opcao == 4) salvar_estoque(lista_estoque, total_produtos);
        else if (opcao == 5) excluir_produtos(lista_estoque, &total_produtos, historico, &total_movimentacoes);       
        else if (opcao == 6) atualizar_estoque(lista_estoque, total_produtos, historico, &total_movimentacoes);
        else if (opcao == 7) consultar_historico(historico, total_movimentacoes);
        else if (opcao != 0) printf("\n[ERRO] Opcao invalida!\n");

    } while (opcao != 0);

    printf("\nEncerrando o sistema. Voce tem %d produtos no estoque.\n", total_produtos);

    return 0;
}

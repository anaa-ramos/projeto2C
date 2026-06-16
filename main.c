/*
 * ============================================================
 *  MERCADINHO DO PORTO — Sistema Integrado
 *  Módulos: Cliente | Produto/Estoque | Pedido/Movimentação
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <locale.h>

/* ==================== CORES ANSI ========================== */
#define RED    "\x1b[31m"
#define YELLOW "\x1b[33m"
#define GREEN  "\x1b[32m"
#define CYAN   "\x1b[36m"
#define BOLD   "\x1b[1m"
#define RESET  "\x1b[0m"

/* ==================== VERSÃO ============================== */
#define VERSAO "1.0.0"

/* ==================== CONSTANTES — CLIENTE ================ */
#define MAX_NOME_CLI   80
#define MAX_CPF        15
#define MAX_EMAIL      60
#define MAX_TELEFONE   20
#define MAX_ENDERECO  100
#define MAX_CIDADE     50
#define MAX_ESTADO      3
#define MAX_CEP        10
#define MAX_OBS_CLI   200
#define ARQ_CLIENTES  "clientes.dat"
#define ARQ_CLI_TXT   "clientes_export.txt"

/* ==================== CONSTANTES — PRODUTO ================ */
#define MAX_NOME_PROD  60
#define MAX_CATEGORIA  35
#define MAX_UNIDADE    10
#define MAX_OBS_PROD  200
#define ARQ_PRODUTOS  "produtos.dat"
#define ARQ_HIST      "historico.dat"
#define ARQ_PROD_TXT  "produtos_export.txt"
#define LIMITE_CRITICO 3
#define LIMITE_ATENCAO 7

/* ==================== CONSTANTES — PEDIDO ================= */
#define MAX_ITENS_PEDIDO 20
#define MAX_PEDIDOS      200
#define ARQ_PEDIDOS      "pedidos.dat"

/* ==================== ESTRUTURA — CLIENTE ================= */
typedef struct {
    int   codigo;
    char  nome[MAX_NOME_CLI];
    char  cpf[MAX_CPF];
    char  rg[MAX_CPF];
    char  data_nascimento[12];
    char  email[MAX_EMAIL];
    char  telefone[MAX_TELEFONE];
    char  celular[MAX_TELEFONE];
    char  endereco[MAX_ENDERECO];
    char  numero[10];
    char  complemento[40];
    char  bairro[50];
    char  cidade[MAX_CIDADE];
    char  estado[MAX_ESTADO];
    char  cep[MAX_CEP];
    float limite_credito;
    int   tipo;   /* 1=PF  2=PJ  */
    int   ativo;  /* 1=ativo 0=inativo */
    char  data_cadastro[20];
    char  observacoes[MAX_OBS_CLI];
} Cliente;

/* ==================== ESTRUTURA — PRODUTO ================= */
typedef struct {
    int   codigo;
    char  nome[MAX_NOME_PROD];
    char  categoria[MAX_CATEGORIA];
    char  unidade[MAX_UNIDADE];
    float peso_volume;
    int   quantidade;
    float preco_custo;
    float preco_venda;
    int   dia_val, mes_val, ano_val;
    int   dias_restantes;
    int   prioridade; /* 0=Vencido 1=Crítico 2=Atenção 3=OK */
    int   ativo;
    char  data_cadastro[20];
    char  observacoes[MAX_OBS_PROD];
} Produto;

/* ==================== ESTRUTURA — MOVIMENTAÇÃO ============ */
typedef struct {
    int  codigo_produto;
    char tipo_acao[15]; /* CADASTRO | ENTRADA | SAIDA | EXCLUSAO | EDICAO */
    int  quantidade;
    char data_hora[25];
} Movimentacao;

/* ==================== ESTRUTURA — ITEM DE PEDIDO ========== */
typedef struct {
    int   codigo_produto;
    char  nome_produto[MAX_NOME_PROD];
    int   quantidade;
    float preco_unitario;
    float subtotal;
} ItemPedido;

/* ==================== ESTRUTURA — PEDIDO ================== */
typedef struct {
    int        numero;
    int        codigo_cliente;
    char       nome_cliente[MAX_NOME_CLI];
    ItemPedido itens[MAX_ITENS_PEDIDO];
    int        total_itens;
    float      total;
    float      desconto;
    float      total_final;
    char       forma_pagamento[20];
    char       data_hora[25];
    int        status; /* 0=Cancelado 1=Aberto 2=Finalizado */
} Pedido;

/* ============================================================
 *  UTILITÁRIOS COMPARTILHADOS
 * ============================================================ */

void limpar_tela(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pausar(void) {
    printf("\n Pressione ENTER para continuar...");
    getchar();
}

void limpar_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void ler_string(const char *prompt, char *dest, int max) {
    printf("%s", prompt);
    fgets(dest, max, stdin);
    int len = strlen(dest);
    if (len > 0 && dest[len - 1] == '\n')
        dest[len - 1] = '\0';
}

void data_hoje(char *buf) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, 20, "%d/%m/%Y %H:%M", tm_info);
}

void data_hora_agora(char *buf) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, 25, "%d/%m/%Y %H:%M:%S", tm_info);
}

static void sep_txt(FILE *f, char c, int n) {
    for (int i = 0; i < n; i++) fputc(c, f);
    fputc('\n', f);
}

/* Lê somente dígitos numéricos com quantidade EXATA de caracteres.
   Se digitos_exatos == 0, aceita qualquer quantidade (campo livre).    */
void ler_numeros_exatos(const char *prompt, char *dest, int max, int digitos_exatos) {
    char buf[256];
    while (1) {
        printf("%s", prompt);
        fgets(buf, sizeof(buf), stdin);
        int len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[--len] = '\0';

        if (len == 0) {
            printf(" Campo obrigatorio! Digite apenas numeros.\n");
            continue;
        }

        int valido = 1;
        for (int i = 0; i < len; i++) {
            if (!isdigit((unsigned char)buf[i])) { valido = 0; break; }
        }
        if (!valido) {
            printf(" Apenas numeros sao permitidos neste campo!\n");
            continue;
        }

        if (digitos_exatos > 0 && len != digitos_exatos) {
            printf(" Quantidade incorreta! Este campo exige exatamente %d digito(s). "
                   "Voce digitou %d.\n", digitos_exatos, len);
            continue;
        }

        strncpy(dest, buf, max - 1);
        dest[max - 1] = '\0';
        return;
    }
}

/* Lê um inteiro >= 0, rejeitando negativos e texto */
int ler_inteiro_positivo(const char *prompt) {
    int val;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &val) == 1) {
            limpar_buffer();
            if (val >= 0) return val;
            printf(" Valor nao pode ser negativo!\n");
        } else {
            limpar_buffer();
            printf(" Entrada invalida! Digite um numero inteiro.\n");
        }
    }
}

/* Lê um float > 0, rejeitando negativos e texto */
float ler_float_positivo(const char *prompt) {
    float val;
    while (1) {
        printf("%s", prompt);
        if (scanf("%f", &val) == 1) {
            limpar_buffer();
            if (val >= 0.0f) return val;
            printf(" Valor nao pode ser negativo!\n");
        } else {
            limpar_buffer();
            printf(" Entrada invalida! Digite um numero.\n");
        }
    }
}

/* Lê e valida data DD MM AAAA — rejeita datas no passado */
void ler_data_validade(int *dia, int *mes, int *ano) {
    while (1) {
        printf(" Data de validade (DD MM AAAA): ");
        if (scanf("%d %d %d", dia, mes, ano) != 3) {
            limpar_buffer();
            printf(" Formato invalido! Use DD MM AAAA (ex: 25 12 2026).\n");
            continue;
        }
        limpar_buffer();

        /* Verifica intervalo básico */
        if (*dia < 1 || *dia > 31 || *mes < 1 || *mes > 12 || *ano < 2000) {
            printf(" Data invalida! Verifique dia (1-31), mes (1-12) e ano (>= 2000).\n");
            continue;
        }

        /* Verifica se a data é futura usando mktime */
        struct tm t = {0};
        t.tm_mday  = *dia;
        t.tm_mon   = *mes - 1;
        t.tm_year  = *ano - 1900;
        t.tm_hour  = 23; t.tm_min = 59; t.tm_sec = 59;
        time_t val = mktime(&t);

        /* mktime normaliza — se dia/mes ficaram iguais, data era válida */
        if (t.tm_mday != *dia || t.tm_mon != (*mes - 1)) {
            printf(" Data invalida! Esse dia nao existe nesse mes.\n");
            continue;
        }

        if (difftime(val, time(NULL)) < 0) {
            printf(" " YELLOW "Atencao: data já passou — produto sera marcado como VENCIDO." RESET "\n");
            printf(" Deseja usar essa data mesmo assim? [S/N]: ");
            char c; scanf(" %c", &c); limpar_buffer();
            if (toupper(c) != 'S') continue;
        }
        return;
    }
}

void formatar_cpf(const char *entrada, char *saida) {
    char digits[12] = {0};
    int j = 0;
    for (int i = 0; entrada[i] && j < 11; i++)
        if (isdigit((unsigned char)entrada[i]))
            digits[j++] = entrada[i];
    if (j == 11)
        sprintf(saida, "%c%c%c.%c%c%c.%c%c%c-%c%c",
                digits[0],digits[1],digits[2],
                digits[3],digits[4],digits[5],
                digits[6],digits[7],digits[8],
                digits[9],digits[10]);
    else
        strcpy(saida, entrada);
}

/* ============================================================
 *  MÓDULO CLIENTE
 * ============================================================ */

int cli_proximo_codigo(void) {
    FILE *f = fopen(ARQ_CLIENTES, "rb");
    if (!f) return 1;
    int max = 0;
    Cliente c;
    while (fread(&c, sizeof(Cliente), 1, f) == 1)
        if (c.codigo > max) max = c.codigo;
    fclose(f);
    return max + 1;
}

int cli_total_registros(void) {
    FILE *f = fopen(ARQ_CLIENTES, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fclose(f);
    return (int)(tam / sizeof(Cliente));
}

int cli_buscar_por_codigo(int codigo, Cliente *dest) {
    FILE *f = fopen(ARQ_CLIENTES, "rb");
    if (!f) return 0;
    Cliente c;
    while (fread(&c, sizeof(Cliente), 1, f) == 1) {
        if (c.codigo == codigo) {
            *dest = c;
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void cli_exibir_ficha(const Cliente *c) {
    printf("\n ╔══════════════════════════════════════════════════════╗\n");
    printf(" ║ FICHA DO CLIENTE — Cód: %-5d Status: %-8s  ║\n",
           c->codigo, c->ativo ? "ATIVO" : "INATIVO");
    printf(" ╠══════════════════════════════════════════════════════╣\n");
    printf(" ║ Nome       : %-38s ║\n", c->nome);
    printf(" ║ Tipo       : %-38s ║\n", c->tipo == 1 ? "Pessoa Física" : "Pessoa Jurídica");
    printf(" ║ CPF/CNPJ   : %-38s ║\n", c->cpf);
    printf(" ║ RG/IE      : %-38s ║\n", c->rg);
    if (c->tipo == 1)
        printf(" ║ Nascimento : %-38s ║\n", c->data_nascimento);
    printf(" ╠══════════════════════════════════════════════════════╣\n");
    printf(" ║ E-mail     : %-38s ║\n", c->email);
    printf(" ║ Telefone   : %-38s ║\n", c->telefone);
    printf(" ║ Celular    : %-38s ║\n", c->celular);
    printf(" ╠══════════════════════════════════════════════════════╣\n");
    printf(" ║ Endereço   : %-38s ║\n", c->endereco);
    printf(" ║ Nº/Compl.  : %-10s / %-25s ║\n", c->numero, c->complemento);
    printf(" ║ Bairro     : %-38s ║\n", c->bairro);
    printf(" ║ Cidade/UF  : %-30s / %-5s ║\n", c->cidade, c->estado);
    printf(" ║ CEP        : %-38s ║\n", c->cep);
    printf(" ╠══════════════════════════════════════════════════════╣\n");
    printf(" ║ Lim. Cred. : R$ %-34.2f ║\n", c->limite_credito);
    printf(" ║ Cadastrado : %-38s ║\n", c->data_cadastro);
    if (strlen(c->observacoes) > 0)
        printf(" ║ Obs.       : %-38s ║\n", c->observacoes);
    printf(" ╚══════════════════════════════════════════════════════╝\n");
}

void cadastrar_cliente(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║       CADASTRO DE NOVO CLIENTE           ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");

    Cliente c;
    memset(&c, 0, sizeof(Cliente));
    c.codigo = cli_proximo_codigo();
    c.ativo  = 1;
    data_hoje(c.data_cadastro);

    printf(" Código gerado automaticamente: %d\n\n", c.codigo);

    printf(" Tipo de cliente:\n [1] Pessoa Física\n [2] Pessoa Jurídica\n Opção: ");
    scanf("%d", &c.tipo);
    limpar_buffer();
    if (c.tipo != 1 && c.tipo != 2) c.tipo = 1;

    printf("\n --- DADOS PESSOAIS ---\n");
    do {
        ler_string(" Nome completo        : ", c.nome, MAX_NOME_CLI);
        if (strlen(c.nome) == 0) printf(" Nome e obrigatorio!\n");
    } while (strlen(c.nome) == 0);

    if (c.tipo == 1) {
        char cpf_raw[MAX_CPF];
        ler_numeros_exatos(" CPF (11 digitos, somente numeros)          : ", cpf_raw, MAX_CPF, 11);
        formatar_cpf(cpf_raw, c.cpf);
        ler_numeros_exatos(" RG  (7 a 9 digitos — ex: 1234567)          : ", c.rg, MAX_CPF, 0);
        ler_numeros_exatos(" Data nasc. (8 digitos DDMMAAAA, ex:01011990): ", c.data_nascimento, 12, 8);
    } else {
        ler_numeros_exatos(" CNPJ (14 digitos, somente numeros)          : ", c.cpf, MAX_CPF, 14);
        ler_numeros_exatos(" Inscricao Estadual (somente numeros)        : ", c.rg, MAX_CPF, 0);
    }

    printf("\n --- CONTATO ---\n");
    ler_string(" E-mail   : ", c.email, MAX_EMAIL);
    ler_numeros_exatos(" Telefone fixo (10 digitos, ex: 8133334444): ", c.telefone, MAX_TELEFONE, 10);
    ler_numeros_exatos(" Celular   (11 digitos, ex: 81988887777)   : ", c.celular,  MAX_TELEFONE, 11);

    printf("\n --- ENDEREÇO ---\n");
    do {
        ler_string(" Logradouro  : ", c.endereco, MAX_ENDERECO);
        if (strlen(c.endereco) == 0) printf(" Logradouro e obrigatorio!\n");
    } while (strlen(c.endereco) == 0);
    ler_string(" Número      : ", c.numero,      10);
    ler_string(" Complemento : ", c.complemento, 40);
    do {
        ler_string(" Bairro      : ", c.bairro, 50);
        if (strlen(c.bairro) == 0) printf(" Bairro e obrigatorio!\n");
    } while (strlen(c.bairro) == 0);
    do {
        ler_string(" Cidade      : ", c.cidade, MAX_CIDADE);
        if (strlen(c.cidade) == 0) printf(" Cidade e obrigatoria!\n");
    } while (strlen(c.cidade) == 0);
    do {
        ler_string(" Estado (UF) : ", c.estado, MAX_ESTADO);
        if (strlen(c.estado) == 0) printf(" Estado e obrigatorio!\n");
    } while (strlen(c.estado) == 0);
    ler_numeros_exatos(" CEP (8 digitos, somente numeros, ex: 50000000): ", c.cep, MAX_CEP, 8);

    printf("\n --- FINANCEIRO ---\n");
    c.limite_credito = ler_float_positivo(" Limite de credito (R$): ");

    printf("\n --- OBSERVAÇÕES ---\n");
    ler_string(" Observações: ", c.observacoes, MAX_OBS_CLI);

    printf("\n Confirmar cadastro? [S/N]: ");
    char conf;
    scanf(" %c", &conf);
    limpar_buffer();

    if (toupper(conf) == 'S') {
        FILE *f = fopen(ARQ_CLIENTES, "ab");
        if (!f) { printf(" ERRO ao abrir arquivo!\n"); pausar(); return; }
        fwrite(&c, sizeof(Cliente), 1, f);
        fclose(f);
        printf("\n " GREEN "✔ Cliente cadastrado! Código: %d" RESET "\n", c.codigo);
    } else {
        printf("\n Cadastro cancelado.\n");
    }
    pausar();
}

void listar_clientes(int apenas_ativos) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════════════════════════╗\n");
    printf(" ║ LISTA DE CLIENTES%-42s║\n", apenas_ativos ? " (ATIVOS) " : " ");
    printf(" ╠═══════╦══════════════════════════════╦══════════════╦═════════╣\n");
    printf(" ║ CÓD.  ║ NOME                         ║ CPF/CNPJ     ║ STATUS  ║\n");
    printf(" ╠═══════╬══════════════════════════════╬══════════════╬═════════╣\n");

    FILE *f = fopen(ARQ_CLIENTES, "rb");
    if (!f) {
        printf(" ║ Nenhum registro encontrado.                                  ║\n");
        printf(" ╚══════════════════════════════════════════════════════════════╝\n");
        pausar(); return;
    }

    Cliente c; int count = 0;
    while (fread(&c, sizeof(Cliente), 1, f) == 1) {
        if (apenas_ativos && !c.ativo) continue;
        char nome_t[31]; strncpy(nome_t, c.nome, 30); nome_t[30] = '\0';
        printf(" ║ %-5d ║ %-28s ║ %-12s ║ %-7s ║\n",
               c.codigo, nome_t, c.cpf, c.ativo ? "ATIVO" : "INATIVO");
        count++;
    }
    fclose(f);
    printf(" ╚═══════╩══════════════════════════════╩══════════════╩═════════╝\n");
    printf(" Total: %d cliente(s).\n", count);
    pausar();
}

void consultar_cliente(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║         CONSULTA DE CLIENTE              ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");
    printf(" Buscar por:\n [1] Código\n [2] Nome\n [3] CPF/CNPJ\n Opção: ");
    int op; scanf("%d", &op); limpar_buffer();

    FILE *f = fopen(ARQ_CLIENTES, "rb");
    if (!f) { printf("\n Nenhum registro encontrado.\n"); pausar(); return; }

    int encontrou = 0; Cliente c;
    if (op == 1) {
        printf(" Código: "); int cod; scanf("%d", &cod); limpar_buffer();
        while (fread(&c, sizeof(Cliente), 1, f) == 1)
            if (c.codigo == cod) { cli_exibir_ficha(&c); encontrou = 1; }
    } else if (op == 2) {
        char busca[MAX_NOME_CLI];
        ler_string(" Nome (parte): ", busca, MAX_NOME_CLI);
        char b_low[MAX_NOME_CLI];
        for (int i = 0; busca[i]; i++) b_low[i] = tolower((unsigned char)busca[i]);
        b_low[strlen(busca)] = '\0';
        while (fread(&c, sizeof(Cliente), 1, f) == 1) {
            char n_low[MAX_NOME_CLI];
            for (int i = 0; c.nome[i]; i++) n_low[i] = tolower((unsigned char)c.nome[i]);
            n_low[strlen(c.nome)] = '\0';
            if (strstr(n_low, b_low)) { cli_exibir_ficha(&c); encontrou = 1; }
        }
    } else if (op == 3) {
        char busca[MAX_CPF]; ler_string(" CPF/CNPJ: ", busca, MAX_CPF);
        while (fread(&c, sizeof(Cliente), 1, f) == 1)
            if (strstr(c.cpf, busca)) { cli_exibir_ficha(&c); encontrou = 1; }
    }
    fclose(f);
    if (!encontrou) printf("\n Nenhum cliente encontrado.\n");
    pausar();
}

void editar_cliente(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║            EDITAR CLIENTE                ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");
    printf(" Código do cliente: "); int cod; scanf("%d", &cod); limpar_buffer();

    Cliente c;
    if (!cli_buscar_por_codigo(cod, &c)) { printf(" Cliente não encontrado!\n"); pausar(); return; }
    cli_exibir_ficha(&c);

    printf("\n [1] Nome  [2] CPF/CNPJ  [3] Contato  [4] Endereço\n");
    printf(" [5] Limite de crédito  [6] Observações  [7] Status\n");
    printf(" [0] Cancelar\n Opção: ");
    int op; scanf("%d", &op); limpar_buffer();

    switch (op) {
        case 1: ler_string(" Novo nome: ", c.nome, MAX_NOME_CLI); break;
        case 2:
            if (c.tipo == 1) {
                char cpf_raw[MAX_CPF];
                ler_numeros_exatos(" Novo CPF (11 digitos): ", cpf_raw, MAX_CPF, 11);
                formatar_cpf(cpf_raw, c.cpf);
            } else {
                ler_numeros_exatos(" Novo CNPJ (14 digitos): ", c.cpf, MAX_CPF, 14);
            }
            break;
        case 3:
            ler_string(" Novo e-mail  : ", c.email, MAX_EMAIL);
            ler_numeros_exatos(" Novo telefone fixo (10 digitos): ", c.telefone, MAX_TELEFONE, 10);
            ler_numeros_exatos(" Novo celular       (11 digitos): ", c.celular,  MAX_TELEFONE, 11);
            break;
        case 4:
            ler_string(" Logradouro  : ", c.endereco,    MAX_ENDERECO);
            ler_string(" Número      : ", c.numero,      10);
            ler_string(" Complemento : ", c.complemento, 40);
            ler_string(" Bairro      : ", c.bairro,      50);
            ler_string(" Cidade      : ", c.cidade,      MAX_CIDADE);
            ler_string(" Estado (UF) : ", c.estado,      MAX_ESTADO);
            ler_string(" CEP         : ", c.cep,         MAX_CEP);
            break;
        case 5:
            printf(" Novo limite (R$): ");
            scanf("%f", &c.limite_credito); limpar_buffer();
            break;
        case 6: ler_string(" Observações: ", c.observacoes, MAX_OBS_CLI); break;
        case 7:
            c.ativo = !c.ativo;
            printf(" Status: %s\n", c.ativo ? "ATIVO" : "INATIVO");
            break;
        default: printf(" Cancelado.\n"); pausar(); return;
    }

    FILE *f = fopen(ARQ_CLIENTES, "r+b");
    if (!f) { printf(" ERRO ao abrir arquivo!\n"); pausar(); return; }
    Cliente tmp;
    while (fread(&tmp, sizeof(Cliente), 1, f) == 1) {
        if (tmp.codigo == cod) {
            fseek(f, -(long)sizeof(Cliente), SEEK_CUR);
            fwrite(&c, sizeof(Cliente), 1, f);
            break;
        }
    }
    fclose(f);
    printf("\n " GREEN "✔ Dados atualizados!" RESET "\n");
    pausar();
}

void excluir_cliente(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║      EXCLUIR / INATIVAR CLIENTE          ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");
    printf(" Código do cliente: "); int cod; scanf("%d", &cod); limpar_buffer();

    Cliente c;
    if (!cli_buscar_por_codigo(cod, &c)) { printf(" Cliente não encontrado!\n"); pausar(); return; }
    cli_exibir_ficha(&c);

    printf("\n [1] Inativar (recomendado)  [2] Excluir definitivamente  [0] Cancelar\n Opção: ");
    int op; scanf("%d", &op); limpar_buffer();

    if (op == 1) {
        c.ativo = 0;
        FILE *f = fopen(ARQ_CLIENTES, "r+b");
        Cliente tmp;
        while (fread(&tmp, sizeof(Cliente), 1, f) == 1) {
            if (tmp.codigo == cod) {
                fseek(f, -(long)sizeof(Cliente), SEEK_CUR);
                fwrite(&c, sizeof(Cliente), 1, f);
                break;
            }
        }
        fclose(f);
        printf("\n " YELLOW "✔ Cliente inativado." RESET "\n");
    } else if (op == 2) {
        printf(" Confirma exclusão definitiva? [S/N]: ");
        char conf; scanf(" %c", &conf); limpar_buffer();
        if (toupper(conf) == 'S') {
            FILE *in  = fopen(ARQ_CLIENTES,    "rb");
            FILE *out = fopen("clientes_tmp.dat", "wb");
            if (!in || !out) {
                printf(" ERRO!\n");
                if (in)  fclose(in);
                if (out) fclose(out);
                pausar(); return;
            }
            Cliente tmp;
            while (fread(&tmp, sizeof(Cliente), 1, in) == 1)
                if (tmp.codigo != cod) fwrite(&tmp, sizeof(Cliente), 1, out);
            fclose(in); fclose(out);
            remove(ARQ_CLIENTES);
            rename("clientes_tmp.dat", ARQ_CLIENTES);
            printf("\n " RED "✔ Cliente excluído." RESET "\n");
        } else printf(" Cancelado.\n");
    }
    pausar();
}

/* --- Grava a ficha completa de um cliente no arquivo .txt --- */
static void cli_ficha_para_txt(FILE *f, const Cliente *c) {
    sep_txt(f, '=', 62);
    fprintf(f, " FICHA DO CLIENTE\n");
    fprintf(f, " Codigo : %d\n", c->codigo);
    fprintf(f, " Status : %s\n", c->ativo ? "ATIVO" : "INATIVO");
    sep_txt(f, '-', 62);
    fprintf(f, " DADOS PESSOAIS\n");
    fprintf(f, " Nome       : %s\n", c->nome);
    fprintf(f, " Tipo       : %s\n", c->tipo == 1 ? "Pessoa Fisica" : "Pessoa Juridica");
    fprintf(f, " CPF/CNPJ   : %s\n", c->cpf);
    fprintf(f, " RG/IE      : %s\n", c->rg);
    if (c->tipo == 1)
        fprintf(f, " Nascimento : %s\n", c->data_nascimento);
    sep_txt(f, '-', 62);
    fprintf(f, " CONTATO\n");
    fprintf(f, " E-mail     : %s\n", c->email);
    fprintf(f, " Telefone   : %s\n", c->telefone);
    fprintf(f, " Celular    : %s\n", c->celular);
    sep_txt(f, '-', 62);
    fprintf(f, " ENDERECO\n");
    fprintf(f, " Logradouro : %s, %s\n", c->endereco, c->numero);
    if (strlen(c->complemento) > 0)
        fprintf(f, " Complemento: %s\n", c->complemento);
    fprintf(f, " Bairro     : %s\n", c->bairro);
    fprintf(f, " Cidade/UF  : %s / %s\n", c->cidade, c->estado);
    fprintf(f, " CEP        : %s\n", c->cep);
    sep_txt(f, '-', 62);
    fprintf(f, " FINANCEIRO\n");
    fprintf(f, " Lim.Credito: R$ %.2f\n", c->limite_credito);
    sep_txt(f, '-', 62);
    fprintf(f, " Cadastro   : %s\n", c->data_cadastro);
    if (strlen(c->observacoes) > 0)
        fprintf(f, " Observacoes: %s\n", c->observacoes);
    sep_txt(f, '=', 62);
    fputc('\n', f);
}

void exportar_clientes_txt(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║   EXPORTAR CLIENTES PARA ARQUIVO .TXT   ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");
    printf(" O que deseja exportar?\n");
    printf(" [1] Ficha de um cliente especifico -> ficha_cli_NNN.txt\n");
    printf(" [2] Lista completa (tabela)        -> %s\n", ARQ_CLI_TXT);
    printf(" [3] Relatorio gerencial completo   -> %s\n", ARQ_CLI_TXT);
    printf(" [0] Cancelar\n");
    printf(" Opcao: ");
    int op; scanf("%d", &op); limpar_buffer();
    if (op == 0) return;

    FILE *dat = fopen(ARQ_CLIENTES, "rb");
    if (!dat) {
        printf("\n Nenhum registro encontrado. Cadastre clientes primeiro.\n");
        pausar(); return;
    }

    char nome_saida[80];
    strcpy(nome_saida, ARQ_CLI_TXT);
    char agora[20]; data_hoje(agora);

    /* ---- Opção 1: ficha individual ---- */
    if (op == 1) {
        printf(" Codigo do cliente: "); int cod; scanf("%d", &cod); limpar_buffer();
        Cliente c; int achou = 0;
        while (fread(&c, sizeof(Cliente), 1, dat) == 1)
            if (c.codigo == cod) { achou = 1; break; }
        fclose(dat);
        if (!achou) { printf("\n Cliente nao encontrado.\n"); pausar(); return; }

        sprintf(nome_saida, "ficha_cli_%03d.txt", c.codigo);
        FILE *txt = fopen(nome_saida, "w");
        if (!txt) { printf("\n ERRO ao criar arquivo!\n"); pausar(); return; }
        fprintf(txt, "SISTEMA DE CADASTRO DE CLIENTES v%s\n", VERSAO);
        fprintf(txt, "Gerado em: %s\n\n", agora);
        cli_ficha_para_txt(txt, &c);
        fclose(txt);
        printf("\n " GREEN "Arquivo gerado: %s" RESET "\n", nome_saida);
        pausar(); return;
    }

    /* ---- Opções 2 e 3: todos os registros ---- */
    FILE *txt = fopen(nome_saida, "w");
    if (!txt) { fclose(dat); printf("\n ERRO ao criar arquivo!\n"); pausar(); return; }

    /* ---- Opção 2: lista em tabela ---- */
    if (op == 2) {
        fprintf(txt, "SISTEMA DE CADASTRO DE CLIENTES v%s\n", VERSAO);
        fprintf(txt, "Relatorio : LISTA COMPLETA DE CLIENTES\n");
        fprintf(txt, "Gerado em : %s\n\n", agora);
        sep_txt(txt, '-', 78);
        fprintf(txt, "%-5s %-30s %-15s %-12s %-7s\n",
                "COD.", "NOME", "CPF/CNPJ", "CIDADE", "STATUS");
        sep_txt(txt, '-', 78);
        Cliente c; int count = 0;
        while (fread(&c, sizeof(Cliente), 1, dat) == 1) {
            char nome_t[31]; strncpy(nome_t, c.nome,   30); nome_t[30] = '\0';
            char cpf_t[16];  strncpy(cpf_t,  c.cpf,   15); cpf_t[15]  = '\0';
            char cid_t[13];  strncpy(cid_t,  c.cidade, 12); cid_t[12]  = '\0';
            fprintf(txt, "%-5d %-30s %-15s %-12s %-7s\n",
                    c.codigo, nome_t, cpf_t, cid_t,
                    c.ativo ? "ATIVO" : "INATIVO");
            count++;
        }
        sep_txt(txt, '-', 78);
        fprintf(txt, "Total: %d cliente(s).\n", count);
    }
    /* ---- Opção 3: relatório gerencial completo ---- */
    else if (op == 3) {
        fprintf(txt, "================================================================\n");
        fprintf(txt, " SISTEMA DE CADASTRO DE CLIENTES v%s\n", VERSAO);
        fprintf(txt, " RELATORIO GERENCIAL COMPLETO\n");
        fprintf(txt, " Gerado em: %s\n", agora);
        fprintf(txt, "================================================================\n\n");

        int total=0,ativos=0,inativos=0,pf=0,pj=0;
        float lim_total=0.0f, lim_max=0.0f;
        char nome_maior[MAX_NOME_CLI] = "";
        Cliente c;
        while (fread(&c, sizeof(Cliente), 1, dat) == 1) {
            total++;
            if (c.ativo) ativos++; else inativos++;
            if (c.tipo == 1) pf++; else pj++;
            lim_total += c.limite_credito;
            if (c.limite_credito > lim_max) {
                lim_max = c.limite_credito;
                strncpy(nome_maior, c.nome, MAX_NOME_CLI - 1);
            }
        }
        fprintf(txt, "RESUMO ESTATISTICO\n");
        sep_txt(txt, '-', 40);
        fprintf(txt, " Total de clientes    : %d\n", total);
        fprintf(txt, " Clientes ativos      : %d\n", ativos);
        fprintf(txt, " Clientes inativos    : %d\n", inativos);
        fprintf(txt, " Pessoa Fisica (PF)   : %d\n", pf);
        fprintf(txt, " Pessoa Juridica (PJ) : %d\n", pj);
        fprintf(txt, " Limite total (R$)    : %.2f\n", lim_total);
        if (total > 0) {
            fprintf(txt, " Limite medio (R$)    : %.2f\n", lim_total / total);
            fprintf(txt, " Maior limite (R$)    : %.2f (%s)\n", lim_max, nome_maior);
        }
        fprintf(txt, "\n\nFICHAS INDIVIDUAIS\n\n");
        rewind(dat);
        while (fread(&c, sizeof(Cliente), 1, dat) == 1)
            cli_ficha_para_txt(txt, &c);
    }

    fclose(dat); fclose(txt);
    printf("\n " GREEN "Arquivo gerado: %s" RESET "\n", nome_saida);
    pausar();
}

void relatorio_clientes(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║      RELATÓRIO — CLIENTES                ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");

    FILE *f = fopen(ARQ_CLIENTES, "rb");
    if (!f) { printf(" Nenhum registro.\n"); pausar(); return; }

    int total=0,ativos=0,inativos=0,pf=0,pj=0;
    float lim_total=0.0f;
    Cliente c;
    while (fread(&c, sizeof(Cliente), 1, f) == 1) {
        total++;
        if (c.ativo) ativos++; else inativos++;
        if (c.tipo == 1) pf++; else pj++;
        lim_total += c.limite_credito;
    }
    fclose(f);

    printf(" Total de clientes      : %d\n", total);
    printf(" Ativos                 : %d\n", ativos);
    printf(" Inativos               : %d\n", inativos);
    printf(" Pessoa Física (PF)     : %d\n", pf);
    printf(" Pessoa Jurídica (PJ)   : %d\n", pj);
    printf(" Limite total (R$)      : %.2f\n", lim_total);
    if (total > 0)
        printf(" Limite médio (R$)      : %.2f\n", lim_total / total);
    pausar();
}

/* ============================================================
 *  MÓDULO PRODUTO / ESTOQUE
 * ============================================================ */

int prod_calcular_dias(int dia, int mes, int ano) {
    time_t agora = time(NULL);
    struct tm t  = {0};
    t.tm_mday = dia; t.tm_mon = mes - 1; t.tm_year = ano - 1900;
    time_t validade = mktime(&t);
    return (int)(difftime(validade, agora) / 86400);
}

void prod_classificar(Produto *p) {
    int dias = prod_calcular_dias(p->dia_val, p->mes_val, p->ano_val);
    p->dias_restantes = dias;
    if      (dias <= 0)              p->prioridade = 0;
    else if (dias <= LIMITE_CRITICO) p->prioridade = 1;
    else if (dias <= LIMITE_ATENCAO) p->prioridade = 2;
    else                             p->prioridade = 3;
}

const char *prod_label(int p) {
    switch (p) {
        case 0: return "VENCIDO";
        case 1: return "CRITICO";
        case 2: return "ATENCAO";
        default: return "OK";
    }
}

const char *prod_cor(int p) {
    switch (p) {
        case 0: return RED;
        case 1: return YELLOW;
        case 2: return YELLOW;
        default: return GREEN;
    }
}

int prod_proximo_codigo(void) {
    FILE *f = fopen(ARQ_PRODUTOS, "rb");
    if (!f) return 1;
    int max = 0; Produto p;
    while (fread(&p, sizeof(Produto), 1, f) == 1)
        if (p.codigo > max) max = p.codigo;
    fclose(f);
    return max + 1;
}

int prod_total_registros(void) {
    FILE *f = fopen(ARQ_PRODUTOS, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fclose(f);
    return (int)(tam / sizeof(Produto));
}

int prod_buscar_por_codigo(int codigo, Produto *dest) {
    FILE *f = fopen(ARQ_PRODUTOS, "rb");
    if (!f) return 0;
    Produto p;
    while (fread(&p, sizeof(Produto), 1, f) == 1) {
        if (p.codigo == codigo) { *dest = p; fclose(f); return 1; }
    }
    fclose(f);
    return 0;
}

void registrar_movimentacao(int cod, const char *tipo, int qtd) {
    Movimentacao m;
    m.codigo_produto = cod;
    strncpy(m.tipo_acao, tipo, sizeof(m.tipo_acao) - 1);
    m.tipo_acao[sizeof(m.tipo_acao) - 1] = '\0';
    m.quantidade = qtd;
    data_hora_agora(m.data_hora);
    FILE *f = fopen(ARQ_HIST, "ab");
    if (!f) return;
    fwrite(&m, sizeof(Movimentacao), 1, f);
    fclose(f);
}

void prod_exibir_ficha(const Produto *p) {
    prod_classificar((Produto *)p);
    const char *cor = prod_cor(p->prioridade);
    const char *lbl = prod_label(p->prioridade);

    printf("\n ╔══════════════════════════════════════════════════════╗\n");
    printf(" ║ FICHA DO PRODUTO — Cód: %-5d Status: %-8s  ║\n",
           p->codigo, p->ativo ? "ATIVO" : "INATIVO");
    printf(" ╠══════════════════════════════════════════════════════╣\n");
    printf(" ║ Nome       : %-38s ║\n", p->nome);
    printf(" ║ Categoria  : %-38s ║\n", p->categoria);
    printf(" ║ Unidade    : %-10s  Peso/Vol: %-18.3f ║\n", p->unidade, p->peso_volume);
    printf(" ╠══════════════════════════════════════════════════════╣\n");
    printf(" ║ Qtd. estoque: %-37d ║\n", p->quantidade);
    printf(" ║ Preço custo : R$ %-34.2f ║\n", p->preco_custo);
    printf(" ║ Preço venda : R$ %-34.2f ║\n", p->preco_venda);
    printf(" ╠══════════════════════════════════════════════════════╣\n");
    printf(" ║ Validade   : %02d/%02d/%04d  Situação: %s%-7s" RESET "  ║\n",
           p->dia_val, p->mes_val, p->ano_val, cor, lbl);
    printf(" ║ Dias rest. : %-38d ║\n", p->dias_restantes);
    printf(" ╠══════════════════════════════════════════════════════╣\n");
    printf(" ║ Cadastrado : %-38s ║\n", p->data_cadastro);
    if (strlen(p->observacoes) > 0)
        printf(" ║ Obs.       : %-38s ║\n", p->observacoes);
    printf(" ╚══════════════════════════════════════════════════════╝\n");
}

void cadastrar_produto(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║       CADASTRO DE NOVO PRODUTO           ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");

    Produto p;
    memset(&p, 0, sizeof(Produto));
    p.codigo = prod_proximo_codigo();
    p.ativo  = 1;
    data_hoje(p.data_cadastro);
    printf(" Código gerado: %d\n\n", p.codigo);

    printf(" --- IDENTIFICAÇÃO ---\n");
    do {
        ler_string(" Nome do produto   : ", p.nome, MAX_NOME_PROD);
        if (strlen(p.nome) == 0) printf(" Nome e obrigatorio!\n");
    } while (strlen(p.nome) == 0);
    do {
        ler_string(" Categoria         : ", p.categoria, MAX_CATEGORIA);
        if (strlen(p.categoria) == 0) printf(" Categoria e obrigatoria!\n");
    } while (strlen(p.categoria) == 0);
    do {
        ler_string(" Unidade (UN/KG/L) : ", p.unidade, MAX_UNIDADE);
        if (strlen(p.unidade) == 0) printf(" Unidade e obrigatoria!\n");
    } while (strlen(p.unidade) == 0);
    p.peso_volume = ler_float_positivo(" Peso/Volume (por unidade): ");

    printf("\n --- ESTOQUE ---\n");
    p.quantidade = ler_inteiro_positivo(" Quantidade inicial: ");

    printf("\n --- PREÇOS ---\n");
    p.preco_custo = ler_float_positivo(" Preco de custo (R$): ");
    p.preco_venda = ler_float_positivo(" Preco de venda (R$): ");

    printf("\n --- VALIDADE ---\n");
    ler_data_validade(&p.dia_val, &p.mes_val, &p.ano_val);
    prod_classificar(&p);

    printf("\n --- OBSERVAÇÕES ---\n");
    ler_string(" Observações: ", p.observacoes, MAX_OBS_PROD);

    prod_exibir_ficha(&p);
    printf("\n Confirmar cadastro? [S/N]: ");
    char conf; scanf(" %c", &conf); limpar_buffer();

    if (toupper(conf) == 'S') {
        FILE *f = fopen(ARQ_PRODUTOS, "ab");
        if (!f) { printf(" ERRO!\n"); pausar(); return; }
        fwrite(&p, sizeof(Produto), 1, f);
        fclose(f);
        registrar_movimentacao(p.codigo, "CADASTRO", p.quantidade);
        printf("\n " GREEN "✔ Produto cadastrado! Código: %d" RESET "\n", p.codigo);
    } else {
        printf("\n Cancelado.\n");
    }
    pausar();
}

void listar_produtos(int apenas_ativos) {
    limpar_tela();
    printf("\n ╔══════╦═══════════════════╦════════════════╦══════╦══════════╦══════════╗\n");
    printf(" ║ CÓD. ║ NOME              ║ CATEGORIA      ║ QTD  ║ VDA(R$)  ║ SITUAÇÃO ║\n");
    printf(" ╠══════╬═══════════════════╬════════════════╬══════╬══════════╬══════════╣\n");

    FILE *f = fopen(ARQ_PRODUTOS, "rb");
    if (!f) {
        printf(" ║ Nenhum produto cadastrado.                                        ║\n");
        printf(" ╚══════╩═══════════════════╩════════════════╩══════╩══════════╩══════════╝\n");
        pausar(); return;
    }

    Produto p; int count = 0;
    while (fread(&p, sizeof(Produto), 1, f) == 1) {
        if (apenas_ativos && !p.ativo) continue;
        prod_classificar(&p);
        char n[20]; strncpy(n, p.nome,      19); n[19] = '\0';
        char c[15]; strncpy(c, p.categoria, 14); c[14] = '\0';
        printf(" ║ %-4d ║ %-17s ║ %-14s ║ %-4d ║ %-8.2f ║ %s%-8s" RESET " ║\n",
               p.codigo, n, c, p.quantidade, p.preco_venda,
               prod_cor(p.prioridade), prod_label(p.prioridade));
        count++;
    }
    fclose(f);
    printf(" ╚══════╩═══════════════════╩════════════════╩══════╩══════════╩══════════╝\n");
    printf(" Total: %d produto(s).\n", count);
    pausar();
}

void consultar_produto(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║         CONSULTA DE PRODUTO              ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");
    printf(" [1] Código  [2] Nome  [3] Categoria\n Opção: ");
    int op; scanf("%d", &op); limpar_buffer();

    FILE *f = fopen(ARQ_PRODUTOS, "rb");
    if (!f) { printf("\n Nenhum registro.\n"); pausar(); return; }

    int encontrou = 0; Produto p;
    if (op == 1) {
        printf(" Código: "); int cod; scanf("%d", &cod); limpar_buffer();
        while (fread(&p, sizeof(Produto), 1, f) == 1)
            if (p.codigo == cod) { prod_classificar(&p); prod_exibir_ficha(&p); encontrou = 1; }
    } else if (op == 2) {
        char busca[MAX_NOME_PROD]; ler_string(" Nome (parte): ", busca, MAX_NOME_PROD);
        char b_low[MAX_NOME_PROD];
        for (int i = 0; busca[i]; i++) b_low[i] = tolower((unsigned char)busca[i]);
        b_low[strlen(busca)] = '\0';
        while (fread(&p, sizeof(Produto), 1, f) == 1) {
            char n_low[MAX_NOME_PROD];
            for (int i = 0; p.nome[i]; i++) n_low[i] = tolower((unsigned char)p.nome[i]);
            n_low[strlen(p.nome)] = '\0';
            if (strstr(n_low, b_low)) { prod_classificar(&p); prod_exibir_ficha(&p); encontrou = 1; }
        }
    } else if (op == 3) {
        char busca[MAX_CATEGORIA]; ler_string(" Categoria: ", busca, MAX_CATEGORIA);
        char b_low[MAX_CATEGORIA];
        for (int i = 0; busca[i]; i++) b_low[i] = tolower((unsigned char)busca[i]);
        b_low[strlen(busca)] = '\0';
        while (fread(&p, sizeof(Produto), 1, f) == 1) {
            char c_low[MAX_CATEGORIA];
            for (int i = 0; p.categoria[i]; i++) c_low[i] = tolower((unsigned char)p.categoria[i]);
            c_low[strlen(p.categoria)] = '\0';
            if (strstr(c_low, b_low)) { prod_classificar(&p); prod_exibir_ficha(&p); encontrou = 1; }
        }
    }
    fclose(f);
    if (!encontrou) printf("\n Produto não encontrado.\n");
    pausar();
}

void editar_produto(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║            EDITAR PRODUTO                ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");
    printf(" Código do produto: "); int cod; scanf("%d", &cod); limpar_buffer();

    Produto p;
    if (!prod_buscar_por_codigo(cod, &p)) { printf(" Produto não encontrado!\n"); pausar(); return; }
    prod_classificar(&p); prod_exibir_ficha(&p);

    printf("\n [1] Nome  [2] Categoria  [3] Unidade/Peso  [4] Preços\n");
    printf(" [5] Validade  [6] Observações  [7] Status  [0] Cancelar\n Opção: ");
    int op; scanf("%d", &op); limpar_buffer();

    switch (op) {
        case 1: ler_string(" Novo nome: ", p.nome, MAX_NOME_PROD); break;
        case 2: ler_string(" Categoria: ", p.categoria, MAX_CATEGORIA); break;
        case 3:
            ler_string(" Unidade: ", p.unidade, MAX_UNIDADE);
            printf(" Peso/Volume: "); scanf("%f", &p.peso_volume); limpar_buffer();
            break;
        case 4:
            p.preco_custo = ler_float_positivo(" Custo (R$): ");
            p.preco_venda = ler_float_positivo(" Venda (R$): ");
            break;
        case 5:
            ler_data_validade(&p.dia_val, &p.mes_val, &p.ano_val);
            prod_classificar(&p);
            break;
        case 6: ler_string(" Observações: ", p.observacoes, MAX_OBS_PROD); break;
        case 7:
            p.ativo = !p.ativo;
            printf(" Status: %s\n", p.ativo ? "ATIVO" : "INATIVO");
            break;
        default: printf(" Cancelado.\n"); pausar(); return;
    }

    FILE *f = fopen(ARQ_PRODUTOS, "r+b");
    if (!f) { printf(" ERRO!\n"); pausar(); return; }
    Produto tmp;
    while (fread(&tmp, sizeof(Produto), 1, f) == 1) {
        if (tmp.codigo == cod) {
            fseek(f, -(long)sizeof(Produto), SEEK_CUR);
            fwrite(&p, sizeof(Produto), 1, f);
            break;
        }
    }
    fclose(f);
    registrar_movimentacao(p.codigo, "EDICAO", 0);
    printf("\n " GREEN "✔ Produto atualizado!" RESET "\n");
    pausar();
}

void excluir_produto(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║      EXCLUIR / INATIVAR PRODUTO          ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");
    printf(" Código: "); int cod; scanf("%d", &cod); limpar_buffer();

    Produto p;
    if (!prod_buscar_por_codigo(cod, &p)) { printf(" Produto não encontrado!\n"); pausar(); return; }
    prod_classificar(&p); prod_exibir_ficha(&p);

    printf("\n [1] Inativar  [2] Excluir definitivamente  [0] Cancelar\n Opção: ");
    int op; scanf("%d", &op); limpar_buffer();

    if (op == 1) {
        p.ativo = 0;
        FILE *f = fopen(ARQ_PRODUTOS, "r+b");
        Produto tmp;
        while (fread(&tmp, sizeof(Produto), 1, f) == 1) {
            if (tmp.codigo == cod) {
                fseek(f, -(long)sizeof(Produto), SEEK_CUR);
                fwrite(&p, sizeof(Produto), 1, f);
                break;
            }
        }
        fclose(f);
        registrar_movimentacao(cod, "INATIVADO", p.quantidade);
        printf("\n " YELLOW "✔ Produto inativado." RESET "\n");
    } else if (op == 2) {
        printf(" Confirma? [S/N]: "); char conf; scanf(" %c", &conf); limpar_buffer();
        if (toupper(conf) == 'S') {
            FILE *in  = fopen(ARQ_PRODUTOS,     "rb");
            FILE *out = fopen("produtos_tmp.dat","wb");
            if (!in || !out) {
                if (in)  fclose(in);
                if (out) fclose(out);
                printf(" ERRO!\n"); pausar(); return;
            }
            Produto tmp;
            while (fread(&tmp, sizeof(Produto), 1, in) == 1)
                if (tmp.codigo != cod) fwrite(&tmp, sizeof(Produto), 1, out);
            fclose(in); fclose(out);
            remove(ARQ_PRODUTOS); rename("produtos_tmp.dat", ARQ_PRODUTOS);
            registrar_movimentacao(cod, "EXCLUSAO", p.quantidade);
            printf("\n " RED "✔ Produto excluído." RESET "\n");
        } else printf(" Cancelado.\n");
    }
    pausar();
}

void movimentar_estoque(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║     MOVIMENTAR ESTOQUE (ENTRADA/SAÍDA)   ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");
    printf(" Código do produto: "); int cod; scanf("%d", &cod); limpar_buffer();

    Produto p;
    if (!prod_buscar_por_codigo(cod, &p)) { printf(" Produto não encontrado!\n"); pausar(); return; }
    prod_classificar(&p);
    printf("\n Produto: %s | Estoque: %d %s\n\n", p.nome, p.quantidade, p.unidade);
    printf(" [1] Entrada (+)  [2] Saída (-)  [0] Cancelar\n Opção: ");
    int op; scanf("%d", &op); limpar_buffer();
    if (op == 0) return;

    printf(" Quantidade: ");
    int qtd = ler_inteiro_positivo(" Quantidade (> 0): ");
    if (qtd <= 0) { printf(" Quantidade deve ser maior que zero!\n"); pausar(); return; }

    if (op == 1) {
        p.quantidade += qtd;
        registrar_movimentacao(cod, "ENTRADA", qtd);
        printf("\n " GREEN "✔ Entrada! Novo saldo: %d %s" RESET "\n", p.quantidade, p.unidade);
    } else if (op == 2) {
        if (qtd > p.quantidade) {
            printf("\n " RED "[ERRO] Estoque insuficiente!" RESET "\n"); pausar(); return;
        }
        p.quantidade -= qtd;
        registrar_movimentacao(cod, "SAIDA", qtd);
        printf("\n " GREEN "✔ Saída! Novo saldo: %d %s" RESET "\n", p.quantidade, p.unidade);
    } else { printf(" Opção inválida.\n"); pausar(); return; }

    FILE *f = fopen(ARQ_PRODUTOS, "r+b");
    if (!f) { printf(" ERRO ao salvar!\n"); pausar(); return; }
    Produto tmp;
    while (fread(&tmp, sizeof(Produto), 1, f) == 1) {
        if (tmp.codigo == cod) {
            fseek(f, -(long)sizeof(Produto), SEEK_CUR);
            fwrite(&p, sizeof(Produto), 1, f);
            break;
        }
    }
    fclose(f);
    pausar();
}

void relatorio_produtos(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║       RELATÓRIO — PRODUTOS               ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");

    FILE *f = fopen(ARQ_PRODUTOS, "rb");
    if (!f) { printf(" Nenhum registro.\n"); pausar(); return; }

    int total=0,ativos=0,inativos=0,vencidos=0,criticos=0,atencao=0,ok=0;
    float v_custo=0.0f, v_venda=0.0f;
    Produto p;
    while (fread(&p, sizeof(Produto), 1, f) == 1) {
        prod_classificar(&p); total++;
        if (p.ativo) ativos++; else inativos++;
        if      (p.prioridade == 0) vencidos++;
        else if (p.prioridade == 1) criticos++;
        else if (p.prioridade == 2) atencao++;
        else                         ok++;
        v_custo += p.preco_custo * p.quantidade;
        v_venda += p.preco_venda * p.quantidade;
    }
    fclose(f);

    printf(" Total de produtos       : %d\n", total);
    printf(" Ativos                  : %d\n", ativos);
    printf(" Inativos                : %d\n\n", inativos);
    printf(" " GREEN  "[OK]      " RESET ": %d\n", ok);
    printf(" " YELLOW "[ATENÇÃO] " RESET ": %d\n", atencao);
    printf(" " YELLOW "[CRÍTICO] " RESET ": %d\n", criticos);
    printf(" " RED    "[VENCIDO] " RESET ": %d\n\n", vencidos);
    printf(" Valor estoque (custo)   : R$ %.2f\n", v_custo);
    printf(" Valor estoque (venda)   : R$ %.2f\n", v_venda);
    if (total > 0)
        printf(" Margem bruta estimada   : R$ %.2f\n", v_venda - v_custo);
    pausar();
}

/* --- Grava a ficha completa de um produto no arquivo .txt --- */
static void prod_ficha_para_txt(FILE *f, const Produto *p) {
    sep_txt(f, '=', 64);
    fprintf(f, " FICHA DO PRODUTO\n");
    fprintf(f, " Codigo    : %d\n", p->codigo);
    fprintf(f, " Status    : %s\n", p->ativo ? "ATIVO" : "INATIVO");
    sep_txt(f, '-', 64);
    fprintf(f, " IDENTIFICACAO\n");
    fprintf(f, " Nome      : %s\n", p->nome);
    fprintf(f, " Categoria : %s\n", p->categoria);
    fprintf(f, " Unidade   : %s\n", p->unidade);
    fprintf(f, " Peso/Vol  : %.3f\n", p->peso_volume);
    sep_txt(f, '-', 64);
    fprintf(f, " ESTOQUE E PRECOS\n");
    fprintf(f, " Quantidade: %d\n",  p->quantidade);
    fprintf(f, " Custo(R$) : %.2f\n", p->preco_custo);
    fprintf(f, " Venda(R$) : %.2f\n", p->preco_venda);
    sep_txt(f, '-', 64);
    fprintf(f, " VALIDADE\n");
    fprintf(f, " Data      : %02d/%02d/%04d\n", p->dia_val, p->mes_val, p->ano_val);
    fprintf(f, " Situacao  : %s\n", prod_label(p->prioridade));
    fprintf(f, " Dias rest.: %d\n",  p->dias_restantes);
    sep_txt(f, '-', 64);
    fprintf(f, " Cadastro  : %s\n", p->data_cadastro);
    if (strlen(p->observacoes) > 0)
        fprintf(f, " Obs.      : %s\n", p->observacoes);
    sep_txt(f, '=', 64);
    fputc('\n', f);
}

void exportar_produtos_txt(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║   EXPORTAR PRODUTOS PARA ARQUIVO .TXT   ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");
    printf(" O que deseja exportar?\n");
    printf(" [1] Ficha de um produto especifico -> ficha_prod_NNN.txt\n");
    printf(" [2] Lista completa (tabela)        -> %s\n", ARQ_PROD_TXT);
    printf(" [3] Relatorio gerencial completo   -> %s\n", ARQ_PROD_TXT);
    printf(" [0] Cancelar\n");
    printf(" Opcao: ");
    int op; scanf("%d", &op); limpar_buffer();
    if (op == 0) return;

    FILE *dat = fopen(ARQ_PRODUTOS, "rb");
    if (!dat) {
        printf("\n Nenhum registro encontrado. Cadastre produtos primeiro.\n");
        pausar(); return;
    }

    char nome_saida[80];
    strcpy(nome_saida, ARQ_PROD_TXT);
    char agora[20]; data_hoje(agora);

    /* ---- Opção 1: ficha individual ---- */
    if (op == 1) {
        printf(" Codigo do produto: "); int cod; scanf("%d", &cod); limpar_buffer();
        Produto p; int achou = 0;
        while (fread(&p, sizeof(Produto), 1, dat) == 1)
            if (p.codigo == cod) { prod_classificar(&p); achou = 1; break; }
        fclose(dat);
        if (!achou) { printf("\n Produto nao encontrado.\n"); pausar(); return; }

        sprintf(nome_saida, "ficha_prod_%03d.txt", p.codigo);
        FILE *txt = fopen(nome_saida, "w");
        if (!txt) { printf("\n ERRO ao criar arquivo!\n"); pausar(); return; }
        fprintf(txt, "SISTEMA DE PRODUTOS - MERCADINHO v%s\n", VERSAO);
        fprintf(txt, "Gerado em: %s\n\n", agora);
        prod_ficha_para_txt(txt, &p);
        fclose(txt);
        printf("\n " GREEN "Arquivo gerado: %s" RESET "\n", nome_saida);
        pausar(); return;
    }

    /* ---- Opções 2 e 3: todos os registros ---- */
    FILE *txt = fopen(nome_saida, "w");
    if (!txt) { fclose(dat); printf("\n ERRO ao criar arquivo!\n"); pausar(); return; }

    /* ---- Opção 2: lista em tabela ---- */
    if (op == 2) {
        fprintf(txt, "SISTEMA DE PRODUTOS - MERCADINHO v%s\n", VERSAO);
        fprintf(txt, "Relatorio : LISTA COMPLETA DE PRODUTOS\n");
        fprintf(txt, "Gerado em : %s\n\n", agora);
        sep_txt(txt, '-', 80);
        fprintf(txt, "%-5s %-20s %-14s %-5s %-9s %-9s %-8s\n",
                "COD.", "NOME", "CATEGORIA", "QTD", "CUSTO", "VENDA", "SITUACAO");
        sep_txt(txt, '-', 80);
        Produto p; int count = 0;
        while (fread(&p, sizeof(Produto), 1, dat) == 1) {
            prod_classificar(&p);
            char n[21]; strncpy(n, p.nome,      20); n[20] = '\0';
            char c[15]; strncpy(c, p.categoria, 14); c[14] = '\0';
            fprintf(txt, "%-5d %-20s %-14s %-5d %-9.2f %-9.2f %-8s\n",
                    p.codigo, n, c, p.quantidade,
                    p.preco_custo, p.preco_venda,
                    prod_label(p.prioridade));
            count++;
        }
        sep_txt(txt, '-', 80);
        fprintf(txt, "Total: %d produto(s).\n", count);
    }
    /* ---- Opção 3: relatório gerencial completo ---- */
    else if (op == 3) {
        fprintf(txt, "================================================================\n");
        fprintf(txt, " SISTEMA DE PRODUTOS - MERCADINHO v%s\n", VERSAO);
        fprintf(txt, " RELATORIO GERENCIAL COMPLETO\n");
        fprintf(txt, " Gerado em: %s\n", agora);
        fprintf(txt, "================================================================\n\n");

        int total=0,ativos=0,inativos=0,vencidos=0,criticos=0,atencao_c=0,ok_c=0;
        float v_custo=0.0f, v_venda=0.0f;
        Produto p;
        while (fread(&p, sizeof(Produto), 1, dat) == 1) {
            prod_classificar(&p); total++;
            if (p.ativo) ativos++; else inativos++;
            if      (p.prioridade == 0) vencidos++;
            else if (p.prioridade == 1) criticos++;
            else if (p.prioridade == 2) atencao_c++;
            else                         ok_c++;
            v_custo += p.preco_custo * p.quantidade;
            v_venda += p.preco_venda * p.quantidade;
        }
        fprintf(txt, "RESUMO ESTATISTICO\n");
        sep_txt(txt, '-', 42);
        fprintf(txt, " Total de produtos    : %d\n", total);
        fprintf(txt, " Ativos               : %d\n", ativos);
        fprintf(txt, " Inativos             : %d\n", inativos);
        fprintf(txt, " [OK]                 : %d\n", ok_c);
        fprintf(txt, " [ATENCAO]            : %d\n", atencao_c);
        fprintf(txt, " [CRITICO]            : %d\n", criticos);
        fprintf(txt, " [VENCIDO]            : %d\n", vencidos);
        fprintf(txt, " Valor custo (R$)     : %.2f\n", v_custo);
        fprintf(txt, " Valor venda (R$)     : %.2f\n", v_venda);
        if (total > 0)
            fprintf(txt, " Margem bruta (R$)    : %.2f\n", v_venda - v_custo);
        fprintf(txt, "\n\nFICHAS INDIVIDUAIS\n\n");
        rewind(dat);
        while (fread(&p, sizeof(Produto), 1, dat) == 1) {
            prod_classificar(&p);
            prod_ficha_para_txt(txt, &p);
        }
    }

    fclose(dat); fclose(txt);
    printf("\n " GREEN "Arquivo gerado: %s" RESET "\n", nome_saida);
    pausar();
}

void consultar_historico(void) {
    limpar_tela();
    printf("\n ╔═══════════════════════════════════════════════════════════════════╗\n");
    printf(" ║       HISTÓRICO DE MOVIMENTAÇÕES (AUDITORIA)                     ║\n");
    printf(" ╠═════════════════════════╦════════════╦══════════════╦═════════════╣\n");
    printf(" ║ Data / Hora             ║ Cód. Prod. ║ Ação         ║ Quantidade  ║\n");
    printf(" ╠═════════════════════════╬════════════╬══════════════╬═════════════╣\n");

    FILE *f = fopen(ARQ_HIST, "rb");
    if (!f) {
        printf(" ║ Nenhuma movimentação registrada ainda.                           ║\n");
        printf(" ╚═════════════════════════╩════════════╩══════════════╩═════════════╝\n");
        pausar(); return;
    }
    Movimentacao m; int count = 0;
    while (fread(&m, sizeof(Movimentacao), 1, f) == 1) {
        printf(" ║ %-23s ║ %-10d ║ %-12s ║ %-11d ║\n",
               m.data_hora, m.codigo_produto, m.tipo_acao, m.quantidade);
        count++;
    }
    fclose(f);
    printf(" ╚═════════════════════════╩════════════╩══════════════╩═════════════╝\n");
    printf(" Total de movimentações: %d\n", count);
    pausar();
}

/* ============================================================
 *  MÓDULO PEDIDO (PONTO DE VENDA)
 * ============================================================ */

int ped_proximo_numero(void) {
    FILE *f = fopen(ARQ_PEDIDOS, "rb");
    if (!f) return 1;
    int max = 0; Pedido p;
    while (fread(&p, sizeof(Pedido), 1, f) == 1)
        if (p.numero > max) max = p.numero;
    fclose(f);
    return max + 1;
}

void imprimir_cupom(const Pedido *ped) {
    printf("\n ========================================\n");
    printf("       MERCADINHO DO PORTO\n");
    printf(" ========================================\n");
    printf(" Pedido Nº : %d\n", ped->numero);
    printf(" Data      : %s\n", ped->data_hora);
    printf(" Cliente   : %s\n", ped->nome_cliente);
    printf(" ----------------------------------------\n");
    printf(" %-20s %4s %10s\n", "PRODUTO", "QTD", "SUBTOTAL");
    printf(" ----------------------------------------\n");
    for (int i = 0; i < ped->total_itens; i++) {
        const ItemPedido *it = &ped->itens[i];
        printf(" %-20s %4d R$ %8.2f\n",
               it->nome_produto, it->quantidade, it->subtotal);
    }
    printf(" ----------------------------------------\n");
    printf(" Total          : R$ %.2f\n", ped->total);
    if (ped->desconto > 0)
        printf(" Desconto       : R$ %.2f\n", ped->desconto);
    printf(" Total Final    : R$ %.2f\n", ped->total_final);
    printf(" Pagamento      : %s\n",      ped->forma_pagamento);
    printf(" ========================================\n");
    printf("        Obrigado pela preferência!\n");
    printf(" ========================================\n\n");
}

void realizar_venda(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║         PONTO DE VENDA                   ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");

    Pedido ped;
    memset(&ped, 0, sizeof(Pedido));
    ped.numero      = ped_proximo_numero();
    ped.total_itens = 0;
    ped.total       = 0.0f;
    ped.status      = 1;
    data_hora_agora(ped.data_hora);

    /* Identifica cliente (opcional) */
    printf(" Código do cliente (0 para venda sem cadastro): ");
    int cod_cli; scanf("%d", &cod_cli); limpar_buffer();
    if (cod_cli > 0) {
        Cliente c;
        if (cli_buscar_por_codigo(cod_cli, &c)) {
            if (!c.ativo) {
                printf("\n " RED "[BLOQUEADO] Cliente INATIVO nao pode realizar compras!" RESET "\n");
                printf(" Ative o cliente antes de continuar.\n");
                pausar(); return;
            }
            ped.codigo_cliente = cod_cli;
            strncpy(ped.nome_cliente, c.nome, MAX_NOME_CLI - 1);
        } else {
            printf(" Cliente nao encontrado. Continuando sem cadastro.\n");
            strcpy(ped.nome_cliente, "CONSUMIDOR FINAL");
        }
    } else {
        strcpy(ped.nome_cliente, "CONSUMIDOR FINAL");
    }

    /* Adiciona itens */
    int continuar = 1;
    while (continuar && ped.total_itens < MAX_ITENS_PEDIDO) {
        printf("\n --- Item %d ---\n", ped.total_itens + 1);
        printf(" Código do produto (0 para finalizar): ");
        int cod_prod; scanf("%d", &cod_prod); limpar_buffer();
        if (cod_prod == 0) break;

        Produto prod;
        if (!prod_buscar_por_codigo(cod_prod, &prod)) {
            printf(" Produto não encontrado!\n"); continue;
        }
        if (!prod.ativo) { printf(" Produto inativo!\n"); continue; }
        if (prod.quantidade == 0) { printf(" Sem estoque!\n"); continue; }

        printf(" Produto: %s | Estoque: %d | Preço: R$ %.2f\n",
               prod.nome, prod.quantidade, prod.preco_venda);
        int qtd = ler_inteiro_positivo(" Quantidade (> 0): ");

        if (qtd <= 0 || qtd > prod.quantidade) {
            printf(" Quantidade invalida ou maior que o estoque!\n"); continue;
        }

        ItemPedido *item = &ped.itens[ped.total_itens];
        item->codigo_produto = cod_prod;
        strncpy(item->nome_produto, prod.nome, MAX_NOME_PROD - 1);
        item->quantidade     = qtd;
        item->preco_unitario = prod.preco_venda;
        item->subtotal       = qtd * prod.preco_venda;

        ped.total += item->subtotal;
        ped.total_itens++;

        /* Atualiza estoque imediatamente */
        prod.quantidade -= qtd;
        FILE *f = fopen(ARQ_PRODUTOS, "r+b");
        if (f) {
            Produto tmp;
            while (fread(&tmp, sizeof(Produto), 1, f) == 1) {
                if (tmp.codigo == cod_prod) {
                    fseek(f, -(long)sizeof(Produto), SEEK_CUR);
                    fwrite(&prod, sizeof(Produto), 1, f);
                    break;
                }
            }
            fclose(f);
        }
        registrar_movimentacao(cod_prod, "SAIDA", qtd);
        printf(" " GREEN "✔ Item adicionado. Subtotal: R$ %.2f" RESET "\n", item->subtotal);
        printf(" Total parcial: R$ %.2f\n", ped.total);
    }

    if (ped.total_itens == 0) { printf("\n Nenhum item. Venda cancelada.\n"); pausar(); return; }

    /* Desconto */
    printf("\n Total: R$ %.2f\n", ped.total);
    ped.desconto = ler_float_positivo(" Desconto (R$, 0 para nenhum): ");
    if (ped.desconto > ped.total) {
        printf(" Desconto nao pode ser maior que o total! Desconto zerado.\n");
        ped.desconto = 0.0f;
    }
    ped.total_final = ped.total - ped.desconto;

    /* Pagamento */
    printf(" Forma de pagamento:\n [1] Dinheiro  [2] Cartão Débito  [3] Cartão Crédito  [4] PIX\n Opção: ");
    int pag; scanf("%d", &pag); limpar_buffer();
    switch (pag) {
        case 1: strcpy(ped.forma_pagamento, "Dinheiro");       break;
        case 2: strcpy(ped.forma_pagamento, "Cartão Débito");  break;
        case 3: strcpy(ped.forma_pagamento, "Cartão Crédito"); break;
        case 4: strcpy(ped.forma_pagamento, "PIX");            break;
        default:strcpy(ped.forma_pagamento, "Não informado");  break;
    }
    ped.status = 2;

    /* Salva pedido */
    FILE *f = fopen(ARQ_PEDIDOS, "ab");
    if (f) { fwrite(&ped, sizeof(Pedido), 1, f); fclose(f); }

    imprimir_cupom(&ped);
    pausar();
}

void listar_pedidos(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════════════════════════╗\n");
    printf(" ║              HISTÓRICO DE PEDIDOS                            ║\n");
    printf(" ╠════════╦══════════════════════╦══════════╦═══════════════════╣\n");
    printf(" ║ Nº PED ║ DATA/HORA            ║ TOTAL(R$)║ CLIENTE           ║\n");
    printf(" ╠════════╬══════════════════════╬══════════╬═══════════════════╣\n");

    FILE *f = fopen(ARQ_PEDIDOS, "rb");
    if (!f) {
        printf(" ║ Nenhum pedido registrado.                                    ║\n");
        printf(" ╚══════════════════════════════════════════════════════════════╝\n");
        pausar(); return;
    }
    Pedido p; int count = 0;
    while (fread(&p, sizeof(Pedido), 1, f) == 1) {
        char cli_t[20]; strncpy(cli_t, p.nome_cliente, 19); cli_t[19] = '\0';
        printf(" ║ %-6d ║ %-20s ║ %-8.2f ║ %-17s ║\n",
               p.numero, p.data_hora, p.total_final, cli_t);
        count++;
    }
    fclose(f);
    printf(" ╚════════╩══════════════════════╩══════════╩═══════════════════╝\n");
    printf(" Total: %d pedido(s).\n", count);
    pausar();
}

void relatorio_vendas(void) {
    limpar_tela();
    printf("\n ╔══════════════════════════════════════════╗\n");
    printf(" ║       RELATÓRIO DE VENDAS                ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");

    FILE *f = fopen(ARQ_PEDIDOS, "rb");
    if (!f) { printf(" Nenhum pedido encontrado.\n"); pausar(); return; }

    int total_pedidos = 0, total_itens = 0;
    float total_bruto = 0.0f, total_desconto = 0.0f, total_liquido = 0.0f;
    Pedido p;
    while (fread(&p, sizeof(Pedido), 1, f) == 1) {
        if (p.status != 2) continue;
        total_pedidos++;
        total_itens     += p.total_itens;
        total_bruto     += p.total;
        total_desconto  += p.desconto;
        total_liquido   += p.total_final;
    }
    fclose(f);

    printf(" Pedidos finalizados     : %d\n", total_pedidos);
    printf(" Total de itens vendidos : %d\n", total_itens);
    printf(" Receita bruta (R$)      : %.2f\n", total_bruto);
    printf(" Total descontos (R$)    : %.2f\n", total_desconto);
    printf(" Receita líquida (R$)    : %.2f\n", total_liquido);
    if (total_pedidos > 0)
        printf(" Ticket médio (R$)       : %.2f\n", total_liquido / total_pedidos);
    pausar();
}

/* ============================================================
 *  SUBMENUS
 * ============================================================ */

void submenu_clientes(void) {
    int op;
    do {
        limpar_tela();
        printf("\n ╔══════════════════════════════════════════╗\n");
        printf(" ║        CADASTRO DE CLIENTES              ║\n");
        printf(" ╠══════════════════════════════════════════╣\n");
        printf(" ║ [1] Cadastrar novo cliente               ║\n");
        printf(" ║ [2] Consultar cliente                    ║\n");
        printf(" ║ [3] Editar cadastro                      ║\n");
        printf(" ║ [4] Excluir / Inativar cliente           ║\n");
        printf(" ║ [5] Listar todos os clientes             ║\n");
        printf(" ║ [6] Listar apenas ativos                 ║\n");
        printf(" ║ [7] Relatório resumido                   ║\n");
        printf(" ║ [8] Exportar para TXT                    ║\n");
        printf(" ║ [0] Voltar ao menu principal             ║\n");
        printf(" ╚══════════════════════════════════════════╝\n");
        printf(" Total de clientes: %d\n\n", cli_total_registros());
        printf(" Opção: "); scanf("%d", &op); limpar_buffer();

        switch (op) {
            case 1: cadastrar_cliente();     break;
            case 2: consultar_cliente();     break;
            case 3: editar_cliente();        break;
            case 4: excluir_cliente();       break;
            case 5: listar_clientes(0);      break;
            case 6: listar_clientes(1);      break;
            case 7: relatorio_clientes();    break;
            case 8: exportar_clientes_txt(); break;
            case 0: break;
            default: printf("\n Opção inválida!\n"); pausar();
        }
    } while (op != 0);
}

void submenu_estoque(void) {
    int op;
    do {
        limpar_tela();
        printf("\n ╔══════════════════════════════════════════╗\n");
        printf(" ║        CONTROLE DE ESTOQUE               ║\n");
        printf(" ╠══════════════════════════════════════════╣\n");
        printf(" ║ [1] Cadastrar novo produto               ║\n");
        printf(" ║ [2] Consultar produto                    ║\n");
        printf(" ║ [3] Editar produto                       ║\n");
        printf(" ║ [4] Excluir / Inativar produto           ║\n");
        printf(" ║ [5] Movimentar estoque (entrada/saída)   ║\n");
        printf(" ║ [6] Listar todos os produtos             ║\n");
        printf(" ║ [7] Listar apenas ativos                 ║\n");
        printf(" ║ [8] Relatório de estoque                 ║\n");
        printf(" ║ [9] Histórico de movimentações           ║\n");
        printf(" ║ [10] Exportar para TXT                  ║\n");
        printf(" ║ [0] Voltar ao menu principal             ║\n");
        printf(" ╚══════════════════════════════════════════╝\n");
        printf(" Total de produtos: %d\n\n", prod_total_registros());
        printf(" Opção: "); scanf("%d", &op); limpar_buffer();

        switch (op) {
            case 1:  cadastrar_produto();    break;
            case 2:  consultar_produto();    break;
            case 3:  editar_produto();       break;
            case 4:  excluir_produto();      break;
            case 5:  movimentar_estoque();   break;
            case 6:  listar_produtos(0);     break;
            case 7:  listar_produtos(1);     break;
            case 8:  relatorio_produtos();   break;
            case 9:  consultar_historico();  break;
            case 10: exportar_produtos_txt();break;
            case 0: break;
            default: printf("\n Opção inválida!\n"); pausar();
        }
    } while (op != 0);
}

void submenu_vendas(void) {
    int op;
    do {
        limpar_tela();
        printf("\n ╔══════════════════════════════════════════╗\n");
        printf(" ║       VENDAS E RELATÓRIOS                ║\n");
        printf(" ╠══════════════════════════════════════════╣\n");
        printf(" ║ [1] Registrar venda (PDV)                ║\n");
        printf(" ║ [2] Listar pedidos                       ║\n");
        printf(" ║ [3] Relatório de vendas                  ║\n");
        printf(" ║ [0] Voltar ao menu principal             ║\n");
        printf(" ╚══════════════════════════════════════════╝\n\n");
        printf(" Opção: "); scanf("%d", &op); limpar_buffer();

        switch (op) {
            case 1: realizar_venda();   break;
            case 2: listar_pedidos();   break;
            case 3: relatorio_vendas(); break;
            case 0: break;
            default: printf("\n Opção inválida!\n"); pausar();
        }
    } while (op != 0);
}

/* ============================================================
 *  MENU PRINCIPAL
 * ============================================================ */

void menu_principal(void) {
    int escolha;
    do {
        limpar_tela();
        printf("\n ╔══════════════════════════════════════════╗\n");
        printf(" ║                                          ║\n");
        printf(" ║       MERCADINHO DO PORTO                ║\n");
        printf(" ║         Sistema v%-5s                   ║\n", VERSAO);
        printf(" ║                                          ║\n");
        printf(" ╠══════════════════════════════════════════╣\n");
        printf(" ║ [1] PONTO DE VENDA                       ║\n");
        printf(" ║ [2] CONTROLE DE ESTOQUE                  ║\n");
        printf(" ║ [3] VENDAS E RELATÓRIOS                  ║\n");
        printf(" ║ [4] CADASTRO DE CLIENTES                 ║\n");
        printf(" ╠══════════════════════════════════════════╣\n");
        printf(" ║ [0] SAIR DO SISTEMA                      ║\n");
        printf(" ╚══════════════════════════════════════════╝\n\n");
        printf(" Opção: ");
        scanf("%d", &escolha);
        limpar_buffer();

        switch (escolha) {
            case 1: realizar_venda();     break;
            case 2: submenu_estoque();    break;
            case 3: submenu_vendas();     break;
            case 4: submenu_clientes();   break;
            case 0:
                limpar_tela();
                printf("\n Encerrando o sistema. Até logo!\n\n");
                break;
            default:
                printf("\n Opção inválida, tente novamente!\n");
                pausar();
        }
    } while (escolha != 0);
}

/* ============================================================
 *  MAIN
 * ============================================================ */

int main(void) {
    setlocale(LC_ALL, "Portuguese");
    menu_principal();
    return 0;
}

/*
 * ============================================================
 *   SISTEMA DE CADASTRO DE CLIENTES / COMPRADORES
 *   Linguagem: C (ANSI C99)
 *   Arquivo de dados: clientes.dat (binario)
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ==================== CONSTANTES ========================== */
#define MAX_NOME       80
#define MAX_CPF        15
#define MAX_EMAIL      60
#define MAX_TELEFONE   20
#define MAX_ENDERECO   100
#define MAX_CIDADE     50
#define MAX_ESTADO     3
#define MAX_CEP        10
#define MAX_OBS        200
#define ARQUIVO_DADOS  "clientes.dat"
#define ARQUIVO_TXT    "clientes_export.txt"
#define VERSAO         "1.0.0"

/* ==================== ESTRUTURAS ========================== */

typedef struct 
{
    int    codigo;
    char   nome[MAX_NOME];
    char   cpf[MAX_CPF];
    char   rg[MAX_CPF];
    char   data_nascimento[12];   /* DD/MM/AAAA */
    char   email[MAX_EMAIL];
    char   telefone[MAX_TELEFONE];
    char   celular[MAX_TELEFONE];
    char   endereco[MAX_ENDERECO];
    char   numero[10];
    char   complemento[40];
    char   bairro[50];
    char   cidade[MAX_CIDADE];
    char   estado[MAX_ESTADO];
    char   cep[MAX_CEP];
    float  limite_credito;
    int    tipo;                  /* 1=PF  2=PJ */
    int    ativo;                 /* 1=ativo  0=inativo */
    char   data_cadastro[20];
    char   observacoes[MAX_OBS];
} Cliente;

/* ==================== UTILITÁRIOS ========================= */

void limpar_tela(void)  //limpar tela
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pausar(void)  //pausar tela
{
    printf("\n  Pressione ENTER para continuar...");
    getchar();
    getchar();
}

void limpar_buffer(void)  //limpar buffer
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void ler_string(const char *prompt, char *dest, int max) //ler a string
{
    printf("%s", prompt);
    fgets(dest, max, stdin);
    /* remove '\n' */
    int len = strlen(dest);
    if (len > 0 && dest[len-1] == '\n')
        dest[len-1] = '\0';
}

void data_hoje(char *buf) //data atual
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, 20, "%d/%m/%Y %H:%M", tm_info);
}

/* Formata CPF: 00000000000 -> 000.000.000-00 */
void formatar_cpf(const char *entrada, char *saida) 
{
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

/* ==================== ARQUIVO / PERSISTÊNCIA ============== */

int proximo_codigo(void) //passar para próxima função do código
{
    FILE *f = fopen(ARQUIVO_DADOS, "rb");
    if (!f) return 1;
    int max = 0;
    Cliente c;
    while (fread(&c, sizeof(Cliente), 1, f) == 1)
        if (c.codigo > max) max = c.codigo;
    fclose(f);
    return max + 1;
}

int total_registros(void) //total de clientes registrados
{
    FILE *f = fopen(ARQUIVO_DADOS, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fclose(f);
    return (int)(tam / sizeof(Cliente));
}

/* ==================== CADASTRAR CLIENTE ================== */

void cadastrar_cliente(void) 
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║        CADASTRO DE NOVO CLIENTE          ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    Cliente c;
    memset(&c, 0, sizeof(Cliente)); //guardar na memória

    c.codigo = proximo_codigo();
    c.ativo  = 1;
    data_hoje(c.data_cadastro);

    printf("  Código gerado automaticamente: %d\n\n", c.codigo);

    /* ----- Tipo de pessoa ----- */
    printf("  Tipo de cliente:\n");
    printf("    [1] Pessoa Física\n");
    printf("    [2] Pessoa Jurídica\n");
    printf("  Opção: ");
    scanf("%d", &c.tipo);
    limpar_buffer();
    if (c.tipo != 1 && c.tipo != 2) c.tipo = 1;

    /* ----- Dados pessoais ----- */
    printf("\n  --- DADOS PESSOAIS ---\n");
    ler_string("  Nome completo : ", c.nome, MAX_NOME);
    if (strlen(c.nome) == 0) {
        printf("  Nome é obrigatório!\n");
        pausar();
        return;
    }

    if (c.tipo == 1) 
    {
        char cpf_raw[MAX_CPF];
        ler_string("  CPF (somente números): ", cpf_raw, MAX_CPF);
        formatar_cpf(cpf_raw, c.cpf);
        ler_string("  RG                   : ", c.rg, MAX_CPF);
        ler_string("  Data de nascimento   : ", c.data_nascimento, 12);
    } else 
    {
        ler_string("  CNPJ                 : ", c.cpf, MAX_CPF);
        ler_string("  Inscrição Estadual   : ", c.rg, MAX_CPF);
    }

    /* ----- Contato ----- */
    printf("\n  --- DADOS DE CONTATO ---\n");
    ler_string("  E-mail    : ", c.email, MAX_EMAIL);
    ler_string("  Telefone  : ", c.telefone, MAX_TELEFONE);
    ler_string("  Celular   : ", c.celular, MAX_TELEFONE);

    /* ----- Endereço ----- */
    printf("\n  --- ENDEREÇO ---\n");
    ler_string("  Logradouro  : ", c.endereco, MAX_ENDERECO);
    ler_string("  Número      : ", c.numero, 10);
    ler_string("  Complemento : ", c.complemento, 40);
    ler_string("  Bairro      : ", c.bairro, 50);
    ler_string("  Cidade      : ", c.cidade, MAX_CIDADE);
    ler_string("  Estado (UF) : ", c.estado, MAX_ESTADO);
    ler_string("  CEP         : ", c.cep, MAX_CEP);

    /* ----- Financeiro ----- */
    printf("\n  --- DADOS FINANCEIROS ---\n");
    printf("  Limite de crédito (R$): ");
    scanf("%f", &c.limite_credito);
    limpar_buffer();

    /* ----- Observações ----- */
    printf("\n  --- OBSERVAÇÕES ---\n");
    ler_string("  Observações: ", c.observacoes, MAX_OBS);

    /* ----- Confirmar e gravar ----- */
    printf("\n  Confirmar cadastro? [S/N]: ");
    char conf;
    scanf(" %c", &conf);
    limpar_buffer();

    if (toupper(conf) == 'S') 
    {
        FILE *f = fopen(ARQUIVO_DADOS, "ab");
        if (!f) 
        {
            printf("  ERRO: não foi possível abrir o arquivo!\n");
            pausar();
            return;
        }
        fwrite(&c, sizeof(Cliente), 1, f);
        fclose(f);
        printf("\n  ✔ Cliente cadastrado com sucesso! Código: %d\n", c.codigo);
    } else 
    {
        printf("\n  Cadastro cancelado.\n");
    }
    pausar();
}

/* ==================== EXIBIR FICHA ======================= */

void exibir_ficha(const Cliente *c) 
{
    printf("\n  ╔══════════════════════════════════════════════════════╗\n");
    printf("  ║  FICHA DO CLIENTE  ─  Cód: %-5d  Status: %-8s  ║\n",
           c->codigo, c->ativo ? "ATIVO" : "INATIVO");
    printf("  ╠══════════════════════════════════════════════════════╣\n");
    printf("  ║  Nome       : %-38s  ║\n", c->nome);
    printf("  ║  Tipo       : %-38s  ║\n", c->tipo == 1 ? "Pessoa Física" : "Pessoa Jurídica");
    printf("  ║  CPF/CNPJ   : %-38s  ║\n", c->cpf);
    printf("  ║  RG/IE      : %-38s  ║\n", c->rg);
    if (c->tipo == 1)
    printf("  ║  Nascimento : %-38s  ║\n", c->data_nascimento);
    printf("  ╠══════════════════════════════════════════════════════╣\n");
    printf("  ║  E-mail     : %-38s  ║\n", c->email);
    printf("  ║  Telefone   : %-38s  ║\n", c->telefone);
    printf("  ║  Celular    : %-38s  ║\n", c->celular);
    printf("  ╠══════════════════════════════════════════════════════╣\n");
    printf("  ║  Endereço   : %-38s  ║\n", c->endereco);
    printf("  ║  Nº/Compl.  : %-10s / %-25s  ║\n", c->numero, c->complemento);
    printf("  ║  Bairro     : %-38s  ║\n", c->bairro);
    printf("  ║  Cidade/UF  : %-30s / %-5s  ║\n", c->cidade, c->estado);
    printf("  ║  CEP        : %-38s  ║\n", c->cep);
    printf("  ╠══════════════════════════════════════════════════════╣\n");
    printf("  ║  Lim. Crédito: R$ %-34.2f  ║\n", c->limite_credito);
    printf("  ║  Cadastrado  : %-38s  ║\n", c->data_cadastro);
    if (strlen(c->observacoes) > 0)
    printf("  ║  Obs.        : %-38s  ║\n", c->observacoes);
    printf("  ╚══════════════════════════════════════════════════════╝\n");
}

/* ==================== LISTAR CLIENTES ==================== */

void listar_clientes(int apenas_ativos) 
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════════════════════════╗\n");
    printf("  ║            LISTA DE CLIENTES%s            ║\n",
           apenas_ativos ? " (ATIVOS)                   " : "                            ");
    printf("  ╠═══════╦══════════════════════════════╦══════════════╦═════════╣\n");
    printf("  ║ CÓD.  ║ NOME                         ║ CPF/CNPJ     ║ STATUS  ║\n");
    printf("  ╠═══════╬══════════════════════════════╬══════════════╬═════════╣\n");

    FILE *f = fopen(ARQUIVO_DADOS, "rb");
    if (!f) 
    {
        printf("  ║  Nenhum registro encontrado.                                  ║\n");
        printf("  ╚═══════════════════════════════════════════════════════════════╝\n");
        pausar();
        return;
    }

    Cliente c;
    int count = 0;
    while (fread(&c, sizeof(Cliente), 1, f) == 1) 
    {
        if (apenas_ativos && !c.ativo) continue;
        char nome_trunc[31];
        strncpy(nome_trunc, c.nome, 30);
        nome_trunc[30] = '\0';
        printf("  ║ %-5d ║ %-28s   ║ %-12s ║ %-7s ║\n",
               c.codigo, nome_trunc, c.cpf,
               c.ativo ? "ATIVO" : "INATIVO");
        count++;
    }
    fclose(f);

    printf("  ╚═══════╩══════════════════════════════╩══════════════╩═════════╝\n");
    printf("  Total: %d cliente(s).\n", count);
    pausar();
}

/* ==================== BUSCAR CLIENTE ===================== */

int buscar_por_codigo(int codigo, Cliente *dest) 
{
    FILE *f = fopen(ARQUIVO_DADOS, "rb");
    if (!f) return 0;
    Cliente c;
    while (fread(&c, sizeof(Cliente), 1, f) == 1) 
    {
        if (c.codigo == codigo) 
        {
            *dest = c;
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void consultar_cliente(void) 
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║          CONSULTA DE CLIENTE             ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  Buscar por:\n");
    printf("    [1] Código\n");
    printf("    [2] Nome\n");
    printf("    [3] CPF / CNPJ\n");
    printf("  Opção: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    FILE *f = fopen(ARQUIVO_DADOS, "rb");
    if (!f) 
    {
        printf("\n  Nenhum registro encontrado.\n");
        pausar();
        return;
    }

    int encontrou = 0;
    Cliente c;

    if (op == 1) 
    {
        printf("  Código: ");
        int cod;
        scanf("%d", &cod);
        limpar_buffer();
        while (fread(&c, sizeof(Cliente), 1, f) == 1) 
        {
            if (c.codigo == cod) { exibir_ficha(&c); encontrou = 1; }
        }
    } else if (op == 2) 
    {
        char busca[MAX_NOME];
        ler_string("  Nome (parte): ", busca, MAX_NOME);
        /* converte busca para lowercase para comparação */
        char b_lower[MAX_NOME];
        for (int i = 0; busca[i]; i++) b_lower[i] = tolower((unsigned char)busca[i]);
        b_lower[strlen(busca)] = '\0';
        while (fread(&c, sizeof(Cliente), 1, f) == 1) 
        {
            char n_lower[MAX_NOME];
            for (int i = 0; c.nome[i]; i++) n_lower[i] = tolower((unsigned char)c.nome[i]);
            n_lower[strlen(c.nome)] = '\0';
            if (strstr(n_lower, b_lower)) { exibir_ficha(&c); encontrou = 1; }
        }
    } else if (op == 3) 
    {
        char busca[MAX_CPF];
        ler_string("  CPF/CNPJ: ", busca, MAX_CPF);
        while (fread(&c, sizeof(Cliente), 1, f) == 1) 
        {
            if (strstr(c.cpf, busca)) { exibir_ficha(&c); encontrou = 1; }
        }
    }

    fclose(f);
    if (!encontrou) printf("\n  Nenhum cliente encontrado.\n");
    pausar();
}

/* ==================== EDITAR CLIENTE ===================== */

void editar_cliente(void) 
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║          EDITAR CLIENTE                  ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  Código do cliente: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Cliente c;
    if (!buscar_por_codigo(cod, &c)) 
    {
        printf("  Cliente não encontrado!\n");
        pausar();
        return;
    }

    exibir_ficha(&c);
    printf("\n  O que deseja alterar?\n");
    printf("    [1] Nome\n");
    printf("    [2] CPF/CNPJ\n");
    printf("    [3] Contato (email/telefone/celular)\n");
    printf("    [4] Endereço completo\n");
    printf("    [5] Limite de crédito\n");
    printf("    [6] Observações\n");
    printf("    [7] Status (ativar/inativar)\n");
    printf("    [0] Cancelar\n");
    printf("  Opção: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    switch(op) 
    {
        case 1:
            ler_string("  Novo nome: ", c.nome, MAX_NOME);
            break;
        case 2:
            ler_string("  Novo CPF/CNPJ: ", c.cpf, MAX_CPF);
            break;
        case 3:
            ler_string("  Novo e-mail  : ", c.email, MAX_EMAIL);
            ler_string("  Novo telefone: ", c.telefone, MAX_TELEFONE);
            ler_string("  Novo celular : ", c.celular, MAX_TELEFONE);
            break;
        case 4:
            ler_string("  Logradouro  : ", c.endereco, MAX_ENDERECO);
            ler_string("  Número      : ", c.numero, 10);
            ler_string("  Complemento : ", c.complemento, 40);
            ler_string("  Bairro      : ", c.bairro, 50);
            ler_string("  Cidade      : ", c.cidade, MAX_CIDADE);
            ler_string("  Estado (UF) : ", c.estado, MAX_ESTADO);
            ler_string("  CEP         : ", c.cep, MAX_CEP);
            break;
        case 5:
            printf("  Novo limite (R$): ");
            scanf("%f", &c.limite_credito);
            limpar_buffer();
            break;
        case 6:
            ler_string("  Observações: ", c.observacoes, MAX_OBS);
            break;
        case 7:
            c.ativo = !c.ativo;
            printf("  Status alterado para: %s\n", c.ativo ? "ATIVO" : "INATIVO");
            break;
        default:
            printf("  Edição cancelada.\n");
            pausar();
            return;
    }

    /* Reescreve o registro no arquivo */
    FILE *f = fopen(ARQUIVO_DADOS, "r+b");
    if (!f) 
    {
        printf("  ERRO ao abrir arquivo!\n");
        pausar();
        return;
    }
    Cliente tmp;
    while (fread(&tmp, sizeof(Cliente), 1, f) == 1) 
    {
        if (tmp.codigo == cod) 
        {
            fseek(f, -(long)sizeof(Cliente), SEEK_CUR);
            fwrite(&c, sizeof(Cliente), 1, f);
            break;
        }
    }
    fclose(f);
    printf("\n  ✔ Dados atualizados com sucesso!\n");
    pausar();
}

/* ==================== EXCLUIR CLIENTE ==================== */

void excluir_cliente(void) 
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║         EXCLUIR / INATIVAR CLIENTE       ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  Código do cliente: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Cliente c;
    if (!buscar_por_codigo(cod, &c)) 
    {
        printf("  Cliente não encontrado!\n");
        pausar();
        return;
    }

    exibir_ficha(&c);

    printf("\n  [1] Inativar cliente (recomendado)\n");
    printf("  [2] Excluir definitivamente\n");
    printf("  [0] Cancelar\n");
    printf("  Opção: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    if (op == 1) 
    {
        c.ativo = 0;
        FILE *f = fopen(ARQUIVO_DADOS, "r+b");
        Cliente tmp;
        while (fread(&tmp, sizeof(Cliente), 1, f) == 1) 
        {
            if (tmp.codigo == cod) 
            {
                fseek(f, -(long)sizeof(Cliente), SEEK_CUR);
                fwrite(&c, sizeof(Cliente), 1, f);
                break;
            }
        }
        fclose(f);
        printf("\n  ✔ Cliente inativado.\n");
    } else if (op == 2) 
    {
        printf("  ATENÇÃO! Isso é irreversível. Confirma? [S/N]: ");
        char conf;
        scanf(" %c", &conf);
        limpar_buffer();
        if (toupper(conf) == 'S') 
        {
            /* Cria novo arquivo sem o registro */
            FILE *in  = fopen(ARQUIVO_DADOS, "rb");
            FILE *out = fopen("clientes_tmp.dat", "wb");
            if (!in || !out) 
            {
                printf("  ERRO ao processar arquivo!\n");
                if (in)  fclose(in);
                if (out) fclose(out);
                pausar();
                return;
            }
            Cliente tmp;
            while (fread(&tmp, sizeof(Cliente), 1, in) == 1)
                if (tmp.codigo != cod)
                    fwrite(&tmp, sizeof(Cliente), 1, out);
            fclose(in);
            fclose(out);
            remove(ARQUIVO_DADOS);
            rename("clientes_tmp.dat", ARQUIVO_DADOS);
            printf("\n  ✔ Cliente excluído definitivamente.\n");
        } else 
        {
            printf("  Exclusão cancelada.\n");
        }
    }
    pausar();
}

/* ==================== RELATÓRIO RESUMO =================== */

void relatorio_resumo(void) 
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║          RELATÓRIO RESUMIDO              ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    FILE *f = fopen(ARQUIVO_DADOS, "rb");
    if (!f) 
    {
        printf("  Nenhum registro.\n");
        pausar();
        return;
    }

    int total = 0, ativos = 0, inativos = 0, pf = 0, pj = 0;
    float limite_total = 0.0f;
    Cliente c;
    while (fread(&c, sizeof(Cliente), 1, f) == 1) 
    {
        total++;
        if (c.ativo) ativos++; else inativos++;
        if (c.tipo == 1) pf++; else pj++;
        limite_total += c.limite_credito;
    }
    fclose(f);

    printf("  Total de clientes   : %d\n", total);
    printf("  Clientes ativos     : %d\n", ativos);
    printf("  Clientes inativos   : %d\n", inativos);
    printf("  Pessoa Física (PF)  : %d\n", pf);
    printf("  Pessoa Jurídica (PJ): %d\n", pj);
    printf("  Limite total (R$)   : %.2f\n", limite_total);
    if (total > 0)
        printf("  Limite médio (R$)   : %.2f\n", limite_total / total);

    pausar();
}

/* ==================== EXPORTAÇÃO TXT ===================== */

/* Linha separadora no arquivo .txt */
static void sep_txt(FILE *f, char c, int n) //separar linhas
{
    for (int i = 0; i < n; i++) fputc(c, f);
    fputc('\n', f);
}

/* Grava a ficha completa de um cliente no arquivo .txt */
static void ficha_para_txt(FILE *f, const Cliente *c) //ficha do cliente em txt
{
    sep_txt(f, '=', 62);
    fprintf(f, "  FICHA DO CLIENTE\n");
    fprintf(f, "  Codigo      : %d\n", c->codigo);
    fprintf(f, "  Status      : %s\n", c->ativo ? "ATIVO" : "INATIVO");
    sep_txt(f, '-', 62);
    fprintf(f, "  DADOS PESSOAIS\n");
    fprintf(f, "  Nome        : %s\n", c->nome);
    fprintf(f, "  Tipo        : %s\n",
            c->tipo == 1 ? "Pessoa Fisica" : "Pessoa Juridica");
    fprintf(f, "  CPF/CNPJ    : %s\n", c->cpf);
    fprintf(f, "  RG/IE       : %s\n", c->rg);
    if (c->tipo == 1)
        fprintf(f, "  Nascimento  : %s\n", c->data_nascimento);
    sep_txt(f, '-', 62);
    fprintf(f, "  CONTATO\n");
    fprintf(f, "  E-mail      : %s\n", c->email);
    fprintf(f, "  Telefone    : %s\n", c->telefone);
    fprintf(f, "  Celular     : %s\n", c->celular);
    sep_txt(f, '-', 62);
    fprintf(f, "  ENDERECO\n");
    fprintf(f, "  Logradouro  : %s, %s\n", c->endereco, c->numero);
    if (strlen(c->complemento) > 0)
        fprintf(f, "  Complemento : %s\n", c->complemento);
    fprintf(f, "  Bairro      : %s\n", c->bairro);
    fprintf(f, "  Cidade/UF   : %s / %s\n", c->cidade, c->estado);
    fprintf(f, "  CEP         : %s\n", c->cep);
    sep_txt(f, '-', 62);
    fprintf(f, "  FINANCEIRO\n");
    fprintf(f, "  Lim.Credito : R$ %.2f\n", c->limite_credito);
    sep_txt(f, '-', 62);
    fprintf(f, "  Cadastro    : %s\n", c->data_cadastro);
    if (strlen(c->observacoes) > 0)
        fprintf(f, "  Observacoes : %s\n", c->observacoes);
    sep_txt(f, '=', 62);
    fputc('\n', f);
}

/* Função principal de exportação — chamada pelo menu [8] */
void exportar_txt(void)
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║         EXPORTAR PARA ARQUIVO .TXT       ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  O que deseja exportar?\n");
    printf("    [1] Ficha de um cliente especifico  ->  ficha_NNN.txt\n");
    printf("    [2] Lista completa (tabela)         ->  clientes_export.txt\n");
    printf("    [3] Relatorio gerencial completo    ->  clientes_export.txt\n");
    printf("    [0] Cancelar\n");
    printf("  Opcao: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    if (op == 0) return;

    /* Verifica se existem dados antes de continuar */
    FILE *dat = fopen(ARQUIVO_DADOS, "rb");
    if (!dat) 
    {
        printf("\n  Nenhum registro encontrado. Cadastre clientes primeiro.\n");
        pausar();
        return;
    }

    char nome_saida[80];
    strcpy(nome_saida, ARQUIVO_TXT); /* padrão para opções 2 e 3 */

    /* -------------------------------------------------- */
    /*  OPÇÃO 1 — ficha individual                        */
    /* -------------------------------------------------- */
    if (op == 1) 
    {
        printf("  Codigo do cliente: ");
        int cod;
        scanf("%d", &cod);
        limpar_buffer();

        Cliente c;
        int achou = 0;
        while (fread(&c, sizeof(Cliente), 1, dat) == 1) 
        {
            if (c.codigo == cod) { achou = 1; break; }
        }
        fclose(dat);

        if (!achou) 
        {
            printf("\n  Cliente nao encontrado.\n");
            pausar();
            return;
        }

        sprintf(nome_saida, "ficha_%03d.txt", c.codigo); /* ex: ficha_001.txt */

        FILE *txt = fopen(nome_saida, "w");
        if (!txt) 
        {
            printf("\n  ERRO: nao foi possivel criar o arquivo!\n");
            pausar();
            return;
        }

        char agora[20];
        data_hoje(agora);
        fprintf(txt, "SISTEMA DE CADASTRO DE CLIENTES v%s\n", VERSAO);
        fprintf(txt, "Gerado em: %s\n\n", agora);
        ficha_para_txt(txt, &c);
        fclose(txt);

        printf("\n  ✔ Arquivo gerado: %s\n", nome_saida);
        pausar();
        return;
    }

    /* -------------------------------------------------- */
    /*  OPÇÕES 2 e 3 — percorrem todos os registros       */
    /* -------------------------------------------------- */
    FILE *txt = fopen(nome_saida, "w");
    if (!txt) 
    {
        fclose(dat);
        printf("\n  ERRO: nao foi possivel criar o arquivo!\n");
        pausar();
        return;
    }

    char agora[20];
    data_hoje(agora);

    /* -------------------------------------------------- */
    /*  OPÇÃO 2 — lista em tabela                         */
    /* -------------------------------------------------- */
    if (op == 2) 
    {
        fprintf(txt, "SISTEMA DE CADASTRO DE CLIENTES v%s\n", VERSAO);
        fprintf(txt, "Relatorio  : LISTA COMPLETA DE CLIENTES\n");
        fprintf(txt, "Gerado em  : %s\n\n", agora);

        sep_txt(txt, '-', 78);
        fprintf(txt, "%-5s  %-30s  %-15s  %-12s  %-7s\n",
                "COD.", "NOME", "CPF/CNPJ", "CIDADE", "STATUS");
        sep_txt(txt, '-', 78);

        Cliente c;
        int count = 0;
        while (fread(&c, sizeof(Cliente), 1, dat) == 1) 
        {
            /* trunca campos longos para caber na tabela */
            char nome_t[31];  strncpy(nome_t, c.nome,   30); nome_t[30] = '\0';
            char cpf_t[16];   strncpy(cpf_t,  c.cpf,    15); cpf_t[15]  = '\0';
            char cid_t[13];   strncpy(cid_t,  c.cidade, 12); cid_t[12]  = '\0';

            fprintf(txt, "%-5d  %-30s  %-15s  %-12s  %-7s\n",
                    c.codigo, nome_t, cpf_t, cid_t,
                    c.ativo ? "ATIVO" : "INATIVO");
            count++;
        }
        sep_txt(txt, '-', 78);
        fprintf(txt, "Total: %d cliente(s).\n", count);
    }

    /* -------------------------------------------------- */
    /*  OPÇÃO 3 — relatório gerencial completo            */
    /* -------------------------------------------------- */
    else if (op == 3) 
    {
        fprintf(txt, "================================================================\n");
        fprintf(txt, "  SISTEMA DE CADASTRO DE CLIENTES v%s\n", VERSAO);
        fprintf(txt, "  RELATORIO GERENCIAL COMPLETO\n");
        fprintf(txt, "  Gerado em: %s\n", agora);
        fprintf(txt, "================================================================\n\n");

        /* Primeira passagem: coleta estatísticas */
        int total = 0, ativos = 0, inativos = 0, pf = 0, pj = 0;
        float lim_total = 0.0f, lim_max = 0.0f;
        char nome_maior[MAX_NOME] = "";
        Cliente c;

        while (fread(&c, sizeof(Cliente), 1, dat) == 1) 
        {
            total++;
            if (c.ativo) ativos++; else inativos++;
            if (c.tipo == 1) pf++; else pj++;
            lim_total += c.limite_credito;
            if (c.limite_credito > lim_max) 
            {
                lim_max = c.limite_credito;
                strncpy(nome_maior, c.nome, MAX_NOME - 1);
            }
        }

        /* Bloco de estatísticas */
        fprintf(txt, "RESUMO ESTATISTICO\n");
        sep_txt(txt, '-', 40);
        fprintf(txt, "  Total de clientes    : %d\n",    total);
        fprintf(txt, "  Clientes ativos      : %d\n",    ativos);
        fprintf(txt, "  Clientes inativos    : %d\n",    inativos);
        fprintf(txt, "  Pessoa Fisica  (PF)  : %d\n",    pf);
        fprintf(txt, "  Pessoa Juridica (PJ) : %d\n",    pj);
        fprintf(txt, "  Limite total   (R$)  : %.2f\n",  lim_total);
        if (total > 0) 
        {
            fprintf(txt, "  Limite medio   (R$)  : %.2f\n",  lim_total / total);
            fprintf(txt, "  Maior limite   (R$)  : %.2f  (%s)\n", lim_max, nome_maior);
        }
        fprintf(txt, "\n\n");

        /* Segunda passagem: fichas completas de todos */
        fprintf(txt, "FICHAS INDIVIDUAIS\n\n");
        rewind(dat);
        while (fread(&c, sizeof(Cliente), 1, dat) == 1)
            ficha_para_txt(txt, &c);
    }

    fclose(dat);
    fclose(txt);
    printf("\n  ✔ Arquivo gerado: %s\n", nome_saida);
    pausar();
}

/* ==================== MENU PRINCIPAL ===================== */

void menu_principal(void) 
{
    int opcao;
    do {
        limpar_tela();
        printf("\n");
        printf("  ╔══════════════════════════════════════════════╗\n");
        printf("  ║                                              ║\n");
        printf("  ║   ██████╗ ██╗     ██╗███████╗███╗  ██╗████╗ ║\n");
        printf("  ║  ██╔════╝ ██║     ██║██╔════╝████╗ ██║╚══██╗║\n");
        printf("  ║  ██║      ██║     ██║█████╗  ██╔██╗██║   ██║║\n");
        printf("  ║  ██║      ██║     ██║██╔══╝  ██║╚████║   ██║║\n");
        printf("  ║  ╚██████╗ ███████╗██║███████╗██║ ╚███║████╔╝║\n");
        printf("  ║   ╚═════╝ ╚══════╝╚═╝╚══════╝╚═╝  ╚══╝╚═══╝ ║\n");
        printf("  ║                                              ║\n");
        printf("  ║     SISTEMA DE CADASTRO DE CLIENTES v%-5s   ║\n", VERSAO);
        printf("  ║                                              ║\n");
        printf("  ╠══════════════════════════════════════════════╣\n");
        printf("  ║  [1] Cadastrar novo cliente                  ║\n");
        printf("  ║  [2] Consultar cliente                       ║\n");
        printf("  ║  [3] Editar cadastro                         ║\n");
        printf("  ║  [4] Excluir / Inativar cliente              ║\n");
        printf("  ╠══════════════════════════════════════════════╣\n");
        printf("  ║  [5] Listar todos os clientes                ║\n");
        printf("  ║  [6] Listar apenas ativos                    ║\n");
        printf("  ║  [7] Relatorio resumido                      ║\n");
        printf("  ║  [8] Exportar para arquivo .TXT              ║\n");
        printf("  ╠══════════════════════════════════════════════╣\n");
        printf("  ║  [0] Sair do sistema                         ║\n");
        printf("  ╚══════════════════════════════════════════════╝\n\n");
        printf("  Total de registros: %d\n\n", total_registros());
        printf("  Sua opcao: ");
        scanf("%d", &opcao);
        limpar_buffer();

        switch(opcao) 
        {
            case 1: cadastrar_cliente();  break;
            case 2: consultar_cliente();  break;
            case 3: editar_cliente();     break;
            case 4: excluir_cliente();    break;
            case 5: listar_clientes(0);   break;
            case 6: listar_clientes(1);   break;
            case 7: relatorio_resumo();   break;
            case 8: exportar_txt();       break;
            case 0:
                limpar_tela();
                printf("\n  Sistema encerrado. Ate logo!\n\n");
                break;
            default:
                printf("\n  Opcao invalida!\n");
                pausar();
        }
    } while (opcao != 0);
}

/* ==================== MAIN ================================ */

int main(void) 
{
    menu_principal();
    return 0;
}

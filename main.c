/* ================================================================
 *   MERCADINHO EXPRESS - PORTO DIGITAL
 *   Sistema unificado: PDV, Estoque, Clientes, Dashboard, Config
 *   Linguagem: C (ANSI C99)
 * ================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <locale.h>

/* ==================== CORES ANSI ========================== */
#define RED     "\x1b[31m"
#define YELLOW  "\x1b[33m"
#define GREEN   "\x1b[32m"
#define CYAN    "\x1b[36m"
#define BOLD    "\x1b[1m"
#define RESET   "\x1b[0m"

/* ==================== CONSTANTES GERAIS =================== */
#define VERSAO          "1.0.0"
#define LIMITE_CRITICO  3
#define LIMITE_ATENCAO  7

/* ----- Produtos / Estoque ----- */
#define MAX_NOME        60
#define MAX_CATEGORIA   35
#define MAX_UNIDADE     10
#define MAX_OBS         200
#define ARQUIVO_PRODUTOS "produtos.dat"
#define ARQUIVO_HIST     "historico.dat"
#define ARQUIVO_TXT_PROD "produtos_export.txt"

/* ----- Clientes ----- */
#define CLI_MAX_NOME      80
#define CLI_MAX_CPF       15
#define CLI_MAX_EMAIL     60
#define CLI_MAX_TELEFONE  20
#define CLI_MAX_ENDERECO  100
#define CLI_MAX_CIDADE    50
#define CLI_MAX_ESTADO    3
#define CLI_MAX_CEP       10
#define CLI_MAX_OBS       200
#define ARQUIVO_CLIENTES  "clientes.dat"
#define ARQUIVO_TXT_CLI   "clientes_export.txt"

/* ----- Vendas (PDV) ----- */
#define ARQUIVO_VENDAS    "vendas.dat"
#define MAX_ITENS_VENDA   50

/* ==================== ESTRUTURAS ========================== */

typedef struct
{
    int   codigo;
    char  nome[MAX_NOME];
    char  categoria[MAX_CATEGORIA];
    char  unidade[MAX_UNIDADE];
    float peso_volume;
    int   quantidade;
    float preco_custo;
    float preco_venda;
    int   dia_val, mes_val, ano_val;
    int   dias_restantes;
    int   prioridade;
    int   ativo;
    char  data_cadastro[20];
    char  observacoes[MAX_OBS];
} Produto;

typedef struct
{
    int  codigo_produto;
    char tipo_acao[15];
    int  quantidade;
    char data_hora[25];
} Movimentacao;

typedef struct
{
    int    codigo;
    char   nome[CLI_MAX_NOME];
    char   cpf[CLI_MAX_CPF];
    char   rg[CLI_MAX_CPF];
    char   data_nascimento[12];
    char   email[CLI_MAX_EMAIL];
    char   telefone[CLI_MAX_TELEFONE];
    char   celular[CLI_MAX_TELEFONE];
    char   endereco[CLI_MAX_ENDERECO];
    char   numero[10];
    char   complemento[40];
    char   bairro[50];
    char   cidade[CLI_MAX_CIDADE];
    char   estado[CLI_MAX_ESTADO];
    char   cep[CLI_MAX_CEP];
    float  limite_credito;
    int    tipo;     /* 1=PF 2=PJ */
    int    ativo;
    char   data_cadastro[20];
    char   observacoes[CLI_MAX_OBS];
} Cliente;

/* Registro simples de venda (cabecalho da venda) */
typedef struct
{
    int   numero_venda;
    int   codigo_cliente;   /* 0 = sem cliente identificado */
    float total;
    char  data_hora[25];
} Venda;

/* ==================== UTILITARIOS GERAIS =================== */

void limpar_tela(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pausar(void)
{
    printf("\n  Pressione ENTER para continuar...");
    getchar();
}

void limpar_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void ler_string(const char *prompt, char *dest, int max)
{
    printf("%s", prompt);
    fgets(dest, max, stdin);
    int len = strlen(dest);
    if (len > 0 && dest[len - 1] == '\n')
        dest[len - 1] = '\0';
}

void data_hoje(char *buf)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, 20, "%d/%m/%Y %H:%M", tm_info);
}

void data_hora_agora(char *buf)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, 25, "%d/%m/%Y %H:%M:%S", tm_info);
}

static void sep_txt(FILE *f, char c, int n)
{
    for (int i = 0; i < n; i++) fputc(c, f);
    fputc('\n', f);
}

/* ===================================================================
 * =====================   MODULO DE PRODUTOS   ======================
 * =================================================================== */

/* ----- Validade / prioridade ----- */

int calcular_dias(int dia, int mes, int ano)
{
    time_t agora = time(NULL);
    struct tm t  = {0};
    t.tm_mday    = dia;
    t.tm_mon     = mes - 1;
    t.tm_year    = ano - 1900;
    time_t validade = mktime(&t);
    double diff  = difftime(validade, agora);
    return (int)(diff / 86400);
}

void classificar(Produto *p)
{
    int dias = calcular_dias(p->dia_val, p->mes_val, p->ano_val);
    p->dias_restantes = dias;
    if      (dias <= 0)              p->prioridade = 0;
    else if (dias <= LIMITE_CRITICO) p->prioridade = 1;
    else if (dias <= LIMITE_ATENCAO) p->prioridade = 2;
    else                             p->prioridade = 3;
}

const char *label_prioridade(int p)
{
    switch (p)
    {
        case 0: return "VENCIDO";
        case 1: return "CRITICO";
        case 2: return "ATENCAO";
        default: return "OK";
    }
}

const char *cor_prioridade(int p)
{
    switch (p)
    {
        case 0: return RED;
        case 1: return YELLOW;
        case 2: return YELLOW;
        default: return GREEN;
    }
}

/* ----- Persistencia ----- */

int proximo_codigo_produto(void)
{
    FILE *f = fopen(ARQUIVO_PRODUTOS, "rb");
    if (!f) return 1;
    int max = 0;
    Produto p;
    while (fread(&p, sizeof(Produto), 1, f) == 1)
        if (p.codigo > max) max = p.codigo;
    fclose(f);
    return max + 1;
}

int total_registros_produto(void)
{
    FILE *f = fopen(ARQUIVO_PRODUTOS, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fclose(f);
    return (int)(tam / sizeof(Produto));
}

int buscar_produto_por_codigo(int codigo, Produto *dest)
{
    FILE *f = fopen(ARQUIVO_PRODUTOS, "rb");
    if (!f) return 0;
    Produto p;
    while (fread(&p, sizeof(Produto), 1, f) == 1)
    {
        if (p.codigo == codigo)
        {
            *dest = p;
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

/* Atualiza um produto inteiro no arquivo (regrava o registro) */
int salvar_produto(const Produto *p)
{
    FILE *f = fopen(ARQUIVO_PRODUTOS, "r+b");
    if (!f) return 0;
    Produto tmp;
    int ok = 0;
    while (fread(&tmp, sizeof(Produto), 1, f) == 1)
    {
        if (tmp.codigo == p->codigo)
        {
            fseek(f, -(long)sizeof(Produto), SEEK_CUR);
            fwrite(p, sizeof(Produto), 1, f);
            ok = 1;
            break;
        }
    }
    fclose(f);
    return ok;
}

/* ----- Historico ----- */

void registrar_movimentacao(int cod, const char *tipo, int qtd)
{
    Movimentacao m;
    m.codigo_produto = cod;
    strncpy(m.tipo_acao, tipo, sizeof(m.tipo_acao) - 1);
    m.tipo_acao[sizeof(m.tipo_acao) - 1] = '\0';
    m.quantidade = qtd;
    data_hora_agora(m.data_hora);

    FILE *f = fopen(ARQUIVO_HIST, "ab");
    if (!f) return;
    fwrite(&m, sizeof(Movimentacao), 1, f);
    fclose(f);
}

void consultar_historico(void)
{
    limpar_tela();
    printf("\n  ================= HISTORICO DE MOVIMENTACOES =================\n");
    FILE *f = fopen(ARQUIVO_HIST, "rb");
    if (!f)
    {
        printf("  Nenhuma movimentacao registrada.\n");
        pausar();
        return;
    }
    printf("  %-20s | %-10s | %-12s | %-5s\n", "Data/Hora", "Cod.Prod", "Acao", "Qtd");
    printf("  ---------------------------------------------------------------\n");
    Movimentacao m;
    int count = 0;
    while (fread(&m, sizeof(Movimentacao), 1, f) == 1)
    {
        printf("  %-20s | %-10d | %-12s | %-5d\n",
               m.data_hora, m.codigo_produto, m.tipo_acao, m.quantidade);
        count++;
    }
    fclose(f);
    printf("  ---------------------------------------------------------------\n");
    printf("  Total: %d movimentacao(oes).\n", count);
    pausar();
}

/* ----- Ficha ----- */

void exibir_ficha_produto(const Produto *p)
{
    Produto tmp = *p;
    classificar(&tmp);
    const char *cor = cor_prioridade(tmp.prioridade);
    const char *lbl = label_prioridade(tmp.prioridade);

    printf("\n  +--------------------------------------------------------+\n");
    printf("  | FICHA DO PRODUTO  - Cod: %-5d  Status: %-8s     |\n",
           tmp.codigo, tmp.ativo ? "ATIVO" : "INATIVO");
    printf("  +--------------------------------------------------------+\n");
    printf("  | Nome       : %-43s|\n", tmp.nome);
    printf("  | Categoria  : %-43s|\n", tmp.categoria);
    printf("  | Unidade    : %-10s   Peso/Vol : %-18.3f|\n", tmp.unidade, tmp.peso_volume);
    printf("  +--------------------------------------------------------+\n");
    printf("  | Quantidade em estoque : %-32d|\n", tmp.quantidade);
    printf("  | Preco de custo  (R$)  : %-32.2f|\n", tmp.preco_custo);
    printf("  | Preco de venda  (R$)  : %-32.2f|\n", tmp.preco_venda);
    printf("  +--------------------------------------------------------+\n");
    printf("  | Validade : %02d/%02d/%04d   Situacao: %s%-7s" RESET "          |\n",
           tmp.dia_val, tmp.mes_val, tmp.ano_val, cor, lbl);
    printf("  | Dias restantes : %-39d|\n", tmp.dias_restantes);
    printf("  +--------------------------------------------------------+\n");
    printf("  | Cadastrado : %-43s|\n", tmp.data_cadastro);
    if (strlen(tmp.observacoes) > 0)
        printf("  | Obs.       : %-43s|\n", tmp.observacoes);
    printf("  +--------------------------------------------------------+\n");
}

/* ----- Cadastrar ----- */

void cadastrar_produto(void)
{
    limpar_tela();
    printf("\n  ====== CADASTRO DE NOVO PRODUTO ======\n\n");

    Produto p;
    memset(&p, 0, sizeof(Produto));

    p.codigo = proximo_codigo_produto();
    p.ativo  = 1;
    data_hoje(p.data_cadastro);

    printf("  Codigo gerado automaticamente: %d\n\n", p.codigo);

    printf("  --- IDENTIFICACAO ---\n");
    ler_string("  Nome do produto  : ", p.nome, MAX_NOME);
    if (strlen(p.nome) == 0)
    {
        printf("  Nome e obrigatorio!\n");
        pausar();
        return;
    }
    ler_string("  Categoria        : ", p.categoria, MAX_CATEGORIA);
    ler_string("  Unidade (UN/KG/L): ", p.unidade, MAX_UNIDADE);
    printf("  Peso / Volume (por unidade): ");
    scanf("%f", &p.peso_volume);
    limpar_buffer();

    printf("\n  --- ESTOQUE ---\n");
    printf("  Quantidade inicial: ");
    scanf("%d", &p.quantidade);
    limpar_buffer();

    printf("\n  --- PRECOS ---\n");
    printf("  Preco de custo  (R$): ");
    scanf("%f", &p.preco_custo);
    limpar_buffer();
    printf("  Preco de venda  (R$): ");
    scanf("%f", &p.preco_venda);
    limpar_buffer();

    printf("\n  --- VALIDADE ---\n");
    printf("  Data de validade (DD MM AAAA): ");
    scanf("%d %d %d", &p.dia_val, &p.mes_val, &p.ano_val);
    limpar_buffer();
    classificar(&p);

    printf("\n  --- OBSERVACOES ---\n");
    ler_string("  Observacoes (Enter p/ pular): ", p.observacoes, MAX_OBS);

    exibir_ficha_produto(&p);
    printf("\n  Confirmar cadastro? [S/N]: ");
    char conf;
    scanf(" %c", &conf);
    limpar_buffer();

    if (toupper(conf) == 'S')
    {
        FILE *f = fopen(ARQUIVO_PRODUTOS, "ab");
        if (!f)
        {
            printf("  ERRO: nao foi possivel abrir o arquivo!\n");
            pausar();
            return;
        }
        fwrite(&p, sizeof(Produto), 1, f);
        fclose(f);

        registrar_movimentacao(p.codigo, "CADASTRO", p.quantidade);
        printf("\n  " GREEN "Produto cadastrado com sucesso! Codigo: %d" RESET "\n", p.codigo);
    }
    else
    {
        printf("\n  Cadastro cancelado.\n");
    }
    pausar();
}

/* ----- Listar ----- */

void listar_produtos(int apenas_ativos)
{
    limpar_tela();
    printf("\n  ================== LISTA DE PRODUTOS %s ==================\n",
           apenas_ativos ? "(ATIVOS)" : "");
    printf("  %-4s %-18s %-15s %-5s %-10s %-9s\n",
           "COD", "NOME", "CATEGORIA", "QTD", "VDA(R$)", "SITUACAO");
    printf("  ----------------------------------------------------------------\n");

    FILE *f = fopen(ARQUIVO_PRODUTOS, "rb");
    if (!f)
    {
        printf("  Nenhum produto cadastrado.\n");
        pausar();
        return;
    }

    Produto p;
    int count = 0;
    while (fread(&p, sizeof(Produto), 1, f) == 1)
    {
        if (apenas_ativos && !p.ativo) continue;
        classificar(&p);

        char nome_t[19];  strncpy(nome_t, p.nome,      18); nome_t[18] = '\0';
        char cat_t[16];   strncpy(cat_t,  p.categoria, 15); cat_t[15]  = '\0';
        const char *cor = cor_prioridade(p.prioridade);
        const char *lbl = label_prioridade(p.prioridade);

        printf("  %-4d %-18s %-15s %-5d %-10.2f %s%-8s" RESET "\n",
               p.codigo, nome_t, cat_t, p.quantidade, p.preco_venda, cor, lbl);
        count++;
    }
    fclose(f);

    printf("  ----------------------------------------------------------------\n");
    printf("  Total: %d produto(s).\n", count);
    pausar();
}

/* ----- Consultar ----- */

void consultar_produto(void)
{
    limpar_tela();
    printf("\n  ====== CONSULTA DE PRODUTO ======\n\n");
    printf("  Buscar por:\n");
    printf("    [1] Codigo\n");
    printf("    [2] Nome\n");
    printf("    [3] Categoria\n");
    printf("  Opcao: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    FILE *f = fopen(ARQUIVO_PRODUTOS, "rb");
    if (!f)
    {
        printf("\n  Nenhum registro encontrado.\n");
        pausar();
        return;
    }

    int encontrou = 0;
    Produto p;

    if (op == 1)
    {
        printf("  Codigo: ");
        int cod;
        scanf("%d", &cod);
        limpar_buffer();
        while (fread(&p, sizeof(Produto), 1, f) == 1)
            if (p.codigo == cod) { exibir_ficha_produto(&p); encontrou = 1; }
    }
    else if (op == 2)
    {
        char busca[MAX_NOME];
        ler_string("  Nome (parte): ", busca, MAX_NOME);
        char b_low[MAX_NOME];
        for (int i = 0; busca[i]; i++) b_low[i] = tolower((unsigned char)busca[i]);
        b_low[strlen(busca)] = '\0';

        while (fread(&p, sizeof(Produto), 1, f) == 1)
        {
            char n_low[MAX_NOME];
            for (int i = 0; p.nome[i]; i++) n_low[i] = tolower((unsigned char)p.nome[i]);
            n_low[strlen(p.nome)] = '\0';
            if (strstr(n_low, b_low)) { exibir_ficha_produto(&p); encontrou = 1; }
        }
    }
    else if (op == 3)
    {
        char busca[MAX_CATEGORIA];
        ler_string("  Categoria (parte): ", busca, MAX_CATEGORIA);
        char b_low[MAX_CATEGORIA];
        for (int i = 0; busca[i]; i++) b_low[i] = tolower((unsigned char)busca[i]);
        b_low[strlen(busca)] = '\0';

        while (fread(&p, sizeof(Produto), 1, f) == 1)
        {
            char c_low[MAX_CATEGORIA];
            for (int i = 0; p.categoria[i]; i++) c_low[i] = tolower((unsigned char)p.categoria[i]);
            c_low[strlen(p.categoria)] = '\0';
            if (strstr(c_low, b_low)) { exibir_ficha_produto(&p); encontrou = 1; }
        }
    }

    fclose(f);
    if (!encontrou) printf("\n  Nenhum produto encontrado.\n");
    pausar();
}

/* ----- Editar ----- */

void editar_produto(void)
{
    limpar_tela();
    printf("\n  ====== EDITAR PRODUTO ======\n\n");

    printf("  Codigo do produto: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Produto p;
    if (!buscar_produto_por_codigo(cod, &p))
    {
        printf("  Produto nao encontrado!\n");
        pausar();
        return;
    }

    exibir_ficha_produto(&p);

    printf("\n  O que deseja alterar?\n");
    printf("    [1] Nome\n");
    printf("    [2] Categoria\n");
    printf("    [3] Unidade / Peso-Volume\n");
    printf("    [4] Precos (custo e venda)\n");
    printf("    [5] Data de validade\n");
    printf("    [6] Observacoes\n");
    printf("    [7] Status (ativar / inativar)\n");
    printf("    [0] Cancelar\n");
    printf("  Opcao: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    switch (op)
    {
        case 1:
            ler_string("  Novo nome: ", p.nome, MAX_NOME);
            break;
        case 2:
            ler_string("  Nova categoria: ", p.categoria, MAX_CATEGORIA);
            break;
        case 3:
            ler_string("  Nova unidade: ", p.unidade, MAX_UNIDADE);
            printf("  Novo peso/volume: ");
            scanf("%f", &p.peso_volume);
            limpar_buffer();
            break;
        case 4:
            printf("  Novo preco de custo  (R$): ");
            scanf("%f", &p.preco_custo);
            limpar_buffer();
            printf("  Novo preco de venda  (R$): ");
            scanf("%f", &p.preco_venda);
            limpar_buffer();
            break;
        case 5:
            printf("  Nova validade (DD MM AAAA): ");
            scanf("%d %d %d", &p.dia_val, &p.mes_val, &p.ano_val);
            limpar_buffer();
            classificar(&p);
            break;
        case 6:
            ler_string("  Observacoes: ", p.observacoes, MAX_OBS);
            break;
        case 7:
            p.ativo = !p.ativo;
            printf("  Status alterado para: %s\n", p.ativo ? "ATIVO" : "INATIVO");
            break;
        default:
            printf("  Edicao cancelada.\n");
            pausar();
            return;
    }

    salvar_produto(&p);
    registrar_movimentacao(p.codigo, "EDICAO", 0);
    printf("\n  " GREEN "Produto atualizado com sucesso!" RESET "\n");
    pausar();
}

/* ----- Excluir ----- */

void excluir_produto(void)
{
    limpar_tela();
    printf("\n  ====== EXCLUIR / INATIVAR PRODUTO ======\n\n");

    printf("  Codigo do produto: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Produto p;
    if (!buscar_produto_por_codigo(cod, &p))
    {
        printf("  Produto nao encontrado!\n");
        pausar();
        return;
    }

    exibir_ficha_produto(&p);

    printf("\n  [1] Inativar produto (recomendado)\n");
    printf("  [2] Excluir definitivamente\n");
    printf("  [0] Cancelar\n");
    printf("  Opcao: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    if (op == 1)
    {
        p.ativo = 0;
        salvar_produto(&p);
        registrar_movimentacao(cod, "INATIVADO", p.quantidade);
        printf("\n  " YELLOW "Produto inativado." RESET "\n");
    }
    else if (op == 2)
    {
        printf("  ATENCAO! Isso e irreversivel. Confirma? [S/N]: ");
        char conf;
        scanf(" %c", &conf);
        limpar_buffer();

        if (toupper(conf) == 'S')
        {
            FILE *in  = fopen(ARQUIVO_PRODUTOS, "rb");
            FILE *out = fopen("produtos_tmp.dat", "wb");
            if (!in || !out)
            {
                printf("  ERRO ao processar arquivo!\n");
                if (in)  fclose(in);
                if (out) fclose(out);
                pausar();
                return;
            }
            Produto tmp;
            while (fread(&tmp, sizeof(Produto), 1, in) == 1)
                if (tmp.codigo != cod)
                    fwrite(&tmp, sizeof(Produto), 1, out);
            fclose(in);
            fclose(out);
            remove(ARQUIVO_PRODUTOS);
            rename("produtos_tmp.dat", ARQUIVO_PRODUTOS);
            registrar_movimentacao(cod, "EXCLUSAO", p.quantidade);
            printf("\n  " RED "Produto excluido definitivamente." RESET "\n");
        }
        else
        {
            printf("  Exclusao cancelada.\n");
        }
    }
    pausar();
}

/* ----- Movimentar estoque (entrada/saida) ----- */

void movimentar_estoque(void)
{
    limpar_tela();
    printf("\n  ====== MOVIMENTAR ESTOQUE (ENTRADA/SAIDA) ======\n\n");

    printf("  Codigo do produto: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Produto p;
    if (!buscar_produto_por_codigo(cod, &p))
    {
        printf("  Produto nao encontrado!\n");
        pausar();
        return;
    }

    printf("\n  Produto      : %s\n", p.nome);
    printf("  Estoque atual: %d %s\n\n", p.quantidade, p.unidade);
    printf("  [1] Registrar ENTRADA (+)\n");
    printf("  [2] Registrar SAIDA   (-)\n");
    printf("  [0] Cancelar\n");
    printf("  Opcao: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    if (op == 0)
    {
        printf("\n  Operacao cancelada.\n");
        pausar();
        return;
    }

    if (op != 1 && op != 2)
    {
        printf("\n  Opcao invalida.\n");
        pausar();
        return;
    }

    int qtd;
    printf("  Quantidade: ");
    scanf("%d", &qtd);
    limpar_buffer();

    if (qtd <= 0)
    {
        printf("\n  Quantidade invalida!\n");
        pausar();
        return;
    }

    if (op == 1)
    {
        p.quantidade += qtd;
        salvar_produto(&p);
        registrar_movimentacao(cod, "ENTRADA", qtd);
        printf("\n  " GREEN "Entrada registrada! Novo saldo: %d" RESET "\n", p.quantidade);
    }
    else /* op == 2 */
    {
        if (qtd > p.quantidade)
        {
            printf("\n  " RED "Quantidade insuficiente em estoque!" RESET "\n");
        }
        else
        {
            p.quantidade -= qtd;
            salvar_produto(&p);
            registrar_movimentacao(cod, "SAIDA", qtd);
            printf("\n  " GREEN "Saida registrada! Novo saldo: %d" RESET "\n", p.quantidade);
        }
    }
    pausar();
}

/* ----- Exportar para TXT ----- */

void exportar_produtos_txt(void)
{
    limpar_tela();
    printf("\n  ====== EXPORTAR PRODUTOS PARA .TXT ======\n\n");

    FILE *dat = fopen(ARQUIVO_PRODUTOS, "rb");
    if (!dat)
    {
        printf("  Nenhum registro encontrado.\n");
        pausar();
        return;
    }

    FILE *txt = fopen(ARQUIVO_TXT_PROD, "w");
    if (!txt)
    {
        fclose(dat);
        printf("  ERRO ao criar arquivo!\n");
        pausar();
        return;
    }

    char agora[20];
    data_hoje(agora);
    fprintf(txt, "MERCADINHO EXPRESS - PORTO DIGITAL v%s\n", VERSAO);
    fprintf(txt, "Relatorio  : LISTA COMPLETA DE PRODUTOS\n");
    fprintf(txt, "Gerado em  : %s\n\n", agora);

    sep_txt(txt, '-', 70);
    fprintf(txt, "%-5s %-20s %-15s %-6s %-10s %-8s\n",
            "COD", "NOME", "CATEGORIA", "QTD", "VDA(R$)", "STATUS");
    sep_txt(txt, '-', 70);

    Produto p;
    int count = 0;
    while (fread(&p, sizeof(Produto), 1, dat) == 1)
    {
        classificar(&p);
        fprintf(txt, "%-5d %-20s %-15s %-6d %-10.2f %-8s\n",
                p.codigo, p.nome, p.categoria, p.quantidade, p.preco_venda,
                p.ativo ? label_prioridade(p.prioridade) : "INATIVO");
        count++;
    }
    sep_txt(txt, '-', 70);
    fprintf(txt, "Total: %d produto(s).\n", count);

    fclose(dat);
    fclose(txt);
    printf("\n  " GREEN "Arquivo gerado: %s" RESET "\n", ARQUIVO_TXT_PROD);
    pausar();
}

/* ----- Menu de estoque ----- */

void menu_estoque(void)
{
    int op;
    do {
        limpar_tela();
        printf("\n  ============================================\n");
        printf("       CONTROLE DE ESTOQUE - PORTO DIGITAL\n");
        printf("  ============================================\n");
        printf("  [1] Cadastrar produto\n");
        printf("  [2] Listar todos os produtos\n");
        printf("  [3] Listar apenas ativos\n");
        printf("  [4] Consultar produto\n");
        printf("  [5] Editar produto\n");
        printf("  [6] Excluir / inativar produto\n");
        printf("  [7] Movimentar estoque (entrada/saida)\n");
        printf("  [8] Consultar historico (auditoria)\n");
        printf("  [9] Exportar para .TXT\n");
        printf("  [0] Voltar ao menu principal\n");
        printf("  ============================================\n");
        printf("  Total de produtos cadastrados: %d\n", total_registros_produto());
        printf("  Opcao: ");
        scanf("%d", &op);
        limpar_buffer();

        switch (op)
        {
            case 1: cadastrar_produto();      break;
            case 2: listar_produtos(0);       break;
            case 3: listar_produtos(1);       break;
            case 4: consultar_produto();      break;
            case 5: editar_produto();         break;
            case 6: excluir_produto();        break;
            case 7: movimentar_estoque();     break;
            case 8: consultar_historico();    break;
            case 9: exportar_produtos_txt();  break;
            case 0: break;
            default:
                printf("\n  Opcao invalida!\n");
                pausar();
        }
    } while (op != 0);
}

/* ===================================================================
 * =====================   MODULO DE CLIENTES   ======================
 * =================================================================== */

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

int proximo_codigo_cliente(void)
{
    FILE *f = fopen(ARQUIVO_CLIENTES, "rb");
    if (!f) return 1;
    int max = 0;
    Cliente c;
    while (fread(&c, sizeof(Cliente), 1, f) == 1)
        if (c.codigo > max) max = c.codigo;
    fclose(f);
    return max + 1;
}

int total_registros_cliente(void)
{
    FILE *f = fopen(ARQUIVO_CLIENTES, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fclose(f);
    return (int)(tam / sizeof(Cliente));
}

int buscar_cliente_por_codigo(int codigo, Cliente *dest)
{
    FILE *f = fopen(ARQUIVO_CLIENTES, "rb");
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

int salvar_cliente(const Cliente *c)
{
    FILE *f = fopen(ARQUIVO_CLIENTES, "r+b");
    if (!f) return 0;
    Cliente tmp;
    int ok = 0;
    while (fread(&tmp, sizeof(Cliente), 1, f) == 1)
    {
        if (tmp.codigo == c->codigo)
        {
            fseek(f, -(long)sizeof(Cliente), SEEK_CUR);
            fwrite(c, sizeof(Cliente), 1, f);
            ok = 1;
            break;
        }
    }
    fclose(f);
    return ok;
}

/* ----- Cadastrar cliente ----- */

void cadastrar_cliente(void)
{
    limpar_tela();
    printf("\n  ====== CADASTRO DE NOVO CLIENTE ======\n\n");

    Cliente c;
    memset(&c, 0, sizeof(Cliente));

    c.codigo = proximo_codigo_cliente();
    c.ativo  = 1;
    data_hoje(c.data_cadastro);

    printf("  Codigo gerado automaticamente: %d\n\n", c.codigo);

    printf("  Tipo de cliente:\n");
    printf("    [1] Pessoa Fisica\n");
    printf("    [2] Pessoa Juridica\n");
    printf("  Opcao: ");
    scanf("%d", &c.tipo);
    limpar_buffer();
    if (c.tipo != 1 && c.tipo != 2) c.tipo = 1;

    printf("\n  --- DADOS PESSOAIS ---\n");
    ler_string("  Nome completo : ", c.nome, CLI_MAX_NOME);
    if (strlen(c.nome) == 0)
    {
        printf("  Nome e obrigatorio!\n");
        pausar();
        return;
    }

    if (c.tipo == 1)
    {
        char cpf_raw[CLI_MAX_CPF];
        ler_string("  CPF (somente numeros): ", cpf_raw, CLI_MAX_CPF);
        formatar_cpf(cpf_raw, c.cpf);
        ler_string("  RG                   : ", c.rg, CLI_MAX_CPF);
        ler_string("  Data de nascimento   : ", c.data_nascimento, 12);
    }
    else
    {
        ler_string("  CNPJ                 : ", c.cpf, CLI_MAX_CPF);
        ler_string("  Inscricao Estadual   : ", c.rg, CLI_MAX_CPF);
    }

    printf("\n  --- DADOS DE CONTATO ---\n");
    ler_string("  E-mail    : ", c.email, CLI_MAX_EMAIL);
    ler_string("  Telefone  : ", c.telefone, CLI_MAX_TELEFONE);
    ler_string("  Celular   : ", c.celular, CLI_MAX_TELEFONE);

    printf("\n  --- ENDERECO ---\n");
    ler_string("  Logradouro  : ", c.endereco, CLI_MAX_ENDERECO);
    ler_string("  Numero      : ", c.numero, 10);
    ler_string("  Complemento : ", c.complemento, 40);
    ler_string("  Bairro      : ", c.bairro, 50);
    ler_string("  Cidade      : ", c.cidade, CLI_MAX_CIDADE);
    ler_string("  Estado (UF) : ", c.estado, CLI_MAX_ESTADO);
    ler_string("  CEP         : ", c.cep, CLI_MAX_CEP);

    printf("\n  --- DADOS FINANCEIROS ---\n");
    printf("  Limite de credito (R$): ");
    scanf("%f", &c.limite_credito);
    limpar_buffer();

    printf("\n  --- OBSERVACOES ---\n");
    ler_string("  Observacoes: ", c.observacoes, CLI_MAX_OBS);

    printf("\n  Confirmar cadastro? [S/N]: ");
    char conf;
    scanf(" %c", &conf);
    limpar_buffer();

    if (toupper(conf) == 'S')
    {
        FILE *f = fopen(ARQUIVO_CLIENTES, "ab");
        if (!f)
        {
            printf("  ERRO: nao foi possivel abrir o arquivo!\n");
            pausar();
            return;
        }
        fwrite(&c, sizeof(Cliente), 1, f);
        fclose(f);
        printf("\n  " GREEN "Cliente cadastrado com sucesso! Codigo: %d" RESET "\n", c.codigo);
    }
    else
    {
        printf("\n  Cadastro cancelado.\n");
    }
    pausar();
}

/* ----- Ficha ----- */

void exibir_ficha_cliente(const Cliente *c)
{
    printf("\n  +----------------------------------------------------------+\n");
    printf("  | FICHA DO CLIENTE  - Cod: %-5d  Status: %-8s        |\n",
           c->codigo, c->ativo ? "ATIVO" : "INATIVO");
    printf("  +----------------------------------------------------------+\n");
    printf("  | Nome       : %-46s|\n", c->nome);
    printf("  | Tipo       : %-46s|\n", c->tipo == 1 ? "Pessoa Fisica" : "Pessoa Juridica");
    printf("  | CPF/CNPJ   : %-46s|\n", c->cpf);
    printf("  | RG/IE      : %-46s|\n", c->rg);
    if (c->tipo == 1)
        printf("  | Nascimento : %-46s|\n", c->data_nascimento);
    printf("  +----------------------------------------------------------+\n");
    printf("  | E-mail     : %-46s|\n", c->email);
    printf("  | Telefone   : %-46s|\n", c->telefone);
    printf("  | Celular    : %-46s|\n", c->celular);
    printf("  +----------------------------------------------------------+\n");
    printf("  | Endereco   : %-46s|\n", c->endereco);
    printf("  | Bairro     : %-46s|\n", c->bairro);
    printf("  | Cidade/UF  : %s / %-3s%37s|\n", c->cidade, c->estado, "");
    printf("  | CEP        : %-46s|\n", c->cep);
    printf("  +----------------------------------------------------------+\n");
    printf("  | Lim. Credito (R$): %-40.2f|\n", c->limite_credito);
    printf("  | Cadastrado       : %-40s|\n", c->data_cadastro);
    if (strlen(c->observacoes) > 0)
        printf("  | Obs.             : %-40s|\n", c->observacoes);
    printf("  +----------------------------------------------------------+\n");
}

/* ----- Listar ----- */

void listar_clientes(int apenas_ativos)
{
    limpar_tela();
    printf("\n  ================== LISTA DE CLIENTES %s ==================\n",
           apenas_ativos ? "(ATIVOS)" : "");
    printf("  %-5s %-30s %-14s %-7s\n", "COD", "NOME", "CPF/CNPJ", "STATUS");
    printf("  ----------------------------------------------------------------\n");

    FILE *f = fopen(ARQUIVO_CLIENTES, "rb");
    if (!f)
    {
        printf("  Nenhum registro encontrado.\n");
        pausar();
        return;
    }

    Cliente c;
    int count = 0;
    while (fread(&c, sizeof(Cliente), 1, f) == 1)
    {
        if (apenas_ativos && !c.ativo) continue;
        char nome_t[31];
        strncpy(nome_t, c.nome, 30);
        nome_t[30] = '\0';
        printf("  %-5d %-30s %-14s %-7s\n",
               c.codigo, nome_t, c.cpf, c.ativo ? "ATIVO" : "INATIVO");
        count++;
    }
    fclose(f);

    printf("  ----------------------------------------------------------------\n");
    printf("  Total: %d cliente(s).\n", count);
    pausar();
}

/* ----- Consultar ----- */

void consultar_cliente(void)
{
    limpar_tela();
    printf("\n  ====== CONSULTA DE CLIENTE ======\n\n");

    printf("  Buscar por:\n");
    printf("    [1] Codigo\n");
    printf("    [2] Nome\n");
    printf("    [3] CPF / CNPJ\n");
    printf("  Opcao: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    FILE *f = fopen(ARQUIVO_CLIENTES, "rb");
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
        printf("  Codigo: ");
        int cod;
        scanf("%d", &cod);
        limpar_buffer();
        while (fread(&c, sizeof(Cliente), 1, f) == 1)
            if (c.codigo == cod) { exibir_ficha_cliente(&c); encontrou = 1; }
    }
    else if (op == 2)
    {
        char busca[CLI_MAX_NOME];
        ler_string("  Nome (parte): ", busca, CLI_MAX_NOME);
        char b_lower[CLI_MAX_NOME];
        for (int i = 0; busca[i]; i++) b_lower[i] = tolower((unsigned char)busca[i]);
        b_lower[strlen(busca)] = '\0';
        while (fread(&c, sizeof(Cliente), 1, f) == 1)
        {
            char n_lower[CLI_MAX_NOME];
            for (int i = 0; c.nome[i]; i++) n_lower[i] = tolower((unsigned char)c.nome[i]);
            n_lower[strlen(c.nome)] = '\0';
            if (strstr(n_lower, b_lower)) { exibir_ficha_cliente(&c); encontrou = 1; }
        }
    }
    else if (op == 3)
    {
        char busca[CLI_MAX_CPF];
        ler_string("  CPF/CNPJ: ", busca, CLI_MAX_CPF);
        while (fread(&c, sizeof(Cliente), 1, f) == 1)
            if (strstr(c.cpf, busca)) { exibir_ficha_cliente(&c); encontrou = 1; }
    }

    fclose(f);
    if (!encontrou) printf("\n  Nenhum cliente encontrado.\n");
    pausar();
}

/* ----- Editar ----- */

void editar_cliente(void)
{
    limpar_tela();
    printf("\n  ====== EDITAR CLIENTE ======\n\n");

    printf("  Codigo do cliente: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Cliente c;
    if (!buscar_cliente_por_codigo(cod, &c))
    {
        printf("  Cliente nao encontrado!\n");
        pausar();
        return;
    }

    exibir_ficha_cliente(&c);
    printf("\n  O que deseja alterar?\n");
    printf("    [1] Nome\n");
    printf("    [2] CPF/CNPJ\n");
    printf("    [3] Contato (email/telefone/celular)\n");
    printf("    [4] Endereco completo\n");
    printf("    [5] Limite de credito\n");
    printf("    [6] Observacoes\n");
    printf("    [7] Status (ativar/inativar)\n");
    printf("    [0] Cancelar\n");
    printf("  Opcao: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    switch (op)
    {
        case 1:
            ler_string("  Novo nome: ", c.nome, CLI_MAX_NOME);
            break;
        case 2:
            ler_string("  Novo CPF/CNPJ: ", c.cpf, CLI_MAX_CPF);
            break;
        case 3:
            ler_string("  Novo e-mail  : ", c.email, CLI_MAX_EMAIL);
            ler_string("  Novo telefone: ", c.telefone, CLI_MAX_TELEFONE);
            ler_string("  Novo celular : ", c.celular, CLI_MAX_TELEFONE);
            break;
        case 4:
            ler_string("  Logradouro  : ", c.endereco, CLI_MAX_ENDERECO);
            ler_string("  Numero      : ", c.numero, 10);
            ler_string("  Complemento : ", c.complemento, 40);
            ler_string("  Bairro      : ", c.bairro, 50);
            ler_string("  Cidade      : ", c.cidade, CLI_MAX_CIDADE);
            ler_string("  Estado (UF) : ", c.estado, CLI_MAX_ESTADO);
            ler_string("  CEP         : ", c.cep, CLI_MAX_CEP);
            break;
        case 5:
            printf("  Novo limite (R$): ");
            scanf("%f", &c.limite_credito);
            limpar_buffer();
            break;
        case 6:
            ler_string("  Observacoes: ", c.observacoes, CLI_MAX_OBS);
            break;
        case 7:
            c.ativo = !c.ativo;
            printf("  Status alterado para: %s\n", c.ativo ? "ATIVO" : "INATIVO");
            break;
        default:
            printf("  Edicao cancelada.\n");
            pausar();
            return;
    }

    salvar_cliente(&c);
    printf("\n  " GREEN "Dados atualizados com sucesso!" RESET "\n");
    pausar();
}

/* ----- Excluir ----- */

void excluir_cliente(void)
{
    limpar_tela();
    printf("\n  ====== EXCLUIR / INATIVAR CLIENTE ======\n\n");

    printf("  Codigo do cliente: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Cliente c;
    if (!buscar_cliente_por_codigo(cod, &c))
    {
        printf("  Cliente nao encontrado!\n");
        pausar();
        return;
    }

    exibir_ficha_cliente(&c);

    printf("\n  [1] Inativar cliente (recomendado)\n");
    printf("  [2] Excluir definitivamente\n");
    printf("  [0] Cancelar\n");
    printf("  Opcao: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    if (op == 1)
    {
        c.ativo = 0;
        salvar_cliente(&c);
        printf("\n  " YELLOW "Cliente inativado." RESET "\n");
    }
    else if (op == 2)
    {
        printf("  ATENCAO! Isso e irreversivel. Confirma? [S/N]: ");
        char conf;
        scanf(" %c", &conf);
        limpar_buffer();
        if (toupper(conf) == 'S')
        {
            FILE *in  = fopen(ARQUIVO_CLIENTES, "rb");
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
            remove(ARQUIVO_CLIENTES);
            rename("clientes_tmp.dat", ARQUIVO_CLIENTES);
            printf("\n  " RED "Cliente excluido definitivamente." RESET "\n");
        }
        else
        {
            printf("  Exclusao cancelada.\n");
        }
    }
    pausar();
}

/* ----- Relatorio resumido ----- */

void relatorio_resumo_cliente(void)
{
    limpar_tela();
    printf("\n  ====== RELATORIO RESUMIDO DE CLIENTES ======\n\n");

    FILE *f = fopen(ARQUIVO_CLIENTES, "rb");
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
    printf("  Pessoa Fisica (PF)  : %d\n", pf);
    printf("  Pessoa Juridica (PJ): %d\n", pj);
    printf("  Limite total (R$)   : %.2f\n", limite_total);
    if (total > 0)
        printf("  Limite medio (R$)   : %.2f\n", limite_total / total);

    pausar();
}

/* ----- Exportar TXT ----- */

void exportar_clientes_txt(void)
{
    limpar_tela();
    printf("\n  ====== EXPORTAR CLIENTES PARA .TXT ======\n\n");

    FILE *dat = fopen(ARQUIVO_CLIENTES, "rb");
    if (!dat)
    {
        printf("  Nenhum registro encontrado.\n");
        pausar();
        return;
    }

    FILE *txt = fopen(ARQUIVO_TXT_CLI, "w");
    if (!txt)
    {
        fclose(dat);
        printf("  ERRO ao criar arquivo!\n");
        pausar();
        return;
    }

    char agora[20];
    data_hoje(agora);
    fprintf(txt, "MERCADINHO EXPRESS - PORTO DIGITAL v%s\n", VERSAO);
    fprintf(txt, "Relatorio  : LISTA COMPLETA DE CLIENTES\n");
    fprintf(txt, "Gerado em  : %s\n\n", agora);

    sep_txt(txt, '-', 70);
    fprintf(txt, "%-5s %-30s %-15s %-12s %-7s\n",
            "COD", "NOME", "CPF/CNPJ", "CIDADE", "STATUS");
    sep_txt(txt, '-', 70);

    Cliente c;
    int count = 0;
    while (fread(&c, sizeof(Cliente), 1, dat) == 1)
    {
        char nome_t[31]; strncpy(nome_t, c.nome,   30); nome_t[30] = '\0';
        char cpf_t[16];  strncpy(cpf_t,  c.cpf,    15); cpf_t[15]  = '\0';
        char cid_t[13];  strncpy(cid_t,  c.cidade, 12); cid_t[12]  = '\0';

        fprintf(txt, "%-5d %-30s %-15s %-12s %-7s\n",
                c.codigo, nome_t, cpf_t, cid_t, c.ativo ? "ATIVO" : "INATIVO");
        count++;
    }
    sep_txt(txt, '-', 70);
    fprintf(txt, "Total: %d cliente(s).\n", count);

    fclose(dat);
    fclose(txt);
    printf("\n  " GREEN "Arquivo gerado: %s" RESET "\n", ARQUIVO_TXT_CLI);
    pausar();
}

/* ----- Menu de clientes ----- */

void menu_clientes(void)
{
    int op;
    do {
        limpar_tela();
        printf("\n  ============================================\n");
        printf("     PAINEL DO CLIENTE & FIDELIDADE\n");
        printf("  ============================================\n");
        printf("  [1] Cadastrar novo cliente\n");
        printf("  [2] Consultar cliente\n");
        printf("  [3] Editar cadastro\n");
        printf("  [4] Excluir / inativar cliente\n");
        printf("  [5] Listar todos os clientes\n");
        printf("  [6] Listar apenas ativos\n");
        printf("  [7] Relatorio resumido\n");
        printf("  [8] Exportar para .TXT\n");
        printf("  [0] Voltar ao menu principal\n");
        printf("  ============================================\n");
        printf("  Total de clientes cadastrados: %d\n", total_registros_cliente());
        printf("  Opcao: ");
        scanf("%d", &op);
        limpar_buffer();

        switch (op)
        {
            case 1: cadastrar_cliente();        break;
            case 2: consultar_cliente();        break;
            case 3: editar_cliente();           break;
            case 4: excluir_cliente();          break;
            case 5: listar_clientes(0);         break;
            case 6: listar_clientes(1);         break;
            case 7: relatorio_resumo_cliente(); break;
            case 8: exportar_clientes_txt();    break;
            case 0: break;
            default:
                printf("\n  Opcao invalida!\n");
                pausar();
        }
    } while (op != 0);
}

/* ===================================================================
 * =====================   MODULO PDV (VENDAS)   ======================
 * =================================================================== */

int proximo_numero_venda(void)
{
    FILE *f = fopen(ARQUIVO_VENDAS, "rb");
    if (!f) return 1;
    int max = 0;
    Venda v;
    while (fread(&v, sizeof(Venda), 1, f) == 1)
        if (v.numero_venda > max) max = v.numero_venda;
    fclose(f);
    return max + 1;
}

void registrar_venda(int cod_cliente, float total)
{
    Venda v;
    v.numero_venda   = proximo_numero_venda();
    v.codigo_cliente = cod_cliente;
    v.total          = total;
    data_hora_agora(v.data_hora);

    FILE *f = fopen(ARQUIVO_VENDAS, "ab");
    if (!f) return;
    fwrite(&v, sizeof(Venda), 1, f);
    fclose(f);
}

void ponto_de_venda(void)
{
    limpar_tela();
    printf("\n  ============================================\n");
    printf("     PONTO DE VENDA (PDV) - NOVA PRE-VENDA\n");
    printf("  ============================================\n\n");

    if (total_registros_produto() == 0)
    {
        printf("  Nenhum produto cadastrado no estoque!\n");
        printf("  Cadastre produtos antes de iniciar uma venda.\n");
        pausar();
        return;
    }

    /* Identificar cliente (opcional) */
    int cod_cliente = 0;
    printf("  Informar cliente nesta venda? [S/N]: ");
    char resp;
    scanf(" %c", &resp);
    limpar_buffer();
    if (toupper(resp) == 'S')
    {
        Cliente c;
        printf("  Codigo do cliente: ");
        scanf("%d", &cod_cliente);
        limpar_buffer();
        if (buscar_cliente_por_codigo(cod_cliente, &c))
            printf("  Cliente: %s\n", c.nome);
        else
        {
            printf("  Cliente nao encontrado. Venda sera registrada sem cliente.\n");
            cod_cliente = 0;
        }
    }

    /* Estruturas auxiliares para os itens da venda */
    int   item_codigo[MAX_ITENS_VENDA];
    int   item_qtd[MAX_ITENS_VENDA];
    float item_subtotal[MAX_ITENS_VENDA];
    char  item_nome[MAX_ITENS_VENDA][MAX_NOME];
    int   total_itens = 0;
    float total_venda = 0.0f;

    int continuar = 1;
    while (continuar)
    {
        if (total_itens >= MAX_ITENS_VENDA)
        {
            printf("\n  Limite de itens por venda atingido!\n");
            break;
        }

        printf("\n  --- Adicionar item (%d/%d) ---\n", total_itens + 1, MAX_ITENS_VENDA);
        printf("  Codigo do produto (0 para finalizar venda): ");
        int cod;
        scanf("%d", &cod);
        limpar_buffer();

        if (cod == 0) break;

        Produto p;
        if (!buscar_produto_por_codigo(cod, &p))
        {
            printf("  " RED "Produto nao encontrado!" RESET "\n");
            continue;
        }
        if (!p.ativo)
        {
            printf("  " YELLOW "Produto inativo, nao pode ser vendido." RESET "\n");
            continue;
        }

        printf("  Produto: %s | Preco: R$ %.2f | Estoque: %d %s\n",
               p.nome, p.preco_venda, p.quantidade, p.unidade);

        int qtd;
        printf("  Quantidade: ");
        scanf("%d", &qtd);
        limpar_buffer();

        if (qtd <= 0)
        {
            printf("  " RED "Quantidade invalida!" RESET "\n");
            continue;
        }
        if (qtd > p.quantidade)
        {
            printf("  " RED "Estoque insuficiente! Disponivel: %d" RESET "\n", p.quantidade);
            continue;
        }

        float subtotal = qtd * p.preco_venda;

        item_codigo[total_itens]   = p.codigo;
        item_qtd[total_itens]      = qtd;
        item_subtotal[total_itens] = subtotal;
        strncpy(item_nome[total_itens], p.nome, MAX_NOME - 1);
        item_nome[total_itens][MAX_NOME - 1] = '\0';
        total_itens++;
        total_venda += subtotal;

        printf("  " GREEN "Item adicionado! Subtotal: R$ %.2f" RESET "\n", subtotal);

        printf("\n  Adicionar outro item? [S/N]: ");
        scanf(" %c", &resp);
        limpar_buffer();
        if (toupper(resp) != 'S') continuar = 0;
    }

    if (total_itens == 0)
    {
        printf("\n  Nenhum item adicionado. Venda cancelada.\n");
        pausar();
        return;
    }

    /* ----- Resumo da venda ----- */
    limpar_tela();
    printf("\n  ============================================\n");
    printf("              RESUMO DA PRE-VENDA\n");
    printf("  ============================================\n");
    printf("  %-5s %-25s %-5s %-10s\n", "COD", "PRODUTO", "QTD", "SUBTOTAL");
    printf("  --------------------------------------------\n");
    for (int i = 0; i < total_itens; i++)
        printf("  %-5d %-25s %-5d %-10.2f\n",
               item_codigo[i], item_nome[i], item_qtd[i], item_subtotal[i]);
    printf("  --------------------------------------------\n");
    printf("  TOTAL DA VENDA: R$ %.2f\n", total_venda);
    if (cod_cliente != 0)
        printf("  Cliente: codigo %d\n", cod_cliente);

    printf("\n  Confirmar venda e baixar do estoque? [S/N]: ");
    scanf(" %c", &resp);
    limpar_buffer();

    if (toupper(resp) == 'S')
    {
        for (int i = 0; i < total_itens; i++)
        {
            Produto p;
            if (buscar_produto_por_codigo(item_codigo[i], &p))
            {
                p.quantidade -= item_qtd[i];
                salvar_produto(&p);
                registrar_movimentacao(p.codigo, "SAIDA", item_qtd[i]);
            }
        }
        registrar_venda(cod_cliente, total_venda);
        printf("\n  " GREEN "Venda finalizada com sucesso!" RESET "\n");
        printf("  Total: R$ %.2f\n", total_venda);
    }
    else
    {
        printf("\n  Venda cancelada. Estoque nao foi alterado.\n");
    }
    pausar();
}

/* ===================================================================
 * =====================   DASHBOARD DE VENDAS   ======================
 * =================================================================== */

void dashboard_vendas(void)
{
    limpar_tela();
    printf("\n  ============================================\n");
    printf("       DASHBOARD DE VENDAS E RELATORIOS\n");
    printf("  ============================================\n\n");

    /* ----- Vendas ----- */
    FILE *fv = fopen(ARQUIVO_VENDAS, "rb");
    int total_vendas = 0;
    float faturamento = 0.0f;
    if (fv)
    {
        Venda v;
        while (fread(&v, sizeof(Venda), 1, fv) == 1)
        {
            total_vendas++;
            faturamento += v.total;
        }
        fclose(fv);
    }

    printf("  --- VENDAS ---\n");
    printf("  Total de vendas realizadas : %d\n", total_vendas);
    printf("  Faturamento total (R$)     : %.2f\n", faturamento);
    if (total_vendas > 0)
        printf("  Ticket medio (R$)          : %.2f\n", faturamento / total_vendas);

    /* ----- Estoque / produtos ----- */
    printf("\n  --- ESTOQUE ---\n");
    FILE *fp = fopen(ARQUIVO_PRODUTOS, "rb");
    int total_prod = 0, ativos = 0, vencidos = 0, criticos = 0, atencao = 0;
    float valor_estoque_custo = 0.0f, valor_estoque_venda = 0.0f;

    if (fp)
    {
        Produto p;
        while (fread(&p, sizeof(Produto), 1, fp) == 1)
        {
            classificar(&p);
            total_prod++;
            if (p.ativo) ativos++;
            valor_estoque_custo += p.preco_custo * p.quantidade;
            valor_estoque_venda += p.preco_venda * p.quantidade;
            switch (p.prioridade)
            {
                case 0: vencidos++; break;
                case 1: criticos++; break;
                case 2: atencao++;  break;
                default: break;
            }
        }
        fclose(fp);
    }

    printf("  Produtos cadastrados       : %d (ativos: %d)\n", total_prod, ativos);
    printf("  Valor em estoque (custo)   : R$ %.2f\n", valor_estoque_custo);
    printf("  Valor em estoque (venda)   : R$ %.2f\n", valor_estoque_venda);
    printf("  Lucro potencial            : R$ %.2f\n", valor_estoque_venda - valor_estoque_custo);
    printf("\n  --- ALERTAS DE VALIDADE ---\n");
    printf("  Produtos VENCIDOS          : " RED "%d" RESET "\n", vencidos);
    printf("  Produtos em situacao CRITICA: " YELLOW "%d" RESET "\n", criticos);
    printf("  Produtos em ATENCAO        : " YELLOW "%d" RESET "\n", atencao);

    /* ----- Clientes ----- */
    printf("\n  --- CLIENTES ---\n");
    printf("  Total de clientes cadastrados: %d\n", total_registros_cliente());

    pausar();
}

/* ===================================================================
 * ===================   CONFIGURACOES E BACKUP   ======================
 * =================================================================== */

void copiar_arquivo(const char *origem, const char *destino)
{
    FILE *in = fopen(origem, "rb");
    if (!in) return; /* se nao existir, apenas ignora */
    FILE *out = fopen(destino, "wb");
    if (!out) { fclose(in); return; }

    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), in)) > 0)
        fwrite(buffer, 1, n, out);

    fclose(in);
    fclose(out);
}

void fazer_backup(void)
{
    limpar_tela();
    printf("\n  ====== BACKUP DOS DADOS ======\n\n");

    char sufixo[20];
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(sufixo, sizeof(sufixo), "%Y%m%d_%H%M", tm_info);

    char destino[100];

    sprintf(destino, "backup_produtos_%s.dat", sufixo);
    copiar_arquivo(ARQUIVO_PRODUTOS, destino);
    printf("  Produtos -> %s\n", destino);

    sprintf(destino, "backup_clientes_%s.dat", sufixo);
    copiar_arquivo(ARQUIVO_CLIENTES, destino);
    printf("  Clientes -> %s\n", destino);

    sprintf(destino, "backup_vendas_%s.dat", sufixo);
    copiar_arquivo(ARQUIVO_VENDAS, destino);
    printf("  Vendas   -> %s\n", destino);

    sprintf(destino, "backup_historico_%s.dat", sufixo);
    copiar_arquivo(ARQUIVO_HIST, destino);
    printf("  Historico-> %s\n", destino);

    printf("\n  " GREEN "Backup concluido!" RESET "\n");
    pausar();
}

void sobre_sistema(void)
{
    limpar_tela();
    printf("\n  ============================================\n");
    printf("       MERCADINHO EXPRESS - PORTO DIGITAL\n");
    printf("  ============================================\n\n");
    printf("  Versao do sistema : %s\n", VERSAO);
    printf("  Arquivos de dados :\n");
    printf("    - %s (produtos)\n", ARQUIVO_PRODUTOS);
    printf("    - %s (clientes)\n", ARQUIVO_CLIENTES);
    printf("    - %s (vendas)\n", ARQUIVO_VENDAS);
    printf("    - %s (historico de movimentacoes)\n", ARQUIVO_HIST);
    printf("\n  Produtos cadastrados : %d\n", total_registros_produto());
    printf("  Clientes cadastrados : %d\n", total_registros_cliente());
    pausar();
}

void menu_configuracoes(void)
{
    int op;
    do {
        limpar_tela();
        printf("\n  ============================================\n");
        printf("         CONFIGURACOES E BACKUP\n");
        printf("  ============================================\n");
        printf("  [1] Fazer backup dos dados\n");
        printf("  [2] Sobre o sistema\n");
        printf("  [0] Voltar ao menu principal\n");
        printf("  ============================================\n");
        printf("  Opcao: ");
        scanf("%d", &op);
        limpar_buffer();

        switch (op)
        {
            case 1: fazer_backup();    break;
            case 2: sobre_sistema();   break;
            case 0: break;
            default:
                printf("\n  Opcao invalida!\n");
                pausar();
        }
    } while (op != 0);
}

/* ===================================================================
 * ========================   MENU PRINCIPAL   =========================
 * =================================================================== */

void exibir_menu_principal(void)
{
    printf("\n==================================================\n");
    printf("       MERCADINHO EXPRESS - PORTO DIGITAL\n");
    printf("==================================================\n");
    printf("[1] PONTO DE VENDA (PDV) - Nova Pre-Venda\n");
    printf("[2] CONTROLE DE ESTOQUE (Rapido)\n");
    printf("[3] PAINEL DO CLIENTE & FIDELIDADE\n");
    printf("[4] DASHBOARD DE VENDAS E RELATORIOS\n");
    printf("[5] CONFIGURACOES E BACKUP\n");
    printf("[0] SAIR DO SISTEMA\n");
    printf("==================================================\n");
    printf("Escolha uma opcao: ");
}

int main(void)
{
    int escolha;
    setlocale(LC_ALL, "Portuguese");

    
        limpar_tela();
        exibir_menu_principal();
        scanf("%d", &escolha);
        limpar_buffer();

        switch (escolha)
        {
            case 1: ponto_de_venda();     break;
            case 2: menu_estoque();       break;
            case 3: menu_clientes();      break;
            case 4: dashboard_vendas();   break;
            case 5: menu_configuracoes(); break;
            case 0:
                limpar_tela();
                printf("\nSaindo do sistema... Ate logo!\n");
                break;
            default:
                printf("\nOpcao invalida, tente novamente!\n");
                pausar();
        }
    } while (escolha != 0);

    return 0;

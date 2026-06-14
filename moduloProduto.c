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

/* ==================== CONSTANTES ========================== */
#define MAX_NOME       60
#define MAX_CATEGORIA  35
#define MAX_UNIDADE    10
#define MAX_OBS        200
#define MAX_HIST       500
#define ARQUIVO_DADOS  "produtos.dat"
#define ARQUIVO_HIST   "historico.dat"
#define ARQUIVO_TXT    "produtos_export.txt"
#define VERSAO         "1.0.0"
#define LIMITE_CRITICO  3
#define LIMITE_ATENCAO  7

/* ==================== ESTRUTURAS ========================== */

typedef struct
{
    int   codigo;
    char  nome[MAX_NOME];
    char  categoria[MAX_CATEGORIA];
    char  unidade[MAX_UNIDADE];    /* KG, UN, L, CX … */
    float peso_volume;             /* peso ou volume por unidade */
    int   quantidade;
    float preco_custo;             /* preço de compra */
    float preco_venda;             /* preço ao cliente */
    int   dia_val, mes_val, ano_val;
    int   dias_restantes;
    int   prioridade;              /* 0=Vencido 1=Crítico 2=Atenção 3=OK */
    int   ativo;                   /* 1=ativo  0=inativo */
    char  data_cadastro[20];
    char  observacoes[MAX_OBS];
} Produto;

typedef struct
{
    int  codigo_produto;
    char tipo_acao[15];   /* CADASTRO | ENTRADA | SAIDA | EXCLUSAO | EDICAO */
    int  quantidade;
    char data_hora[25];   /* DD/MM/AAAA HH:MM:SS */
} Movimentacao;

/* ==================== UTILITÁRIOS ========================= */

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

/* ==================== VALIDADE / PRIORIDADE =============== */

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

/* ==================== ARQUIVO / PERSISTÊNCIA ============== */

int proximo_codigo(void)
{
    FILE *f = fopen(ARQUIVO_DADOS, "rb");
    if (!f) return 1;
    int max = 0;
    Produto p;
    while (fread(&p, sizeof(Produto), 1, f) == 1)
        if (p.codigo > max) max = p.codigo;
    fclose(f);
    return max + 1;
}

int total_registros(void)
{
    FILE *f = fopen(ARQUIVO_DADOS, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fclose(f);
    return (int)(tam / sizeof(Produto));
}

/* Busca produto por código; retorna 1 se achou, 0 caso contrário */
int buscar_por_codigo(int codigo, Produto *dest)
{
    FILE *f = fopen(ARQUIVO_DADOS, "rb");
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

/* ==================== HISTÓRICO =========================== */

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

/* ==================== EXIBIR FICHA ======================== */

void exibir_ficha(const Produto *p)
{
    classificar((Produto *)p); /* atualiza dias sem alterar arquivo */
    const char *cor = cor_prioridade(p->prioridade);
    const char *lbl = label_prioridade(p->prioridade);

    printf("\n  ╔══════════════════════════════════════════════════════╗\n");
    printf("  ║  FICHA DO PRODUTO  ─  Cód: %-5d  Status: %-8s  ║\n",
           p->codigo, p->ativo ? "ATIVO" : "INATIVO");
    printf("  ╠══════════════════════════════════════════════════════╣\n");
    printf("  ║  Nome       : %-38s  ║\n", p->nome);
    printf("  ║  Categoria  : %-38s  ║\n", p->categoria);
    printf("  ║  Unidade    : %-10s   Peso/Vol : %-18.3f  ║\n",
           p->unidade, p->peso_volume);
    printf("  ╠══════════════════════════════════════════════════════╣\n");
    printf("  ║  Quantidade em estoque : %-29d  ║\n", p->quantidade);
    printf("  ║  Preço de custo  (R$)  : %-29.2f  ║\n", p->preco_custo);
    printf("  ║  Preço de venda  (R$)  : %-29.2f  ║\n", p->preco_venda);
    printf("  ╠══════════════════════════════════════════════════════╣\n");
    printf("  ║  Validade   : %02d/%02d/%04d   Situação: %s%-7s" RESET "          ║\n",
           p->dia_val, p->mes_val, p->ano_val, cor, lbl);
    printf("  ║  Dias rest. : %-38d  ║\n", p->dias_restantes);
    printf("  ╠══════════════════════════════════════════════════════╣\n");
    printf("  ║  Cadastrado : %-38s  ║\n", p->data_cadastro);
    if (strlen(p->observacoes) > 0)
    printf("  ║  Obs.       : %-38s  ║\n", p->observacoes);
    printf("  ╚══════════════════════════════════════════════════════╝\n");
}

/* ==================== CADASTRAR PRODUTO ================== */

void cadastrar_produto(void)
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║        CADASTRO DE NOVO PRODUTO          ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    Produto p;
    memset(&p, 0, sizeof(Produto));

    p.codigo = proximo_codigo();
    p.ativo  = 1;
    data_hoje(p.data_cadastro);

    printf("  Código gerado automaticamente: %d\n\n", p.codigo);

    /* ----- Identificação ----- */
    printf("  --- IDENTIFICAÇÃO ---\n");
    ler_string("  Nome do produto  : ", p.nome, MAX_NOME);
    if (strlen(p.nome) == 0)
    {
        printf("  Nome é obrigatório!\n");
        pausar();
        return;
    }
    ler_string("  Categoria        : ", p.categoria, MAX_CATEGORIA);
    ler_string("  Unidade (UN/KG/L): ", p.unidade, MAX_UNIDADE);
    printf("  Peso / Volume (por unidade): ");
    scanf("%f", &p.peso_volume);
    limpar_buffer();

    /* ----- Estoque ----- */
    printf("\n  --- ESTOQUE ---\n");
    printf("  Quantidade inicial: ");
    scanf("%d", &p.quantidade);
    limpar_buffer();

    /* ----- Preços ----- */
    printf("\n  --- PREÇOS ---\n");
    printf("  Preço de custo  (R$): ");
    scanf("%f", &p.preco_custo);
    limpar_buffer();
    printf("  Preço de venda  (R$): ");
    scanf("%f", &p.preco_venda);
    limpar_buffer();

    /* ----- Validade ----- */
    printf("\n  --- VALIDADE ---\n");
    printf("  Data de validade (DD MM AAAA): ");
    scanf("%d %d %d", &p.dia_val, &p.mes_val, &p.ano_val);
    limpar_buffer();
    classificar(&p);

    /* ----- Observações ----- */
    printf("\n  --- OBSERVAÇÕES ---\n");
    ler_string("  Observações (Enter p/ pular): ", p.observacoes, MAX_OBS);

    /* ----- Confirmar ----- */
    exibir_ficha(&p);
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
        fwrite(&p, sizeof(Produto), 1, f);
        fclose(f);

        registrar_movimentacao(p.codigo, "CADASTRO", p.quantidade);
        printf("\n  " GREEN "✔ Produto cadastrado com sucesso! Código: %d" RESET "\n", p.codigo);
    }
    else
    {
        printf("\n  Cadastro cancelado.\n");
    }
    pausar();
}

/* ==================== LISTAR PRODUTOS ==================== */

void listar_produtos(int apenas_ativos)
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("  ║  LISTA DE PRODUTOS%-53s║\n",
           apenas_ativos ? " (ATIVOS)                                     " :
                           "                                              ");
    printf("  ╠══════╦═══════════════════╦════════════════╦══════╦═══════════╦══════════╣\n");
    printf("  ║ CÓD. ║ NOME              ║ CATEGORIA      ║ QTD  ║ VDA(R$)   ║ SITUAÇÃO ║\n");
    printf("  ╠══════╬═══════════════════╬════════════════╬══════╬═══════════╬══════════╣\n");

    FILE *f = fopen(ARQUIVO_DADOS, "rb");
    if (!f)
    {
        printf("  ║  Nenhum produto cadastrado.                                            ║\n");
        printf("  ╚══════════════════════════════════════════════════════════════════════════╝\n");
        pausar();
        return;
    }

    Produto p;
    int count = 0;
    while (fread(&p, sizeof(Produto), 1, f) == 1)
    {
        if (apenas_ativos && !p.ativo) continue;
        classificar(&p);

        char nome_t[20];   strncpy(nome_t, p.nome,      19); nome_t[19]  = '\0';
        char cat_t[15];    strncpy(cat_t,  p.categoria,  14); cat_t[14]   = '\0';
        const char *cor = cor_prioridade(p.prioridade);
        const char *lbl = label_prioridade(p.prioridade);

        printf("  ║ %-4d ║ %-17s ║ %-14s ║ %-4d ║ %-9.2f ║ %s%-8s" RESET " ║\n",
               p.codigo, nome_t, cat_t, p.quantidade, p.preco_venda, cor, lbl);
        count++;
    }
    fclose(f);

    printf("  ╚══════╩═══════════════════╩════════════════╩══════╩═══════════╩══════════╝\n");
    printf("  Total: %d produto(s).\n", count);
    pausar();
}

/* ==================== CONSULTAR PRODUTO ================== */

void consultar_produto(void)
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║         CONSULTA DE PRODUTO              ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  Buscar por:\n");
    printf("    [1] Código\n");
    printf("    [2] Nome\n");
    printf("    [3] Categoria\n");
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
    Produto p;

    if (op == 1)
    {
        printf("  Código: ");
        int cod;
        scanf("%d", &cod);
        limpar_buffer();
        while (fread(&p, sizeof(Produto), 1, f) == 1)
            if (p.codigo == cod) { classificar(&p); exibir_ficha(&p); encontrou = 1; }
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
            if (strstr(n_low, b_low)) { classificar(&p); exibir_ficha(&p); encontrou = 1; }
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
            if (strstr(c_low, b_low)) { classificar(&p); exibir_ficha(&p); encontrou = 1; }
        }
    }

    fclose(f);
    if (!encontrou) printf("\n  Nenhum produto encontrado.\n");
    pausar();
}

/* ==================== EDITAR PRODUTO ===================== */

void editar_produto(void)
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║           EDITAR PRODUTO                 ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  Código do produto: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Produto p;
    if (!buscar_por_codigo(cod, &p))
    {
        printf("  Produto não encontrado!\n");
        pausar();
        return;
    }

    classificar(&p);
    exibir_ficha(&p);

    printf("\n  O que deseja alterar?\n");
    printf("    [1] Nome\n");
    printf("    [2] Categoria\n");
    printf("    [3] Unidade / Peso-Volume\n");
    printf("    [4] Preços (custo e venda)\n");
    printf("    [5] Data de validade\n");
    printf("    [6] Observações\n");
    printf("    [7] Status (ativar / inativar)\n");
    printf("    [0] Cancelar\n");
    printf("  Opção: ");
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
            printf("  Novo preço de custo  (R$): ");
            scanf("%f", &p.preco_custo);
            limpar_buffer();
            printf("  Novo preço de venda  (R$): ");
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
            ler_string("  Observações: ", p.observacoes, MAX_OBS);
            break;
        case 7:
            p.ativo = !p.ativo;
            printf("  Status alterado para: %s\n", p.ativo ? "ATIVO" : "INATIVO");
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
    Produto tmp;
    while (fread(&tmp, sizeof(Produto), 1, f) == 1)
    {
        if (tmp.codigo == cod)
        {
            fseek(f, -(long)sizeof(Produto), SEEK_CUR);
            fwrite(&p, sizeof(Produto), 1, f);
            break;
        }
    }
    fclose(f);

    registrar_movimentacao(p.codigo, "EDICAO", 0);
    printf("\n  " GREEN "✔ Produto atualizado com sucesso!" RESET "\n");
    pausar();
}

/* ==================== EXCLUIR PRODUTO ==================== */

void excluir_produto(void)
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║        EXCLUIR / INATIVAR PRODUTO        ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  Código do produto: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Produto p;
    if (!buscar_por_codigo(cod, &p))
    {
        printf("  Produto não encontrado!\n");
        pausar();
        return;
    }

    classificar(&p);
    exibir_ficha(&p);

    printf("\n  [1] Inativar produto (recomendado)\n");
    printf("  [2] Excluir definitivamente\n");
    printf("  [0] Cancelar\n");
    printf("  Opção: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    if (op == 1)
    {
        p.ativo = 0;
        FILE *f = fopen(ARQUIVO_DADOS, "r+b");
        if (!f) { printf("  ERRO ao abrir arquivo!\n"); pausar(); return; }
        Produto tmp;
        while (fread(&tmp, sizeof(Produto), 1, f) == 1)
        {
            if (tmp.codigo == cod)
            {
                fseek(f, -(long)sizeof(Produto), SEEK_CUR);
                fwrite(&p, sizeof(Produto), 1, f);
                break;
            }
        }
        fclose(f);
        registrar_movimentacao(cod, "INATIVADO", p.quantidade);
        printf("\n  " YELLOW "✔ Produto inativado." RESET "\n");
    }
    else if (op == 2)
    {
        printf("  ATENÇÃO! Isso é irreversível. Confirma? [S/N]: ");
        char conf;
        scanf(" %c", &conf);
        limpar_buffer();

        if (toupper(conf) == 'S')
        {
            FILE *in  = fopen(ARQUIVO_DADOS, "rb");
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
            remove(ARQUIVO_DADOS);
            rename("produtos_tmp.dat", ARQUIVO_DADOS);
            registrar_movimentacao(cod, "EXCLUSAO", p.quantidade);
            printf("\n  " RED "✔ Produto excluído definitivamente." RESET "\n");
        }
        else
        {
            printf("  Exclusão cancelada.\n");
        }
    }
    pausar();
}

/* ==================== MOVIMENTAR ESTOQUE ================= */

void movimentar_estoque(void)
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║      MOVIMENTAR ESTOQUE (ENTRADA/SAÍDA)  ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  Código do produto: ");
    int cod;
    scanf("%d", &cod);
    limpar_buffer();

    Produto p;
    if (!buscar_por_codigo(cod, &p))
    {
        printf("  Produto não encontrado!\n");
        pausar();
        return;
    }

    classificar(&p);
    printf("\n  Produto : %s\n", p.nome);
    printf("  Estoque atual: %d %s\n\n", p.quantidade, p.unidade);
    printf("  [1] Registrar ENTRADA (+)\n");
    printf("  [2] Registrar SAÍDA   (-)\n");
    printf("  [0] Cancelar\n");
    printf("  Opção: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    if (op == 0) return;

    int qtd;
    printf("  Quantidade: ");
    scanf("%d", &qtd);
    limpar_buffer();

    if (qtd <= 0)
    {
        printf("  Quantidade inválida!\n");
        pausar();
        return;
    }

    if (op == 1)
    {
        p.quantidade += qtd;
        registrar_movimentacao(cod, "ENTRADA", qtd);
        printf("\n  " GREEN "✔ Entrada registrada! Novo saldo: %d %s" RESET "\n",
               p.quantidade, p.unidade);
    }
    else if (op == 2)
    {
        if (qtd > p.quantidade)
        {
            printf("\n  " RED "[ERRO] Quantidade insuficiente em estoque!" RESET "\n");
            pausar();
            return;
        }
        p.quantidade -= qtd;
        registrar_movimentacao(cod, "SAIDA", qtd);
        printf("\n  " GREEN "✔ Saída registrada! Novo saldo: %d %s" RESET "\n",
               p.quantidade, p.unidade);
    }
    else
    {
        printf("  Opção inválida.\n");
        pausar();
        return;
    }

    /* Persiste a quantidade atualizada */
    FILE *f = fopen(ARQUIVO_DADOS, "r+b");
    if (!f) { printf("  ERRO ao salvar!\n"); pausar(); return; }
    Produto tmp;
    while (fread(&tmp, sizeof(Produto), 1, f) == 1)
    {
        if (tmp.codigo == cod)
        {
            fseek(f, -(long)sizeof(Produto), SEEK_CUR);
            fwrite(&p, sizeof(Produto), 1, f);
            break;
        }
    }
    fclose(f);
    pausar();
}

/* ==================== RELATÓRIO RESUMIDO ================= */

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

    int total = 0, ativos = 0, inativos = 0;
    int vencidos = 0, criticos = 0, atencao = 0, ok = 0;
    float valor_custo = 0.0f, valor_venda = 0.0f;
    Produto p;

    while (fread(&p, sizeof(Produto), 1, f) == 1)
    {
        classificar(&p);
        total++;
        if (p.ativo)   ativos++;   else inativos++;
        if (p.prioridade == 0) vencidos++;
        else if (p.prioridade == 1) criticos++;
        else if (p.prioridade == 2) atencao++;
        else ok++;
        valor_custo += p.preco_custo  * p.quantidade;
        valor_venda += p.preco_venda  * p.quantidade;
    }
    fclose(f);

    printf("  Total de produtos      : %d\n",    total);
    printf("  Ativos                 : %d\n",    ativos);
    printf("  Inativos               : %d\n",    inativos);
    printf("\n");
    printf("  " GREEN "  [OK]      " RESET "              : %d\n",  ok);
    printf("  " YELLOW "  [ATENÇÃO]  " RESET "              : %d\n",  atencao);
    printf("  " YELLOW "  [CRÍTICO]  " RESET "              : %d\n",  criticos);
    printf("  " RED    "  [VENCIDO]  " RESET "              : %d\n",  vencidos);
    printf("\n");
    printf("  Valor em estoque (custo): R$ %.2f\n",  valor_custo);
    printf("  Valor em estoque (venda): R$ %.2f\n",  valor_venda);
    if (total > 0)
        printf("  Margem bruta estimada   : R$ %.2f\n",
               valor_venda - valor_custo);

    pausar();
}

/* ==================== HISTÓRICO DE MOVIMENTAÇÕES ========= */

void consultar_historico(void)
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("  ║              HISTÓRICO DE MOVIMENTAÇÕES (AUDITORIA)                    ║\n");
    printf("  ╠═════════════════════════╦════════════╦══════════════╦══════════════════╣\n");
    printf("  ║ Data / Hora             ║ Cód.Prod.  ║ Ação         ║ Quantidade       ║\n");
    printf("  ╠═════════════════════════╬════════════╬══════════════╬══════════════════╣\n");

    FILE *f = fopen(ARQUIVO_HIST, "rb");
    if (!f)
    {
        printf("  ║  Nenhuma movimentação registrada ainda.                                ║\n");
        printf("  ╚══════════════════════════════════════════════════════════════════════════╝\n");
        pausar();
        return;
    }

    Movimentacao m;
    int count = 0;
    while (fread(&m, sizeof(Movimentacao), 1, f) == 1)
    {
        printf("  ║ %-23s ║ %-10d ║ %-12s ║ %-16d ║\n",
               m.data_hora, m.codigo_produto, m.tipo_acao, m.quantidade);
        count++;
    }
    fclose(f);

    printf("  ╚═════════════════════════╩════════════╩══════════════╩══════════════════╝\n");
    printf("  Total de movimentações: %d\n", count);
    pausar();
}

/* ==================== EXPORTAR TXT ======================= */

static void ficha_para_txt(FILE *f, const Produto *p)
{
    sep_txt(f, '=', 64);
    fprintf(f, "  FICHA DO PRODUTO\n");
    fprintf(f, "  Código     : %d\n",  p->codigo);
    fprintf(f, "  Status     : %s\n",  p->ativo ? "ATIVO" : "INATIVO");
    sep_txt(f, '-', 64);
    fprintf(f, "  IDENTIFICAÇÃO\n");
    fprintf(f, "  Nome       : %s\n",  p->nome);
    fprintf(f, "  Categoria  : %s\n",  p->categoria);
    fprintf(f, "  Unidade    : %s\n",  p->unidade);
    fprintf(f, "  Peso/Vol   : %.3f\n",p->peso_volume);
    sep_txt(f, '-', 64);
    fprintf(f, "  ESTOQUE E PREÇOS\n");
    fprintf(f, "  Quantidade : %d\n",  p->quantidade);
    fprintf(f, "  Custo (R$) : %.2f\n",p->preco_custo);
    fprintf(f, "  Venda (R$) : %.2f\n",p->preco_venda);
    sep_txt(f, '-', 64);
    fprintf(f, "  VALIDADE\n");
    fprintf(f, "  Data       : %02d/%02d/%04d\n", p->dia_val, p->mes_val, p->ano_val);
    fprintf(f, "  Situação   : %s\n",  label_prioridade(p->prioridade));
    fprintf(f, "  Dias rest. : %d\n",  p->dias_restantes);
    sep_txt(f, '-', 64);
    fprintf(f, "  Cadastro   : %s\n",  p->data_cadastro);
    if (strlen(p->observacoes) > 0)
        fprintf(f, "  Obs.       : %s\n", p->observacoes);
    sep_txt(f, '=', 64);
    fputc('\n', f);
}

void exportar_txt(void)
{
    limpar_tela();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║        EXPORTAR PARA ARQUIVO .TXT        ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  O que deseja exportar?\n");
    printf("    [1] Ficha de um produto específico  ->  ficha_NNN.txt\n");
    printf("    [2] Lista completa (tabela)          ->  %s\n", ARQUIVO_TXT);
    printf("    [3] Relatório gerencial completo     ->  %s\n", ARQUIVO_TXT);
    printf("    [0] Cancelar\n");
    printf("  Opção: ");
    int op;
    scanf("%d", &op);
    limpar_buffer();

    if (op == 0) return;

    FILE *dat = fopen(ARQUIVO_DADOS, "rb");
    if (!dat)
    {
        printf("\n  Nenhum registro encontrado. Cadastre produtos primeiro.\n");
        pausar();
        return;
    }

    char nome_saida[80];
    strcpy(nome_saida, ARQUIVO_TXT);

    /* ---- Opção 1: ficha individual ---- */
    if (op == 1)
    {
        printf("  Código do produto: ");
        int cod;
        scanf("%d", &cod);
        limpar_buffer();

        Produto p;
        int achou = 0;
        while (fread(&p, sizeof(Produto), 1, dat) == 1)
            if (p.codigo == cod) { classificar(&p); achou = 1; break; }
        fclose(dat);

        if (!achou)
        {
            printf("\n  Produto não encontrado.\n");
            pausar();
            return;
        }

        sprintf(nome_saida, "ficha_%03d.txt", p.codigo);
        FILE *txt = fopen(nome_saida, "w");
        if (!txt) { printf("\n  ERRO ao criar arquivo!\n"); pausar(); return; }

        char agora[20]; data_hoje(agora);
        fprintf(txt, "SISTEMA DE PRODUTOS - MERCADINHO v%s\n", VERSAO);
        fprintf(txt, "Gerado em: %s\n\n", agora);
        ficha_para_txt(txt, &p);
        fclose(txt);

        printf("\n  " GREEN "✔ Arquivo gerado: %s" RESET "\n", nome_saida);
        pausar();
        return;
    }

    /* ---- Opções 2 e 3: todos os registros ---- */
    FILE *txt = fopen(nome_saida, "w");
    if (!txt)
    {
        fclose(dat);
        printf("\n  ERRO ao criar arquivo!\n");
        pausar();
        return;
    }

    char agora[20]; data_hoje(agora);

    /* ---- Opção 2: lista em tabela ---- */
    if (op == 2)
    {
        fprintf(txt, "SISTEMA DE PRODUTOS - MERCADINHO v%s\n", VERSAO);
        fprintf(txt, "Relatório  : LISTA COMPLETA DE PRODUTOS\n");
        fprintf(txt, "Gerado em  : %s\n\n", agora);

        sep_txt(txt, '-', 80);
        fprintf(txt, "%-5s  %-20s  %-14s  %-5s  %-9s  %-9s  %-8s\n",
                "CÓD.", "NOME", "CATEGORIA", "QTD", "CUSTO", "VENDA", "SITUAÇÃO");
        sep_txt(txt, '-', 80);

        Produto p; int count = 0;
        while (fread(&p, sizeof(Produto), 1, dat) == 1)
        {
            classificar(&p);
            char n[21]; strncpy(n, p.nome,      20); n[20] = '\0';
            char c[15]; strncpy(c, p.categoria,  14); c[14] = '\0';
            fprintf(txt, "%-5d  %-20s  %-14s  %-5d  %-9.2f  %-9.2f  %-8s\n",
                    p.codigo, n, c, p.quantidade,
                    p.preco_custo, p.preco_venda,
                    label_prioridade(p.prioridade));
            count++;
        }
        sep_txt(txt, '-', 80);
        fprintf(txt, "Total: %d produto(s).\n", count);
    }

    /* ---- Opção 3: relatório gerencial completo ---- */
    else if (op == 3)
    {
        fprintf(txt, "================================================================\n");
        fprintf(txt, "  SISTEMA DE PRODUTOS - MERCADINHO v%s\n", VERSAO);
        fprintf(txt, "  RELATÓRIO GERENCIAL COMPLETO\n");
        fprintf(txt, "  Gerado em: %s\n", agora);
        fprintf(txt, "================================================================\n\n");

        int total = 0, ativos = 0, inativos = 0;
        int vencidos = 0, criticos = 0, atencao_c = 0, ok_c = 0;
        float v_custo = 0.0f, v_venda = 0.0f;
        Produto p;

        while (fread(&p, sizeof(Produto), 1, dat) == 1)
        {
            classificar(&p);
            total++;
            if (p.ativo) ativos++; else inativos++;
            if      (p.prioridade == 0) vencidos++;
            else if (p.prioridade == 1) criticos++;
            else if (p.prioridade == 2) atencao_c++;
            else ok_c++;
            v_custo += p.preco_custo * p.quantidade;
            v_venda += p.preco_venda * p.quantidade;
        }

        fprintf(txt, "RESUMO ESTATÍSTICO\n");
        sep_txt(txt, '-', 42);
        fprintf(txt, "  Total de produtos       : %d\n",     total);
        fprintf(txt, "  Ativos                  : %d\n",     ativos);
        fprintf(txt, "  Inativos                : %d\n",     inativos);
        fprintf(txt, "  [OK]                    : %d\n",     ok_c);
        fprintf(txt, "  [ATENÇÃO]               : %d\n",     atencao_c);
        fprintf(txt, "  [CRÍTICO]               : %d\n",     criticos);
        fprintf(txt, "  [VENCIDO]               : %d\n",     vencidos);
        fprintf(txt, "  Valor estoque (custo)   : R$ %.2f\n",v_custo);
        fprintf(txt, "  Valor estoque (venda)   : R$ %.2f\n",v_venda);
        if (total > 0)
            fprintf(txt, "  Margem bruta estimada   : R$ %.2f\n", v_venda - v_custo);
        fprintf(txt, "\n\n");

        fprintf(txt, "FICHAS INDIVIDUAIS\n\n");
        rewind(dat);
        while (fread(&p, sizeof(Produto), 1, dat) == 1)
        {
            classificar(&p);
            ficha_para_txt(txt, &p);
        }
    }

    fclose(dat);
    fclose(txt);
    printf("\n  " GREEN "✔ Arquivo gerado: %s" RESET "\n", nome_saida);
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
        printf("  ║      MERCADINHO  —  MÓDULO DE PRODUTOS       ║\n");
        printf("  ║               versão %-5s                   ║\n", VERSAO);
        printf("  ║                                              ║\n");
        printf("  ╠══════════════════════════════════════════════╣\n");
        printf("  ║  [1] Cadastrar novo produto                  ║\n");
        printf("  ║  [2] Consultar produto                       ║\n");
        printf("  ║  [3] Editar cadastro                         ║\n");
        printf("  ║  [4] Excluir / Inativar produto              ║\n");
        printf("  ╠══════════════════════════════════════════════╣\n");
        printf("  ║  [5] Listar todos os produtos                ║\n");
        printf("  ║  [6] Listar apenas ativos                    ║\n");
        printf("  ║  [7] Relatório resumido                      ║\n");
        printf("  ║  [8] Exportar para arquivo .TXT              ║\n");
        printf("  ╠══════════════════════════════════════════════╣\n");
        printf("  ║  [9] Movimentar estoque (Entrada / Saída)    ║\n");
        printf("  ║  [10] Consultar histórico (Auditoria)        ║\n");
        printf("  ╠══════════════════════════════════════════════╣\n");
        printf("  ║  [0] Sair do sistema                         ║\n");
        printf("  ╚══════════════════════════════════════════════╝\n\n");
        printf("  Total de produtos cadastrados: %d\n\n", total_registros());
        printf("  Sua opção: ");
        scanf("%d", &opcao);
        limpar_buffer();

        switch (opcao)
        {
            case 1:  cadastrar_produto();    break;
            case 2:  consultar_produto();    break;
            case 3:  editar_produto();       break;
            case 4:  excluir_produto();      break;
            case 5:  listar_produtos(0);     break;
            case 6:  listar_produtos(1);     break;
            case 7:  relatorio_resumo();     break;
            case 8:  exportar_txt();         break;
            case 9:  movimentar_estoque();   break;
            case 10: consultar_historico();  break;
            case 0:
                limpar_tela();
                printf("\n  Sistema encerrado. Até logo!\n\n");
                break;
            default:
                printf("\n  Opção inválida!\n");
                pausar();
        }
    } while (opcao != 0);
}

/* ==================== MAIN ================================ */

int main(void)
{
    setlocale(LC_ALL, "");
    menu_principal();
    return 0;
}

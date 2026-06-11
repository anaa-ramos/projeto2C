#include <stdio.h>
#include <locale.h>
#define erro "Opção inválida, tente novamente!\n"

   int main (){
    int escolha;

    setlocale(LC_ALL, "Portuguese");
    do {
        printf("==============================\n");
        printf("\tMERCADINHO DO PORTO\n");
        printf("==============================\n");
        printf("[1] PONTO DE VENDA\n");
        printf("[2] CONTROLE DE ESTOQUE\n");
        printf("[3] VENDAS E RELATÓRIOS\n");
        printf("[0] SAIR DO SISTEMA\n");
        scanf("%d", &escolha);

        if (escolha != 1 && escolha != 2 && escolha != 3 && escolha != 0){
            printf(erro);
        }
    } while (escolha != 0);

    return 0;
   }

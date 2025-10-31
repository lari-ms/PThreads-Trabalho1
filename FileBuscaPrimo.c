#pragma once
#pragma comment(lib,"pthreadVC2.lib")
#define HAVE_STRUCT_TIMESPEC

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SEED 2
#define ROWS 10000
#define COLS 10000
#define MACRO_ROWS 1000
#define MACRO_COLS 1000
#define N_THREADS 8

/*
"Faça testes com macroblocos de tamanhos diferentes, entre os
extremos: um único elemento e a matriz toda.
(desde 1x1 até as dimensões da matriz, algo como: 1x1,
10x10, 100x100, 1000x1000, ???, “matriz completa”)
Anote esses valores, pois serão usados no relatório"
*/


/*
"Faça uma análise dessa comparação:
Quantidade de Threads igual ao número de núcleos físicos x Quantidade
de Threads igual ao número de núcleos lógicos/virtuais, estimando,
assim, o ganho proporcionado pelo SMT."

2, 4 e 8 threads se o processador for 4 (8) núcleos (<= MEU PC)
*/

long long primos_count = 0;
int** matriz;
int proximoMacroDisponivel = 0;
int totalMacros = (ROWS * COLS) / (MACRO_ROWS * MACRO_COLS);

pthread_mutex_t mutex_pmacro_count;
pthread_mutex_t mutex_primos_count;

int isPrimo(int n) {
    if (n <= 1) {
        return 0;
    }
    if (n == 2) {
        return 1;
    }
    if (n % 2 == 0) {
        return 0;
    }

    double limite = sqrt(n);
    for (int i = 3; i <= limite; i += 2) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

double buscaSerial() {
    clock_t start_serial, end_serial;
    primos_count = 0;

	start_serial = clock();
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (isPrimo(matriz[i][j])) {
                primos_count++;
            }
        }
    }
	end_serial = clock();

	return ((double)(end_serial - start_serial)) / CLOCKS_PER_SEC;
}

void* buscaPrimo(void* arg) {
    
    int thisMacro;
    while (1) {
        long long primos_count_local = 0;
        //regiao critica
        pthread_mutex_lock(&mutex_pmacro_count);
        if (proximoMacroDisponivel >= totalMacros) {
            pthread_mutex_unlock(&mutex_pmacro_count);
            return NULL;
        }

        thisMacro = proximoMacroDisponivel;
        proximoMacroDisponivel++;
        pthread_mutex_unlock(&mutex_pmacro_count);
        //fim regiao critica

        int macros_por_linha = COLS / MACRO_COLS;
        int j_inicio = (thisMacro % macros_por_linha) * MACRO_COLS;
        int i_inicio = (thisMacro / macros_por_linha) * MACRO_ROWS;

        for (int i = i_inicio; i < i_inicio + MACRO_ROWS; i++) {
            for (int j = j_inicio; j < j_inicio + MACRO_COLS; j++) {
                if (isPrimo(matriz[i][j])) {
					primos_count_local++;
                }
            }
        }
        pthread_mutex_lock(&mutex_primos_count);
        primos_count +=primos_count_local;
        pthread_mutex_unlock(&mutex_primos_count);
    }
	
    return NULL;
}

float buscaParalela() {
    primos_count = 0;
    proximoMacroDisponivel = 0;

    clock_t start_paral, end_paral;
    
    pthread_mutex_init(&mutex_pmacro_count, NULL);
    pthread_mutex_init(&mutex_primos_count, NULL);
    
    pthread_t workers[N_THREADS];

    //criando as threads
	start_paral = clock();
    for (int i = 0; i < N_THREADS; i++) {
		pthread_create(&workers[i], NULL, buscaPrimo, NULL);//thread, attr, start_routine, arg
	}
    
    //dando join nas threads (finalizando)
    //ta certo isso ???
    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(workers[i], NULL);
    }
	end_paral = clock();

    pthread_mutex_destroy(&mutex_pmacro_count);
    pthread_mutex_destroy(&mutex_primos_count);

    if (CLOCKS_PER_SEC == 0) {
        printf("CLOCKS_PER_SEC = 0 !");
        return;
    }
	return ((float)(end_paral - start_paral)) / (float)CLOCKS_PER_SEC;
}


int main() {
    matriz = (int**)malloc(ROWS * sizeof(int*));
    if (matriz == NULL) {
        printf("erro ao alocar memória\n");
        free(matriz);
        return NULL;
    }

    //alocando cada linha
    for (int i = 0; i < ROWS; i++) {
        matriz[i] = (int*)malloc(COLS * sizeof(int));
        if (matriz[i] == NULL) {
            printf("erro ao alocar memoria\n");
            for (int j = 0; j < i; j++) {
                free(matriz[j]);
            }
            free(matriz);
            return NULL;//deveria retornar 1 aq, quando a alocacao falhar?
        }
    }

    //preenchendo a matriz
    srand(SEED);
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            matriz[i][j] = rand() % 32000; //numeros entre 0 e 31999
        }
    }

    /*
    "permita que os testes multi e single aconteçam
    numa única “rodada” do programa, ok? (Ou seja, que eu possa rodar tanto o serial quanto o paralelo na
    mesma execução). De posse desses tempos, já exiba o speedup (Lei de Amdahl) pois ele é o principal
    parâmetro a ser avaliado no quesito desempenho"
    */
    printf("matriz %dx%d    ..    macroblocos %dx%d\n", ROWS, COLS, MACRO_ROWS, MACRO_COLS);
	printf("total de macroblocos: %d\n", totalMacros);

	double tempo_serial = buscaSerial();
	printf("\n---BUSCA SERIAL---\n");
	printf("Quantidade de primos encontrados: %lld\nTempo decorrido: %fs\n", primos_count, tempo_serial);

	double tempo_paralelo = buscaParalela();
	printf("\n---BUSCA PARALELA---\n");
	printf("Quantidade de primos encontrados: %lld\nTempo decorrido: %fs\n", primos_count, tempo_paralelo);
	printf("Quantidade de threads: %d\n", N_THREADS);
	printf("\n---SPEEDUP---\n");
	printf("Speedup: %f\n", tempo_serial / tempo_paralelo);



    // Libera a memória
    for (int i = 0; i < ROWS; i++) {
        free(matriz[i]);
    }
    free(matriz);

    return 0;
}

/*
        MAIS COMENTARIOS DO GIRALDELI !!!
• Aumente muito o número de threads (algumas centenas ou mais) a fim de que o overhead possa realmente ficar
crítico e analise os resultados. Nesse caso, mantenha fixo o tamanho do macrobloco.
• Remova os mutexes que protegem as Regiões Críticas. Isso mesmo... remova as proteções temporariamente, rode o
programa com macroblocos pequenos e macroblocos grandes (por quê?) e observe os resultados.

*/


/*
#include <time.h>
#include <stdio.h>

int main () {
   clock_t start_t, end_t;
   double total_t;
   int i;

   start_t = clock();
   printf("Starting of the program, start_t = %ld\n", start_t);

   printf("Going to scan a big loop, start_t = %ld\n", start_t);
   for(i=0; i< 10000000; i++) {
   }
   end_t = clock();
   printf("End of the big loop, end_t = %ld\n", end_t);

   total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
   printf("Total time taken by CPU: %f\n", total_t  );
   printf("Exiting of the program...\n");

   return(0);
}
*/
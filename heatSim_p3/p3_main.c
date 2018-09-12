/*
// Projeto SO - exercicio 3
// Daniela Lopes,
// Francisco Matos, grupo 93
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"

#define GO 1
#define NOT_GO 0


int counter = 0, counter2 = 0, flag = GO, real_iter = 0, flagOut = NOT_GO;
double maxDiff = 0;
pthread_cond_t wait_for_all;
pthread_mutex_t mutex;


/*--------------------------------------------------------------------
| Type: thread_info
| Description: Estrutura com Informação para Trabalhadoras
---------------------------------------------------------------------*/

typedef struct {
    int id;
    int N;
    int iter;
    int trab;
    int tam_fatia;
    double maxD;
    DoubleMatrix2D *m, *aux;
}thread_info;


/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/

void *tarefa_trabalhadora(void* args){

  thread_info *tinfo = (thread_info *) args;
  DoubleMatrix2D *m, *aux, *tmp;
  int myId, colunas, iter, numIteracoes, numTarefas, tam_fatia, local_flag, i, j;
  double value, maxD, diff, tDiff;

  myId = tinfo->id - 1;
  colunas = tinfo->N+2;
  numIteracoes = tinfo->iter;
  numTarefas = tinfo->trab;
  tam_fatia = tinfo->tam_fatia;
  maxD = tinfo->maxD;
  local_flag = GO;


  m = tinfo->m;
  aux = tinfo->aux;

  for (iter = 0; iter < numIteracoes; iter++) {
    tDiff = 0;

    /*se a flag tiver sido accionada, todas as tarefas saem do ciclo*/
    if(flagOut == GO) break;

    real_iter = iter;

    for (i = (tam_fatia*myId)+1; i <= tam_fatia*(myId + 1) ; i++) {
      for (j = 1; j < colunas - 1; j++) {
        value = ( dm2dGetEntry(m, i-1, j) + dm2dGetEntry(m, i+1, j) +
                  dm2dGetEntry(m, i, j-1) + dm2dGetEntry(m, i, j+1) ) / 4.0;
        dm2dSetEntry(aux, i, j, value);
        diff = value - dm2dGetEntry(m,i,j);

        if(diff > tDiff) tDiff = diff;
      }
    }

    tmp = aux;
    aux = m;
    m = tmp;

	   /* BARREIRA */
    if(pthread_mutex_lock(&mutex) != 0) {
    	fprintf(stderr, "\nErro ao bloquear mutex\n");
    	exit(1);
    }

    local_flag = 1 - local_flag;
    counter++;
    if(tDiff > maxDiff) maxDiff = tDiff;

    if(counter == numTarefas) {
    	counter = 0;
      flag = local_flag;

      /*apenas e executados por uma tarefa*/
      if(maxDiff < maxD) flagOut = GO;
        maxDiff = 0;

    	if(pthread_cond_broadcast(&wait_for_all) != 0) {
    		fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
    		exit(1);
    	}

    } else {
    	while(local_flag != flag)
    		if(pthread_cond_wait(&wait_for_all, &mutex) != 0) {
    			fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
    			exit(1);
    		}

    }


    if(pthread_mutex_unlock(&mutex) != 0) {
    	fprintf(stderr, "\nErro ao bloquear mutex\n");
    	exit(1);
    }

  }

  return NULL;
}

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name)
{
  int value;

  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name)
{
  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}


/*--------------------------------------------------------------------
| Function: main
| Description: Entrada do programa
---------------------------------------------------------------------*/

int main (int argc, char** argv) {
  int N;
  double tEsq, tSup, tDir, tInf;
  int iter;
  int trab;
  double maxD;
  int tam_fatia;
  int res;
  int i;
  DoubleMatrix2D  *matrix, *matrix_aux;
  thread_info *tinfo;
  pthread_t *trabalhadoras;


 if(argc != 9) {
    fprintf(stderr, "\nNúmero de Argumentos Inválido.\n");
    fprintf(stderr, "Utilização: heatSim_p1 N tEsq tSup tDir tInf iter trab csz\n\n");
    return -1;
  }

  /* Ler Input */
  N = parse_integer_or_exit(argv[1], "n");
  tEsq = parse_double_or_exit(argv[2], "tEsq");
  tSup = parse_double_or_exit(argv[3], "tSup");
  tDir = parse_double_or_exit(argv[4], "tDir");
  tInf = parse_double_or_exit(argv[5], "tInf");
  iter = parse_integer_or_exit(argv[6], "iter");
  trab = parse_integer_or_exit(argv[7], "trab");
  maxD =  parse_double_or_exit(argv[8], "maxD");

  fprintf(stderr, "\nArgumentos:\n"
   " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d maxD=%f\n",
   N, tEsq, tSup, tDir, tInf, iter, trab, maxD);


  /* Verificações de Input */
  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 || trab < 1 || maxD < 0) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
  " Lembrar que N >= 1, temperaturas >= 0, iter >= 1, trab >=1 e maxD >= 0\n\n");
    return -1;
  }

  if (N % trab != 0) {
    fprintf(stderr, "\nErro: Argumento %s e %s invalidos\n"
            "%s deve ser multiplo de %s.", "N", "trab", "N", "trab");
    return -1;
  }

  /* Calcular Tamanho de cada Fatia */
  tam_fatia = N/trab;

  /* Criar Matriz Inicial */
  matrix = dm2dNew(N+2, N+2);
  matrix_aux = dm2dNew(N+2, N+2);


  if (matrix == NULL || matrix_aux == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes.\n\n");
    return -1;
  }

  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);

  dm2dCopy (matrix_aux, matrix);

/* Reservar Memória para Trabalhadoras */
  tinfo = (thread_info *)malloc(trab * sizeof(thread_info));
  trabalhadoras = (pthread_t *)malloc(trab * sizeof(pthread_t));

  if (tinfo == NULL || trabalhadoras == NULL) {
    fprintf(stderr, "\nErro ao alocar memória para trabalhadoras.\n");
    return -1;
  }

  if(pthread_mutex_init(&mutex, NULL) != 0) {
    fprintf(stderr, "\nErro ao inicializar mutex\n");
    exit(1);
  }

  if(pthread_cond_init(&wait_for_all, NULL) != 0) {
    fprintf(stderr, "\nErro ao inicializar variável de condição\n");
    exit(1);
  }

  /* Criar Trabalhadoras */
  for (i = 0; i < trab; i++) {
    tinfo[i].id = i+1;
    tinfo[i].N = N;
    tinfo[i].iter = iter;
    tinfo[i].trab = trab;
    tinfo[i].tam_fatia = tam_fatia;
    tinfo[i].maxD = maxD;
    tinfo[i].m = matrix;
    tinfo[i].aux = matrix_aux;
    res = pthread_create(&trabalhadoras[i], NULL, tarefa_trabalhadora, &tinfo[i]);

    if(res != 0) {
      fprintf(stderr, "\nErro ao criar uma tarefa trabalhadora.\n");
      return -1;
    }
  }

  /* Esperar que as Trabalhadoras Terminem */
  for (i = 0; i < trab; i++) {
    res = pthread_join(trabalhadoras[i], NULL);

    if (res != 0) {
      fprintf(stderr, "\nErro ao esperar por uma tarefa trabalhadora.\n");
      return -1;
    }
  }

  if(real_iter % 2 == 0) dm2dPrint(matrix_aux);
  else dm2dPrint(matrix);

/* Libertar Memória */
  dm2dFree(matrix);
  dm2dFree(matrix_aux);
  free(tinfo);
  free(trabalhadoras);

  if(pthread_mutex_destroy(&mutex) != 0) {
      fprintf(stderr, "\nErro ao destruir mutex\n");
      return -1;
  }

  if(pthread_cond_destroy(&wait_for_all) != 0) {
      fprintf(stderr, "\nErro ao destruir variável de condição\n");
      return -1;
  }

  return 0;
}

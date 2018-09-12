/*
 Projeto SO - exercicio 1, version 03
 Sistemas Operativos, DEI/IST/ULisboa 2017-18

 ist86403 - Daniela Lopes
 ist86415 - Francisco Matos
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"
#include "leQueue.h"
#include "mplib3.h"

typedef struct {
  int id; /*identificador da tarefa*/
  int lin; /*n de linhas de cada fatia*/
  int col; /*n de colunas de cada fatia*/
  int nIt; /*n de iteracoes*/
  int nTarefas; /*numero total de tarefas*/
} args_mythread_t;


/*--------------------------------------------------------------------
| Function: simul
  Versão sequencial do simulador
---------------------------------------------------------------------*/


DoubleMatrix2D *simul(DoubleMatrix2D *matrix, DoubleMatrix2D *matrix_aux, int linhas, int colunas, int numIteracoes) {

  DoubleMatrix2D *m, *aux, *tmp;
  int iter, i, j;
  double value;


  if(linhas < 2 || colunas < 2)
    return NULL;

  m = matrix;
  aux = matrix_aux;

  for (iter = 0; iter < numIteracoes; iter++) {
  
    for (i = 1; i < linhas - 1; i++)
      for (j = 1; j < colunas - 1; j++) {
        value = ( dm2dGetEntry(m, i-1, j) + dm2dGetEntry(m, i+1, j) +
		dm2dGetEntry(m, i, j-1) + dm2dGetEntry(m, i, j+1) ) / 4.0;
        dm2dSetEntry(aux, i, j, value);
      }

    tmp = aux;
    aux = m;
    m = tmp;
  }

  return m;
}

/*--------------------------------------------------------------------
| Function: parSimul
  Versão paralela do simulador
---------------------------------------------------------------------*/

void *parSimul(void *a) {

  args_mythread_t *args = (args_mythread_t *) a; /*cast*/

  DoubleMatrix2D *slice, *slice_aux, *tmp;
  int iter, numIteracoes, linhas, colunas, myId,numTarefas, i, j;
  double value;

  numIteracoes = args->nIt;
  linhas = args->lin; 
  colunas = args->col;
  myId = args->id;
  numTarefas = args->nTarefas;

  double receive_buff[colunas];

  if(linhas-2 < 2 || colunas-2 < 2)
    return NULL;

  slice = dm2dNew(linhas, colunas); 
  slice_aux = dm2dNew(linhas, colunas);

  /*recebe as linhas da matriz original correspondentes a fatia interior*/
  for (i=0;i<linhas;i++) {    
    if(receberMensagem(0, myId,receive_buff, sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
    dm2dSetLine(slice, i,receive_buff);
  }

  dm2dCopy (slice_aux, slice);

  for(iter = 0; iter < numIteracoes; iter++) {

    for(i = 1; i < linhas-1; i++)
      for(j=1; j < colunas-1; j++) {
        value = ( dm2dGetEntry(slice, i-1, j) + dm2dGetEntry(slice, i+1, j) +
    dm2dGetEntry(slice, i, j-1) + dm2dGetEntry(slice, i, j+1) ) / 4.0;
        dm2dSetEntry(slice_aux, i, j, value);

      }

    dm2dPrint(slice);
    dm2dPrint(slice_aux);

  /*no final de cada iteracao e necessario realizar-se trocas de mensagens entre as tarefas*/

    if(myId % 2 != 0) {
      if(myId == 1) { 
        if(enviarMensagem(myId, myId+1, dm2dGetLine(slice_aux, linhas-2), sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
        if(receberMensagem(myId+1, myId, receive_buff, sizeof(double) * colunas) == -1) pthread_exit((void*)-1);
        dm2dSetLine (slice_aux, linhas-1, receive_buff);
      }

      else if (myId == numTarefas) { 
        if(enviarMensagem(myId, myId-1, dm2dGetLine(slice_aux, 1), sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
        if(receberMensagem(myId-1, myId, receive_buff, sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
        dm2dSetLine (slice_aux, 0, receive_buff);
      }

      else { 
        if(enviarMensagem(myId, myId-1, dm2dGetLine(slice_aux, 1), sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
        if(receberMensagem(myId-1, myId, receive_buff, sizeof(double) * colunas) ==-1) pthread_exit((void*)-1);
        dm2dSetLine (slice_aux, 0, receive_buff);

        if(enviarMensagem(myId, myId+1, dm2dGetLine(slice_aux, linhas-2), sizeof(double) * colunas) ==-1) pthread_exit((void*)-1);
        if(receberMensagem(myId+1, myId, receive_buff, sizeof(double) * colunas) ==-1) pthread_exit((void*)-1);
        dm2dSetLine (slice_aux, linhas-1, receive_buff);
      }
    }
    else {
      if(myId == numTarefas){ 
        if(receberMensagem(myId-1, myId, receive_buff, sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
        dm2dSetLine (slice_aux, 0, receive_buff);
        if(enviarMensagem(myId, myId-1, dm2dGetLine(slice_aux, 1), sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
      }

      else{ 
        if(receberMensagem(myId-1, myId, receive_buff, sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
        dm2dSetLine (slice_aux, 0, receive_buff);
        if(enviarMensagem(myId, myId-1, dm2dGetLine(slice_aux, 1), sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
        
        if(receberMensagem(myId+1, myId, receive_buff, sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
        dm2dSetLine (slice_aux, linhas-1, receive_buff);
        if(enviarMensagem(myId, myId+1, dm2dGetLine(slice_aux, linhas-2), sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
      }
    }

    tmp = slice_aux;
    slice_aux = slice;
    slice = tmp;
  }

  /*Depois de efetuados os calculos, as linhas da fatia interior sao enviadas para a matriz principal */
  for (i=1;i<linhas-1;i++) {   
    if(enviarMensagem (myId, 0, dm2dGetLine(slice_aux, i), sizeof(double) * colunas)==-1) pthread_exit((void*)-1);
  }


  dm2dFree(slice);
  dm2dFree(slice_aux);
  return 0;
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
---------------------------------------------------------------------*/

int main (int argc, char** argv) {

  if(argc != 9) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trab csz\n\n");
    return 1;
  }

  /* argv[0] = program name */
  int N = parse_integer_or_exit(argv[1], "N");
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
  int trab = parse_integer_or_exit(argv[7], "trab");
  int csz = parse_integer_or_exit(argv[8], "csz");

  DoubleMatrix2D *matrix, *matrix_aux, *result;


  fprintf(stderr, "\nArgumentos:\n"
	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d trab=%d csz=%d\n",
	N, tEsq, tSup, tDir, tInf, iteracoes, trab, csz);

  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || trab < 1 || csz < 0 || N%trab!=0) {
    fprintf(stderr, "\nErro: Ar`gumentos invalidos.\n");
	  fprintf(stderr, " Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, trab >= 0, N mod trab == 0 e csz >= 0\n\n");
    return 1;
  }


  matrix = dm2dNew(N+2, N+2);
  matrix_aux = dm2dNew(N+2, N+2);

  if (matrix == NULL || matrix_aux == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes.\n\n");
    return -1;
  }


  int i, j;


  for(i=0; i<N+2; i++)
    dm2dSetLineTo(matrix, i, 0);

  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);

  dm2dCopy (matrix_aux, matrix);

  /*--------------------------------------------------------------------
| Criacao das tarefas
---------------------------------------------------------------------*/
  args_mythread_t *slave_args;
  int              numTarefas;
  int              linInt;
  pthread_t       *slaves;
  double          buffer[N+2];

/*linInt corresponde ao numero de linhas da fatia interior de cada tarefa*/
  numTarefas = trab;
  linInt = N/numTarefas; 

  /*caso so haja uma tarefa faz-se uma simulacao sequencial e nao se criam tarefas*/
  if (numTarefas == 1) { 
    result = simul(matrix, matrix_aux, N+2, N+2, iteracoes);
    if (result == NULL) {
      printf("\nErro na simulacao.\n\n");
      return -1;
    } 
    else {
      dm2dPrint(result);
    }
  }

  else {


    slave_args = (args_mythread_t*)malloc(numTarefas*sizeof(args_mythread_t));
    slaves     = (pthread_t*)malloc(numTarefas*sizeof(pthread_t));

    if (inicializarMPlib(csz, trab + 1) == -1) {
      printf("\nErro na inicializacao da MPlib\n\n");
      return 1;
    }
    
    /*criar tarefas escravas*/
    for (i=0; i<numTarefas; i++) {
      slave_args[i].id = i+1; 
      slave_args[i].lin = linInt+2;
      slave_args[i].col = N+2;
      slave_args[i].nIt = iteracoes;
      slave_args[i].nTarefas = numTarefas;
      pthread_create(&slaves[i], NULL, parSimul, &slave_args[i]);
    }

    /*envia a cada tarefa as linhas iniciais das fatias correspondentes*/
    for (i=0; i<numTarefas; i++) {
      for (j=linInt*i; j<=linInt*(i+1)+1; j++) { 
        if(enviarMensagem(0, i+1, dm2dGetLine(matrix, j), sizeof(double) * (N+2))==-1) return 1;
      }
    }

    /*recebe as linhas calculadas de cada tarefa e coloca-as na matriz principal*/
    for (i=0; i<numTarefas; i++) {
      for (j=linInt*i+1; j<=linInt*(i+1); j++) { 
        if(receberMensagem(i+1, 0, buffer, sizeof(double) * (N+2))==-1) return 1;
        dm2dSetLine(matrix, j, buffer);
      }
    }

    for (i=0; i<numTarefas; i++) {
      pthread_join(slaves[i], NULL);
    }


    dm2dPrint(matrix);

    libertarMPlib();
    free(slaves);
    free(slave_args);


  }

  dm2dFree(matrix);
  dm2dFree(matrix_aux);


  return 0;
}

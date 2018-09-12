/*
// Projeto SO - exercicio 3
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "matrix2d.h"
#include "util.h"

/*--------------------------------------------------------------------
| Type: thread_info
| Description: Estrutura com Informacao para Trabalhadoras
---------------------------------------------------------------------*/

typedef struct {
  int    id;
  int    N;
  int    iter;
  int    trab;
  int    tam_fatia;
  double maxD;
} thread_info;

/*--------------------------------------------------------------------
| Type: doubleBarrierWithMax
| Description: Barreira dupla com variavel de max-reduction
---------------------------------------------------------------------*/

typedef struct {
  int             total_nodes;
  int             pending[2];
  double          maxdelta[2];
  int             iteracoes_concluidas;
  pthread_mutex_t mutex;
  pthread_cond_t  wait[2];
  int             process_counter;
  int             periodoS;
} DualBarrierWithMax;

/*--------------------------------------------------------------------
| Global variables
---------------------------------------------------------------------*/

DoubleMatrix2D     *matrix_copies[2];
DualBarrierWithMax *dual_barrier;
double              maxD;
char               *fichS;
char               *fichS_aux;
pid_t               pid;
int                 status;

int                 flag_alarm = 0;
int                 flag_int = 0;

/*--------------------------------------------------------------------
| Function: codigoDoFilho
| Description: Funcao executada por cada processo filho.
|              Recebe como argumento uma matriz e cria um ficheiro
              de salvaguarda dessa matriz
---------------------------------------------------------------------*/

void *codigoDoFilho(DoubleMatrix2D *matrix) {
  FILE *f = fopen(fichS_aux, "w");
  dm2dPrintToFile(matrix, f); //fazer verificacoes

  if (fclose(f) != 0)
    die("Erro ao fechar ficheiro");
  if (rename(fichS_aux, fichS)==-1)
    die("Erro ao alterar o nome do ficheiro");

  return 0;
}

/*--------------------------------------------------------------------
| Function: trata_signal_alarm
| Description:
---------------------------------------------------------------------*/

void trata_signal_alarm(int n) {
  if (signal(SIGALRM, trata_signal_alarm) == SIG_ERR)
    printf("Erro no SIGALARM");
  alarm(dual_barrier->periodoS);
  flag_alarm = 1;
}

/*--------------------------------------------------------------------
| Function: trata_signal_int
| Description:
---------------------------------------------------------------------*/

void trata_signal_int(int n) {
  flag_int = 1;
}
/*--------------------------------------------------------------------
| Function: dualBarrierInit
| Description: Inicializa uma barreira dupla
---------------------------------------------------------------------*/

DualBarrierWithMax *dualBarrierInit(int ntasks, int periodo) {
  DualBarrierWithMax *b;
  b = (DualBarrierWithMax*) malloc (sizeof(DualBarrierWithMax));

  b->total_nodes = ntasks;
  b->pending[0]  = ntasks;
  b->pending[1]  = ntasks;
  b->maxdelta[0] = 0;
  b->maxdelta[1] = 0;
  b->iteracoes_concluidas = 0;
  b->process_counter = 0;
  b->periodoS = periodo;

  pthread_mutex_init(&(b->mutex), NULL);
  pthread_cond_init(&(b->wait[0]), NULL);
  pthread_cond_init(&(b->wait[1]), NULL);
  return b;
}

/*--------------------------------------------------------------------
| Function: dualBarrierFree
| Description: Liberta os recursos de uma barreira dupla
---------------------------------------------------------------------*/

void dualBarrierFree(DualBarrierWithMax* b) {
  pthread_mutex_destroy(&(b->mutex));
  pthread_cond_destroy(&(b->wait[0]));
  pthread_cond_destroy(&(b->wait[1]));
  free(b);
}

/*--------------------------------------------------------------------
| Function: dualBarrierWait
| Description: Ao chamar esta funcao, a tarefa fica bloqueada ate que
|              o numero 'ntasks' de tarefas necessario tenham chamado
|              esta funcao, especificado ao ininializar a barreira em
|              dualBarrierInit(ntasks). Esta funcao tambem calcula o
|              delta maximo entre todas as threads e devolve o
|              resultado no valor de retorno
---------------------------------------------------------------------*/

double dualBarrierWait (DualBarrierWithMax* b, int current, double localmax) {
  int status;
  int next = 1 - current;
  if (pthread_mutex_lock(&(b->mutex)) != 0) {
    die("Erro ao bloquear mutex");
  }
  // decrementar contador de tarefas restantes
  b->pending[current]--;
  // actualizar valor maxDelta entre todas as threads
  if (b->maxdelta[current]<localmax)
    b->maxdelta[current]=localmax;
  // verificar se sou a ultima tarefa
  if (b->pending[current]==0) {
    // sim -- inicializar proxima barreira e libertar threads
    b->iteracoes_concluidas++;
    b->pending[next]  = b->total_nodes;
    b->maxdelta[next] = 0;

    // se o alarme ja tiver sido ativado
    if (flag_alarm == 1) {
      flag_alarm = 0;
      // se for o primeiro processo filho, nao precisa de verificar se ha algum processo filho ainda em execucao
      // waitpid - opcao nao bloqueante, o processo filho anterior ja terminou
      if (b->process_counter == 0 || waitpid(pid, &status, WNOHANG) != 0) {
        pid = fork();
        dual_barrier->process_counter++;
        // processo filho
        if (pid == 0) {
          codigoDoFilho(matrix_copies[next]);
          exit(0);
        }
        // se fork() der erro
        else if (pid == -1) {
          die("Fork() falhou");
        }
      }
    }
    else if (flag_int == 1) {
      pid = fork();
      // processo filho
      if (pid == 0) {
        codigoDoFilho(matrix_copies[next]);
        exit(0);
      }
      // se fork() der erro
      else if (pid == -1) {
        die("Fork() falhou");
      }
      else {
        wait(&status); // esperar que o filho termine
        flag_int = 1;
      }
    }
    if (pthread_cond_broadcast(&(b->wait[current]))!=0) {
      die("Erro ao desbloquear a variaver de condicao");
    }
  }
  else {
    // nao -- esperar pelas outras tarefas
    while (b->pending[current]>0)
      if (pthread_cond_wait(&(b->wait[current]), &(b->mutex))!=0) {
        die("Erro ao esperar pela variavel de condicao");
      }
  }
  double maxdelta = b->maxdelta[current];
  if (pthread_mutex_unlock(&(b->mutex))!=0) {
    die("Erro ao desbloquear mutex");
  }
  return maxdelta;
}

/*--------------------------------------------------------------------
| Function: tarefa_trabalhadora
| Description: Funcao executada por cada tarefa trabalhadora.
|              Recebe como argumento uma estrutura do tipo thread_info
---------------------------------------------------------------------*/

void *tarefa_trabalhadora(void *args) {
  thread_info *tinfo = (thread_info *) args;

  int iter = 0;
  int tam_fatia = tinfo->tam_fatia;
  int my_base = tinfo->id * tam_fatia;

  double global_delta = INFINITY;


  do {
    int atual = iter % 2;
    int prox = 1 - iter % 2;
    double max_delta = 0;

    // Calcular Pontos Internos
    for (int i = my_base; i < my_base + tinfo->tam_fatia; i++) {
      for (int j = 0; j < tinfo->N; j++) {
        double val = (dm2dGetEntry(matrix_copies[atual], i,   j+1) +
                      dm2dGetEntry(matrix_copies[atual], i+2, j+1) +
                      dm2dGetEntry(matrix_copies[atual], i+1, j) +
                      dm2dGetEntry(matrix_copies[atual], i+1, j+2))/4;
        // calcular delta
        double delta = fabs(val - dm2dGetEntry(matrix_copies[atual], i+1, j+1));
        if (delta > max_delta) {
          max_delta = delta;
        }
        dm2dSetEntry(matrix_copies[prox], i+1, j+1, val);
      }
    }
    // barreira de sincronizacao; calcular delta global
    global_delta = dualBarrierWait(dual_barrier, atual, max_delta);
  } while (flag_int == 0 && ++iter < tinfo->iter && global_delta >= tinfo->maxD);

  return 0;
}

/*--------------------------------------------------------------------
| Function: main
| Description: Entrada do programa
---------------------------------------------------------------------*/

int main (int argc, char** argv) {
  int N;
  double tEsq, tSup, tDir, tInf;
  int iter, trab;
  int tam_fatia;
  int res;
  int periodoS;
  FILE *fp;

  if (argc != 11) {
    fprintf(stderr, "Utilizacao: ./heatSim N tEsq tSup tDir tInf iter trab maxD fichS periodoS\n\n");
    die("Numero de argumentos invalido");
  }

  // Ler Input
  N    = parse_integer_or_exit(argv[1], "N",    1);
  tEsq = parse_double_or_exit (argv[2], "tEsq", 0);
  tSup = parse_double_or_exit (argv[3], "tSup", 0);
  tDir = parse_double_or_exit (argv[4], "tDir", 0);
  tInf = parse_double_or_exit (argv[5], "tInf", 0);
  iter = parse_integer_or_exit(argv[6], "iter", 1);
  trab = parse_integer_or_exit(argv[7], "trab", 1);
  maxD = parse_double_or_exit (argv[8], "maxD", 0);
  fichS = argv[9];
  periodoS = parse_integer_or_exit (argv[10], "periodoS", 1);

  fprintf(stderr, "\nArgumentos:\n"
    " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d maxD=%.1f fichS=%s periodoS=%d \n" ,
    N, tEsq, tSup, tDir, tInf, iter, trab, maxD, fichS, periodoS);

  //criar nome do ficheiro auxiliar
  fichS_aux = (char*)malloc(sizeof(char) * (strlen(fichS) + 2));
  strcpy(fichS_aux, fichS);
  strcat(fichS_aux, "~");

  if (N % trab != 0) {
    fprintf(stderr, "\nErro: Argumento %s e %s invalidos.\n"
                    "%s deve ser multiplo de %s.", "N", "trab", "N", "trab");
    return -1;
  }

  // Inicializar Barreira
  dual_barrier = dualBarrierInit(trab, periodoS);
  if (dual_barrier == NULL)
    die("Nao foi possivel inicializar barreira");

  // Calcular tamanho de cada fatia
  tam_fatia = N / trab;

  // Criar e Inicializar Matrizes
  matrix_copies[1] = dm2dNew(N+2,N+2);
  if(matrix_copies[1] == NULL)
    die("Erro ao criar matrizes");

  // abrir ficheiro com nome fichS
  fp = fopen(fichS, "r");
  if (fp == NULL) { // se nao encontrar o ficheiro com esse nome
    matrix_copies[0] = dm2dNew(N+2,N+2);
    if (matrix_copies[0] == NULL)
      die("Erro ao criar matrizes");

    dm2dSetLineTo (matrix_copies[0], 0, tSup);
    dm2dSetLineTo (matrix_copies[0], N+1, tInf);
    dm2dSetColumnTo (matrix_copies[0], 0, tEsq);
    dm2dSetColumnTo (matrix_copies[0], N+1, tDir);
  }
  else { // se encontrar ficheiro com esse nome
    matrix_copies[0] = readMatrix2dFromFile(fp, N+2, N+2);
    fclose(fp); // so fecha o ficheiro se abrir algum

    if (matrix_copies[0] == NULL)
      die("Erro ao criar matrizes");

  }

  dm2dCopy (matrix_copies[1],matrix_copies[0]);

  // Reservar memoria para trabalhadoras
  thread_info *tinfo = (thread_info*) malloc(trab * sizeof(thread_info));
  pthread_t *trabalhadoras = (pthread_t*) malloc(trab * sizeof(pthread_t));

  if (tinfo == NULL || trabalhadoras == NULL) {
    die("Erro ao alocar memoria para trabalhadoras");
  }

  // passa para a rotina de tratamento do sinal do alarme a cada periodoS
  if (signal(SIGALRM, trata_signal_alarm) == SIG_ERR)
    printf("Erro no SIGALARM");

  //passa para a rotina de tratamento do sinal de interrupcao ctrl + c
  if (signal(SIGINT, trata_signal_int) == SIG_ERR)
    printf("Erro no SIGINT");

  //comeca a contagem de segundos entre as salvaguardas
  alarm(periodoS);

  // Criar trabalhadoras
  for (int i=0; i < trab; i++) {
    tinfo[i].id = i;
    tinfo[i].N = N;
    tinfo[i].iter = iter;
    tinfo[i].trab = trab;
    tinfo[i].tam_fatia = tam_fatia;
    tinfo[i].maxD = maxD;
    res = pthread_create(&trabalhadoras[i], NULL, tarefa_trabalhadora, &tinfo[i]);
    if (res != 0) {
      die("Erro ao criar uma tarefa trabalhadora");
    }
  }


  // Esperar que as trabalhadoras terminem
  for (int i=0; i<trab; i++) {
    res = pthread_join(trabalhadoras[i], NULL);
    if (res != 0)
      die("Erro ao esperar por uma tarefa trabalhadora");
  }

  wait(&status);

  /*apaga o ficheiro de salvaguarda se o programa chegar ao fim*/
  if(flag_int == 0) {
    dm2dPrint (matrix_copies[dual_barrier->iteracoes_concluidas%2]);
    if(access(fichS, F_OK) == 0)
      if (unlink(fichS) != 0)
        die("O ficheiro nao foi apagado corretamente");
  }


  // Libertar memoria
  dm2dFree(matrix_copies[0]);
  dm2dFree(matrix_copies[1]);
  free(tinfo);
  free(trabalhadoras);
  dualBarrierFree(dual_barrier);
  free(fichS_aux);

  return 0;
}

// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#include <ucontext.h>
#include "queue.h"

#ifndef __DATATYPES__
#define __DATATYPES__

//Estado de uma tarefa (conforme diagrama de estados): Nova, Pronta, Suspensa, Executando e Terminada.
enum status_t{NOVA, PRONTA, SUSPENSA, EXECUTANDO, TERMINADA};

// Estrutura que define uma tarefa
typedef struct task_t
{
    ucontext_t context;     //Contexto da tarefa
    enum status_t status;   //Estado da tarefa
    struct task_t *parent;  //"Pai" da tarefa (tarefa em execução quando esta tarefa foi criada)
    struct task_t *prev;    //Próxima tarefa da fila
    struct task_t *next;    //Tarefa anterior da fila
    int id;                //Id da tarefa
    struct queue_t **fila_atual; //Mostra a ID da fila atual
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

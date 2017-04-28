/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "pingpong.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>

#define STACKSIZE 32768 /*Tamanho da pilha de threads*/

task_t tarefa_principal, *tarefa_atual = NULL, despachante, *tarefas_prontas = NULL;
int count = 0;
int userTasks = 0;

// funções gerais ==============================================================
/** >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Funções da P03<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
//Inicializa variáveis da tarefa principal
void inicializa_tarefa_principal();

//Função despachante de tarefas (corpo associada à tarefa despachante)
void dispatcher_body(void *arg);

//Despachante de tarefas
task_t *scheduler();

//Altera o estado de uma tarefa para PRONTA e adicona na fila de tarefas prontas
int task_set_ready(task_t* task);

//Altera o estado de uma tarefa para EXECUTANDO e à retira da fila da qual pertence
int task_set_executing(task_t* task);

/** >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Funções da P03<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

// Inicializa o sistema operacional; deve ser chamada no inicio do main()

void pingpong_init() {

    setvbuf(stdout, 0, _IONBF, 0); //desativa o buffer de saida padrao (stdout), usado pela função printf.
    inicializa_tarefa_principal();
    task_create(&despachante, dispatcher_body, "despachante :"); //Inicializa despachante de tarefas
}

//Inicializa tarefa principal

void inicializa_tarefa_principal() {
    //A tarefa principal não pertence a nenhuma fila
    tarefa_principal.next = NULL;
    tarefa_principal.prev = NULL;
    tarefa_principal.fila_atual = NULL;

    tarefa_principal.id = count++; //ID da tarefa principal
    tarefa_principal.parent = NULL; //A primeira tarefa não possui pai,...
    tarefa_principal.status = EXECUTANDO; //... já está em execução quando foi criada ...

    tarefa_atual = &tarefa_principal; //... e é a tarefa em execução no momento.
}
// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.

int task_create(task_t *task, // descritor da nova tarefa
        void (*start_func)(void *), // funcao corpo da tarefa
        void *arg) { // argumentos para a tarefa   

    //Exceptions
    if (!task) {
        perror("Tarefa vazia: ");
        return -1;
    }else{

    task->status = NOVA;
    //Iniciando tarefa, no momento ela não pertence a nenhuma fila
    task->next = NULL;
    task->prev = NULL;
    task->fila_atual = NULL;

    getcontext(&(task->context));
    char *pilha = malloc(STACKSIZE);

    if (pilha) {
        task->context.uc_stack.ss_sp = pilha;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
        task->id = count++;
        task->parent = tarefa_atual;

    } else {
        perror("Erro ao criar pilha");
        return -1;
    }
    }
    makecontext(&task->context, (void*) (*start_func), 1, arg); //Associa o contexto à função passada por argumento    

    //Tarefa de usuário é sempre maior que 1
    if (task->id > 1) {
        userTasks++; //Nova tarefa de usuário criada
        //queue_append((queue_t **) &tarefas_prontas, (queue_t *) task);
       // task->fila_atual = (queue_t **) &tarefas_prontas;        
        if (task_set_ready(task)) //Tenta mudar seu estado para PRONTO e inserir na fila de prontos
        {
            perror("Erro ao mudar estado para PRONTA.");
            return -1;
        }
        
    } else {
        task->status = PRONTA; //Finalizada as inicializações da tarefa
    }

    

#ifdef DEBUG

    printf("task_create: criou a tarefa  nro %d\n", task->id);

#endif 

    return task->id;
}

// Termina a tarefa corrente, indicando um valor de status encerramento

void task_exit(int exitCode) {

    if (tarefa_atual->id == 0) {
        exit(exitCode);
    }

    task_t *tarefa_final = tarefa_atual;

    if (tarefa_final->id == 1) {
        tarefa_atual = &tarefa_principal;
    } else {
        tarefa_atual = &despachante;
    }
    userTasks--;
    tarefa_final->status = TERMINADA;
    tarefa_atual->status = EXECUTANDO;

#ifdef DEBUG
    printf("task_exit: tarefa %d sendo encerrado com codigo %d\n", tarefa_final->id, exitCode);
#endif // DEBUG

    //Efetua a troca de contexto da a última tarefa e a tarefa principal
    swapcontext(&tarefa_final->context, &tarefa_atual->context);
}

// alterna a execução para a tarefa indicada

int task_switch(task_t *task) {
    //Exceptions
    if (!task) {
        perror("Tarefa vazia: ");
        return -1;
    }

    task_t *tarefa_final = tarefa_atual;
    tarefa_atual = task;

    //tarefa_final->status = PRONTA;
    //tarefa_atual->status = EXECUTANDO;

#ifdef DEBUG
    printf("task_switch: trocando contexto %d -> %d\n", tarefa_final->id, tarefa_atual->id);
#endif // DEBUG

    //Troca o contexto entre as tarefas passadas como parâmetro
    swapcontext(&tarefa_final->context, &tarefa_atual->context);
    return 0;

}

// retorna o identificador da tarefa corrente (main eh 0)

int task_id() {
    return tarefa_atual->id;
}

// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"; usa a tarefa atual se task==NULL

void task_suspend(task_t *task, task_t **queue) {
    task_t * working_task;

    if (task) //Caso passado uma tarefa como parâmetro...
        working_task = task; //... se trabalhará com ela, ...
    else
        working_task = tarefa_atual; //... caso contrário, utilize a tarefa em execução.

    working_task->status = SUSPENSA; //Suspende a tarefa em trabalho

    if (queue) //Se for passado uma fila como parâmetro...
    {
        if (working_task->fila_atual) //... verifique se a tarefa está contida em alguma fila, ...
            queue_remove(working_task->fila_atual, (queue_t *) working_task); //... se estiver, remova-a da fila atual e, ...
        queue_append((queue_t **) queue, (queue_t *) working_task); //... em seguida, adicione à fila passado por parâmetro, ...
        working_task->fila_atual = (queue_t **) queue; //... atualizando para a nova fila em que se encontra.
    }

#ifdef DEBUG
    printf("task_suspend: tarefa %d entrando em suspensão.\n", working_task->id);
#endif  //DEBUG


    //Volta para o despachante, caso a tarefa seja a corrente
    if (!task)
        task_switch(&despachante);
}

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"

void task_resume(task_t *task) {
    task_set_ready(task);

#ifdef DEBUG
    printf("task_resume: tarefa %d preparada para execução\n", task->id);
#endif  
}

void task_yield() {
#ifdef DEBUG
    printf("task_yield: liberando-se da tarefa %d\n", tarefa_atual->id);
#endif  

    if (tarefa_atual->id > 1) //Caso seje uma tarefa de usuário...
    {
        if (task_set_ready(tarefa_atual)) //Insere a tarefa corrente na fila de prontas, mudando seu estado para PRONTO, ...
        {
            perror("Erro ao mudar estado para PRONTA.");
            exit(-1);
        }
    } else
        tarefa_atual->status = PRONTA; //Caso contrário, apenas muda seu estado para PRONTO

    //Retorna para o despachante
    task_switch(&despachante);
}

//Mostra o ID de uma tarefa na tela (para debug)
#ifdef DEBUG

void task_print(void* task_v) {
    task_t *task = (task_t *) task_v;
    printf("<%d>", task->id);
}
#endif 

//Corpo de função da tarefa despachante

void dispatcher_body(void *arg) {

    despachante.status = EXECUTANDO; //Despachante em execução

    while (userTasks > 0) //Enquanto houver tarefas de usuários
    {
        task_t* next = scheduler(); //Próxima tarefa dada pelo escalonador
        if (next) {
#ifdef DEBUG
            printf("dispatcher_body: tarefa %d a ser executada\n", next->id);
            queue_print("Tarefas", (queue_t **) tarefas_prontas, task_print);
#endif //DEBUG
            if (task_set_executing(next)) //Muda estado da próxima tarefa para EXECUTANDO e retira-a da fila atual
            {
                char error[32];
                sprintf(error, "Erro ao mudar estado para EXECUTANDO.");
                perror(error);
                exit(-1);
            }
            despachante.status = PRONTA; //Preparando despachante para troca de tarefa
            task_switch(next); //Executa a próxima tarefa
            despachante.status = EXECUTANDO; //Ao voltar da última tarefa, despachante entra em execução
        } else if (!tarefas_prontas) {
            break;
        }

    }
    task_exit(0);
}

task_t *scheduler() {

    //Se a fila de tarefas prontas estiver vazia, retorne nulo
    if (!tarefas_prontas) {
        return NULL;
    }
    //FCFS- ṕrimeiro elemento da fila será o próximo a executar
    task_t * next = tarefas_prontas;
    //Prepara a próxima tarefa para a próxima execução
    tarefas_prontas = tarefas_prontas->next;
    return next;
}



//Função interna para ajudar a mudar o estado de uma tarefa e inseri-la na fila de prontas
//Retorna 0 caso ocorra tudo certo, -1 caso haja um erro

int task_set_ready(task_t* task) {

    task->status = PRONTA; //Preparado para execução
    //Se estiver inserido em uma fila, ...
    if (task->fila_atual) {
        queue_remove(task->fila_atual, (queue_t *) task); //... remove-lo desta fila e...
    }
    queue_append((queue_t **) & tarefas_prontas, (queue_t *) task); //... inseri-lo na fila de prontos, ...
    task->fila_atual = (queue_t **) & tarefas_prontas; //... atualizando sua nova fila em seguida.

    return 0;
}

//Função interna para ajudar a executar e remove-la da fila que está inserida
//Retorna 0 caso ocorra tudo certo, -1 caso haja um erro

int task_set_executing(task_t* task) {

    task->status = EXECUTANDO; //Em execução

    if (task->fila_atual) //Se estiver inserido em uma fila, ...
    {
        queue_remove(task->fila_atual, (queue_t *) task); //... remove-lo desta fila e...
        task->fila_atual = NULL; //... atualizar para fila nula em seguida.
    }

    return 0;
}
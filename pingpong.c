#include "pingpong.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>

#define STACKSIZE 32768		/* tamanho de pilha das threads */

///Variáveis globais    ========================================================
task_t tarefa_principal, dispatcher, *tarefa_atual = NULL, *fila_tprontas = NULL;     //Tarefa em execução

int userTasks = 0;      //Contador de tarefas de usuário ativas
int id_count = 0;       //Contador de IDs

///Funções P03 ============================================================
//Inicializa variáveis da tarefa principal
void init_tarefa_principal();

//Função despachante de tarefas (corpo associada à tarefa despachante)
void dispatcher_body(void *arg);

//Despachante de tarefas
task_t *scheduler();

//Altera o estado de uma tarefa para PRONTA e adicona na fila de tarefas prontas
int task_set_ready(task_t* task);

//Altera o estado de uma tarefa para EXECUTANDO e à retira da fila da qual pertence
int task_set_executing(task_t* task);


// funções gerais ==============================================================
// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void pingpong_init (){
    //desativa o buffer de saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);
    init_tarefa_principal();           //Inicializa tarefa principal (atual)
    task_create(&dispatcher, dispatcher_body, "dispatcher :");   //Inicializa despachante de tarefas
}

// gerência de tarefas =========================================================
// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg){
    //Sanity check
    if(!task){
        perror ("Tarefa não alocada corretamente: ");
        return -1;
    }

    task->status = NEW;                 //Tarefa criada, mas não inicializada

    //A tarefa criada não pertence a nenhuma fila (por enquanto)
    task->next = NULL;
    task->prev = NULL;
    task->fila_atual = NULL;

    getcontext (&(task->context));      //Incializa com contexto atual
    char *stack = malloc (STACKSIZE);   //Inicialização da pilha

    //Inicialização do contexto da tarefa
    if (stack){
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
        task->id = id_count++;         //Novo ID
        task->parent = tarefa_atual;    //Tarefa corrente é a criadora desta tarefa
    }
    else{
        char error[32];
        sprintf(error, "Erro na criação da pilha da tarefa %d", task->id);
        perror (error);
        exit(-1);
    }

    makecontext (&task->context, (void*)(*start_func), 1, arg);     //Associa o contexto à função passada por argumento

    //Caso seja uma tarefa de usuário (ID > 1)
    if(task->id > 1){
        userTasks++;                //Nova tarefa de usuário criada
        if(task_set_ready(task)){    //Tenta mudar seu estado para PRONTO e inserir na fila de prontos
        
            char error[32];
            sprintf(error, "Erro ao mudar estado da tarefa %d para PRONTA.", task->id);
            perror (error);
            exit(-1);
        }
    }
    else{
        task->status = READY;       //Apenas muda o estado, caso seja tarefa principal ou despachante
    }


    #ifdef DEBUG
    printf("task_create: criou a tarefa %d\n", task->id);
    #endif // DEBUG

    return task->id;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode){
    //Se a tarefa corrente for a principal, finaliza com código da saída
    if(tarefa_atual->id == 0){
        exit (exitCode);
    }

    task_t *last_task = tarefa_atual;   //Última tarefa em execução

    if(last_task->id == 1){             //Caso o despachante tente sair...
        tarefa_atual = &tarefa_principal;      //... a próxima tarefa será a principal, ...
    }
    else{
        tarefa_atual = &dispatcher;     //... caso uma tarefa de usuário tente sair, o despachante será o próximo a executar
    }

    userTasks--;                        //Menos uma tarefa em operação
    last_task->status = FINISHED;       //Tarefa atual será finalizada
    tarefa_atual->status = EXECUTING;   //Próxima tarefa entrará em execução

    #ifdef DEBUG
    printf("task_exit: tarefa %d sendo encerrado com codigo %d\n", last_task->id, exitCode);
    #endif // DEBUG

    //Efetua a troca de contexto da a última tarefa e a tarefa principal
    swapcontext(&last_task->context, &tarefa_atual->context);
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
    //Sanity Check
    if(!task){
        perror ("Tarefa não alocada corretamente: ");
        return -1;
    }

    task_t *last_task = tarefa_atual;   //Última tarefa executada
    tarefa_atual = task;                //Troca da tarefa antiga para a atual

    #ifdef DEBUG
    printf("task_switch: trocando contexto %d -> %d\n", last_task->id, tarefa_atual->id);
    #endif // DEBUG

    //Troca o contexto entre as tarefas passadas como parâmetro
    swapcontext(&last_task->context, &tarefa_atual->context);
    return 0;
}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id (){
    return tarefa_atual->id;
}

// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"; usa a tarefa atual se task==NULL
void task_suspend (task_t *task, task_t **queue){
    task_t * working_task;

    if(task){                            //Caso passado uma tarefa como parâmetro...
        working_task = task;            //... se trabalhará com ela, ...
    }
    else{
        working_task = tarefa_atual;    //... caso contrário, utilize a tarefa em execução.
    }

    working_task->status = SUSPENDED;   //Suspende a tarefa em trabalho

    if(queue){ //Se for passado uma fila como parâmetro...
    
        if(working_task->fila_atual){ //... verifique se a tarefa está conida em alguma fila, ...
            queue_remove(working_task->fila_atual, (queue_t *) working_task); //... se estiver, remova-a da fila atual e, ...
        }
        queue_append((queue_t **) queue,(queue_t *) working_task);  //... em seguida, adicione à fila passado por parâmetro, ...
        working_task->fila_atual = (queue_t **) queue;    //... atualizando para a nova fila em que se encontra.
    }

    #ifdef DEBUG
        printf("task_suspend: tarefa %d entrando em suspensão.\n", working_task->id);
    #endif  //DEBUG


    //Volta para o despachante, caso a tarefa seja a corrente
    if(!task){
        task_switch(&dispatcher);
    }
        
}

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume (task_t *task){
    
    task_set_ready(task);

    #ifdef DEBUG
    printf("task_resume: tarefa %d preparada para execução\n", task->id);
    #endif  //DEBUG
}

// operações de escalonamento ==================================================

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield (){
    
    #ifdef DEBUG
    printf("task_yield: liberando-se da tarefa %d\n", tarefa_atual->id);
    #endif  //DEBUG

    //Tarefa de usuário é sempre maior que 1
    if(tarefa_atual->id > 1) { //Caso seje uma tarefa de usuário...
   
        if(task_set_ready(tarefa_atual)){ //Insere a tarefa corrente na fila de prontas, mudando seu estado para PRONTO, ...        
            char error[32];
            sprintf(error, "Erro ao mudar estado da tarefa %d para PRONTA.", tarefa_atual->id);
            perror (error);
            exit(-1);
        }
    }
    else{
             tarefa_atual->status = READY;   //Caso contrário, apenas muda seu estado para PRONTO
    }  

    //Retorna para o despachante
    task_switch(&dispatcher);
}

//Mostra o ID de uma tarefa na tela (para debug)
#ifdef DEBUG
    void task_print(void* task_v){
        task_t *task = (task_t *) task_v;
        printf("<%d>", task->id);
    }
#endif //DEBUG

//Corpo de função da tarefa despachante
void dispatcher_body(void *arg){
    
    dispatcher.status = EXECUTING;  //Despachante em execução
    
    while(userTasks > 0) {           //Enquanto houver tarefas de usuários
    
        task_t* next = scheduler(); //Próxima tarefa dada pelo escalonador
        if(next){
            
            #ifdef DEBUG
                printf("dispatcher_body: tarefa %d a ser executada\n", next->id);
                queue_print("Tarefas",(queue_t **)fila_tprontas, task_print);
            #endif //DEBUG

            if(task_set_executing(next)){    //Muda estado da próxima tarefa para EXECUTANDO e retira-a da fila atual
            
                char error[32];
                sprintf(error, "Erro ao mudar estado da tarefa %d para EXECUTANDO.", next->id);
                perror(error);
                exit(-1);
            }
            dispatcher.status = READY;      //Preparando despachante para troca de tarefa
            task_switch(next);              //Executa a próxima tarefa
            dispatcher.status = EXECUTING;  //Ao voltar da última tarefa, despachante entra em execução
        }
        else if (!fila_tprontas){
            break;
        }
    }
    task_exit(0);
}


//Função do escalonador
task_t *scheduler(){
    
    //Se a fila de tarefas prontas estiver vazia, retorne nulo
    if(!fila_tprontas){
        return NULL;
    }
    
    //FCFS- ṕrimeiro elemento da fila será o próximo a executar
    task_t * next = fila_tprontas;
    //Prepara a próxima tarefa para a próxima execução
    fila_tprontas = fila_tprontas->next;
    
    return next;
}

//Inicializa tarefa principal
void init_tarefa_principal(){
    
    //A tarefa principal não pertence a nenhuma fila
    tarefa_principal.next = NULL;
    tarefa_principal.prev = NULL;
    tarefa_principal.fila_atual = NULL;

    tarefa_principal.id = id_count++;     //ID da tarefa principal
    tarefa_principal.parent = NULL;        //A primeira tarefa não possui pai,...
    tarefa_principal.status = EXECUTING;   //... já está em execução quando foi criada ...

    tarefa_atual = &tarefa_principal;      //... e é a tarefa em execução no momento.
}

//Função interna para ajudar a mudar o estado de uma tarefa e inseri-la na fila de prontas
//Retorna 0 caso ocorra tudo certo, -1 caso haja um erro
int task_set_ready(task_t* task){

        task->status = READY;       //Preparado para execução

        if(task->fila_atual){     //Se estiver inserido em uma fila, ...
            queue_remove(task->fila_atual, (queue_t *) task);   //... remove-lo desta fila e...
        }
        
        queue_append((queue_t **) &fila_tprontas, (queue_t *) task);     //... inseri-lo na fila de prontos, ...
        task->fila_atual = (queue_t **) &fila_tprontas;    //... atualizando sua nova fila em seguida.

        return 0;
}

//Função interna para ajudar a executar e remove-la da fila que está inserida
//Retorna 0 caso ocorra tudo certo, -1 caso haja um erro
int task_set_executing(task_t* task){
    
        task->status = EXECUTING; //Em execução

        if(task->fila_atual) { //Se estiver inserido em uma fila, ...       
            queue_remove(task->fila_atual, (queue_t *) task);   //... remove-lo desta fila e...
            task->fila_atual = NULL; //... atualizar para fila nula em seguida.
        }

        return 0;
}

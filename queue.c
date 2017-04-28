/*
 * Este sotfware foi feito para a UTFPR - Campus Curitiba;
 * O Código é livre para uso não comercial;
 * Desenvolvido através do Netbeans IDE.
 */

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>


//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append(queue_t **queue, queue_t *elem) {

    //Exceptions
    if (!queue) {
        printf("Fila está vazia.\n");
        return;
    }
    if (!elem) {
        printf("Inclusão de elemento vazio.\n");
        return;
    }
    if (elem->next || elem->prev) {
        printf("Este elemento já está em uma fila.\n");
        return;
    }

    if (!*queue) { //Caso a fila esteja vazia, adiciona elemento a fila
        *queue = elem;
        elem->next = elem;
        elem->prev = elem;
        return;
    }
    queue_t* primeiro = *queue;
    queue_t* ultimo = (*queue)->prev;

    primeiro->prev = elem; //o elemento passa a apontar para o inicio da fila
    ultimo->next = elem; //o elemento passa a apontar para o antigo final da fila 
    elem->next = primeiro;
    elem->prev = ultimo;




};

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove(queue_t **queue, queue_t *elem) {

    queue_t* aux = *queue;
    queue_t* fim = aux;
    queue_t* verso;
    queue_t* frente;

    //Exceptions
    if (!queue) {
        printf("Fila está vazia.\n");
        return NULL;
    }
    if (!elem) {
        printf("Inclusão de elemento vazio.\n");
        return NULL;
    }
    if (!*queue) {
        printf("Fila está nula.\n");
        return NULL;
    }
    //Remover elementos
    if (aux == elem) {

        if (aux->next != aux) {
            *queue = aux->next;
        } else {
            *queue = NULL;
        }

        verso = elem->prev;
        frente = elem->next;

        verso->next = frente;
        frente->prev = verso;

        elem->next = NULL;
        elem->prev = NULL;

        return elem;

    }

    for (aux = aux->next; aux != fim; aux = aux->next) {
        if (aux == elem) {
            verso = elem->prev;
            frente = elem->next;

            verso->next = frente;
            frente->prev = verso;

            elem->next = NULL;
            elem->prev = NULL;

            return elem;
        }
    }

    printf("Elemento não encontrado.\n");
    return NULL;

};

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size(queue_t *queue) {



    if (!queue) {
        return 0;
    }

    queue_t* aux = queue;
    queue_t* fim = aux->prev;
    int cont = 1;

    for (; aux != fim; aux = aux->next) {
        cont++;
    }
    return cont;

};

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print(char *name, queue_t *queue, void print_elem(void*)) {
    queue_t* aux = queue;
    queue_t* fim = aux;

    if (name) {
        printf("%s: ", name);
    }
    printf("[");

    if (queue) {
        print_elem((void *) aux);
        printf(" ");

        for (aux = aux->next; aux != fim; aux = aux->next) {
            print_elem((void *) aux);

            if (aux != fim->prev) {
                printf(" ");
            }
        }
    }

    printf("]\n");

};
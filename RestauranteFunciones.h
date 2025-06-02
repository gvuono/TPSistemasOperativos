#ifndef FUNCIONES_H
#define FUNCIONES_H

#include <semaphore.h>

#define MAX_PEDIDOS 15
#define MAX_COMBOS 8

typedef struct {
    int id;
    char estado[50];
    char combo[50];
} Pedido;

typedef struct {
    Pedido pedidos[MAX_PEDIDOS];
    int contador_pedidos;
    int finalizar;
} SharedData;

// Variables globales externas
extern SharedData *datos;
extern sem_t *sem_mutex;

void cargar_combo_aleatorio(char *dest);
void crear_pedido();
void avanzar_estado(const char *estado_actual, const char *estado_nuevo, const char *rol);
void cocinar();
void empaquetar();
void repartir();
void limpiar_recursos();

#endif


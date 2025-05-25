#ifndef FUNCIONES_H
#define FUNCIONES_H

#define MAX_PEDIDOS 10
#define MAX_COMBOS 3

typedef struct {
    int id;
    char estado[20];
    char combo[20];
} Pedido;

typedef struct {
    Pedido pedidos[MAX_PEDIDOS];
    int contador_pedidos;
    int finalizar;
} SharedData;

// Variables externas que se usarán
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

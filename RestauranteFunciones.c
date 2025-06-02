#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <string.h>  
#include <signal.h>
#include <time.h>


#include "funciones.h"

// Variables globales compartidas
SharedData *datos;
sem_t *sem_mutex;

char *combos[MAX_COMBOS] = {"Stacker", "BigMac", "Wopper","Mcnifica","Nuggets","Papas con cheddar","Cajita Feliz","McPollo"};

void cargar_combo_aleatorio(char *dest) {
    int r = rand() % MAX_COMBOS;
    strcpy(dest, combos[r]);
}

void crear_pedido() {
    while (!datos->finalizar) {
        sem_wait(sem_mutex);

        if (datos->contador_pedidos >= MAX_PEDIDOS) {
            sem_post(sem_mutex);
            break;
        }

        for (int i = 0; i < MAX_PEDIDOS; i++) {
            if (strcmp(datos->pedidos[i].estado, "") == 0) {
                datos->pedidos[i].id = i + 1;
                strcpy(datos->pedidos[i].estado, "Nuevo");
                cargar_combo_aleatorio(datos->pedidos[i].combo);
                datos->contador_pedidos++;
                printf("[TOMADOR PEDIDO] Pedido %d creado (%s)\n", datos->pedidos[i].id, datos->pedidos[i].combo);
                break;
            }
        }

        sem_post(sem_mutex);
        sleep(1);
    }
}

void avanzar_estado(const char *estado_actual, const char *estado_nuevo, const char *rol) {
    while (!datos->finalizar) {
        sem_wait(sem_mutex);
        int procesado = 0;
        for (int i = 0; i < MAX_PEDIDOS; i++) {
            if (strcmp(datos->pedidos[i].estado, estado_actual) == 0) {
                strcpy(datos->pedidos[i].estado, estado_nuevo);
                printf("[%s] Pedido %d %s\n", rol, datos->pedidos[i].id, estado_nuevo);
                procesado = 1;
                break;
            }
        }
        sem_post(sem_mutex);
        if (procesado)
            sleep(1);
        else
            usleep(100000);
    }
}

void cocinar() {
    avanzar_estado("Nuevo", "Cocinado", "COCINERO");
}

void empaquetar() {
    avanzar_estado("Cocinado", "Empaquetado", "EMPAQUETADOR");
}

void repartir() {
    avanzar_estado("Empaquetado", "Finalizado", "REPARTIDOR");
}

void limpiar_recursos() {
    sem_close(sem_mutex);
    sem_unlink("/sem_mutex");
    munmap(datos, sizeof(SharedData));
    shm_unlink("/mem_pedidos");
}

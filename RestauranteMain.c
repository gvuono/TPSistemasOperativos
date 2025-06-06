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

int main() {
    // Registro de señales
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGHUP, handle_signal);

    srand(time(NULL));

    int shm_fd = shm_open("/mem_pedidos", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData));
    datos = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    memset(datos, 0, sizeof(SharedData));

    sem_mutex = sem_open("/sem_mutex", O_CREAT, 0666, 1);

    pid_t pids[10];

    pids[0] = fork(); if (pids[0] == 0) { crear_pedido(); exit(0); }
    pids[1] = fork(); if (pids[1] == 0) { cocinar(); exit(0); }
    pids[2] = fork(); if (pids[2] == 0) { cocinar(); exit(0); }
    pids[3] = fork(); if (pids[3] == 0) { empaquetar(); exit(0); }
    pids[4] = fork(); if (pids[4] == 0) { repartir(); exit(0); }
    pids[5] = fork(); if (pids[5] == 0) { repartir(); exit(0); }
    pids[6] = fork(); if (pids[6] == 0) { repartir(); exit(0); }

    printf("[PADRE] Presione ENTER para terminar...\n");
    getchar();
    datos->finalizar = 1;

    for (int i = 0; i < 7; i++) waitpid(pids[i], NULL, 0);

    printf("[PADRE] Pedidos completados:\n");
    for (int i = 0; i < MAX_PEDIDOS; i++) {
        if (strcmp(datos->pedidos[i].estado, "Finalizado") == 0)
            printf("  Pedido %d (%s): %s\n", datos->pedidos[i].id, datos->pedidos[i].combo, datos->pedidos[i].estado);
    }

    limpiar_recursos();
    return 0;
}


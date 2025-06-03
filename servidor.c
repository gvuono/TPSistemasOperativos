#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <semaphore.h>  // 🔴 Agregado para semáforo

#include "funcionescliente.h"

#define PUERTO 8080
#define MAX_CLIENTES 100
#define BUFFER_SIZE 512

Cuenta cuentas[MAX_CLIENTES];
int totalCuentas = 0;
pthread_mutex_t mutexCuentas;
sem_t semaforoClientes;  // 🔴 Semáforo global para limitar clientes

typedef struct {
    int socket;
    struct sockaddr_in clienteAddr;
} ClienteArgs;

void enviarMensaje(int socket, const char* estado, const char* mensaje) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer),
             "{\"estado\":\"%s\",\"mensaje\":\"%s\"}", estado, mensaje);
    send(socket, buffer, strlen(buffer), 0);
}

void* manejarCliente(void* arg) {
    ClienteArgs* args = (ClienteArgs*)arg;
    int socketCliente = args->socket;
    free(arg);

    char buffer[BUFFER_SIZE];
    char cuentaActiva[20] = "";
    int cuentaIndex = -1;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(socketCliente, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;

        if (strstr(buffer, "\"operacion\":\"login\"")) {
            char cuenta[20];
            sscanf(buffer, "{\"operacion\":\"login\",\"cuenta\":\"%[^\"]\"}", cuenta);

            pthread_mutex_lock(&mutexCuentas);
            cuentaIndex = buscarCuenta(cuentas, totalCuentas, cuenta);

            if (cuentaIndex != -1 && cuentas[cuentaIndex].enUso == 0) {
                cuentas[cuentaIndex].enUso = 1;
                strcpy(cuentaActiva, cuenta);
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "ok", "Acceso concedido");
            } else {
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "error", "Cuenta inválida o en uso");
            }
        }

        else if (strstr(buffer, "\"operacion\":\"crear\"")) {
            char cuenta[20];
            float limite = 0.0;

             sscanf(buffer, "{\"operacion\":\"crear\",\"cuenta\":\"%[^\"]\",\"limite\":%f}", cuenta, &limite);

            pthread_mutex_lock(&mutexCuentas);
            int existe = buscarCuenta(cuentas, totalCuentas, cuenta);
            if (existe != -1) {
                pthread_mutex_unlock(&mutexCuentas);
                 enviarMensaje(socketCliente, "error", "Cuenta ya existe");
            } else {
        // Nueva cuenta saldo=0, enUso=0
            Cuenta nueva;
            strcpy(nueva.cuenta, cuenta);
            nueva.saldo = 0.0;
            nueva.limite = limite;
            nueva.enUso = 0;

            if (totalCuentas < MAX_CLIENTES) {
                cuentas[totalCuentas++] = nueva;
                agregarCuentaEnArchivo(nueva);
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "ok", "Cuenta creada");
            } else {
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "error", "Límite de cuentas alcanzado");
            }
            }
        }


        else if (strstr(buffer, "\"operacion\":\"consultar\"")) {
            pthread_mutex_lock(&mutexCuentas);
            float saldo = cuentas[cuentaIndex].saldo;
            float limite = cuentas[cuentaIndex].limite;
            pthread_mutex_unlock(&mutexCuentas);

            char msg[128];
            snprintf(msg, sizeof(msg),
                     "Saldo: $%.2f - Límite: $%.2f", saldo, limite);
            enviarMensaje(socketCliente, "ok", msg);
        }

        else if (strstr(buffer, "\"operacion\":\"depositar\"")) {
          float monto;
            float limite = cuentas[cuentaIndex].limite;
            sscanf(buffer, "{\"operacion\":\"depositar\",\"monto\":%f}", &monto);
            if (monto > limite){
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "error", "Monto excede limite");
            }
            else
            {
            pthread_mutex_lock(&mutexCuentas);
            cuentas[cuentaIndex].saldo += monto;
            actualizarCuentaEnArchivo(cuentas[cuentaIndex]);
            pthread_mutex_unlock(&mutexCuentas);
 
            enviarMensaje(socketCliente, "ok", "Depósito realizado");
            }
        }

        else if (strstr(buffer, "\"operacion\":\"retirar\"")) {
            float monto;
            sscanf(buffer, "{\"operacion\":\"retirar\",\"monto\":%f}", &monto);

            pthread_mutex_lock(&mutexCuentas);
            if (cuentas[cuentaIndex].saldo >= monto) {
                cuentas[cuentaIndex].saldo -= monto;
                actualizarCuentaEnArchivo(cuentas[cuentaIndex]);
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "ok", "Retiro exitoso");
            } else {
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "error", "Saldo insuficiente");
            }
        }

        else if (strstr(buffer, "\"operacion\":\"transferir\"")) {
            char destino[20];
            float monto;
            sscanf(buffer,
                   "{\"operacion\":\"transferir\",\"destino\":\"%[^\"]\",\"monto\":%f}",
                   destino, &monto);

            pthread_mutex_lock(&mutexCuentas);
            int idxDestino = buscarCuenta(cuentas, totalCuentas, destino);

            if (idxDestino == -1) {
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "error", "Cuenta destino no existe");
            } else if (cuentas[cuentaIndex].saldo < monto) {
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "error", "Saldo insuficiente");
            } else {
                cuentas[cuentaIndex].saldo -= monto;
                cuentas[idxDestino].saldo += monto;
                actualizarCuentaEnArchivo(cuentas[cuentaIndex]);
                actualizarCuentaEnArchivo(cuentas[idxDestino]);
                pthread_mutex_unlock(&mutexCuentas);
                enviarMensaje(socketCliente, "ok", "Transferencia exitosa");
            }
        }
        
        else if (strstr(buffer, "\"operacion\":\"cerrar\"")) {
            pthread_mutex_lock(&mutexCuentas);
            cuentas[cuentaIndex].enUso = 0;
            actualizarCuentaEnArchivo(cuentas[cuentaIndex]);
            pthread_mutex_unlock(&mutexCuentas);
            enviarMensaje(socketCliente, "ok", "Sesión cerrada");
            break;
        }

        else {
            enviarMensaje(socketCliente, "error", "Operación inválida");
        }
    }

    close(socketCliente);
    printf("Cliente desconectado\n");
    sem_post(&semaforoClientes);  // 🟢 Libera el lugar para otro cliente
    pthread_exit(NULL);
}

int main() {
    int servidorSocket, clienteSocket;
    struct sockaddr_in servidorAddr, clienteAddr;
    socklen_t clienteLen = sizeof(clienteAddr);

    signal(SIGPIPE, SIG_IGN);
    pthread_mutex_init(&mutexCuentas, NULL);
    sem_init(&semaforoClientes, 0, 5);  // 🔴 Inicializa semáforo con 5 clientes permitidos

    totalCuentas = cargarCuentas(cuentas, MAX_CLIENTES);
    printf("Cuentas cargadas: %d\n", totalCuentas);

    servidorSocket = socket(AF_INET, SOCK_STREAM, 0);
    servidorAddr.sin_family = AF_INET;
    servidorAddr.sin_port = htons(PUERTO);
    inet_pton(AF_INET, "192.168.165.251", &servidorAddr.sin_addr);


    if (bind(servidorSocket, (struct sockaddr*)&servidorAddr, sizeof(servidorAddr)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    if (listen(servidorSocket, 5) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en 192.168.165.251:%d...\n", PUERTO);

    while (1) {
        sem_wait(&semaforoClientes);

        clienteSocket = accept(servidorSocket,
                               (struct sockaddr*)&clienteAddr, &clienteLen);
        if (clienteSocket < 0) {
            perror("Error en accept");
            sem_post(&semaforoClientes);
            continue;
        }

        printf("Cliente conectado\n");

        ClienteArgs* args = malloc(sizeof(ClienteArgs));
        args->socket = clienteSocket;
        args->clienteAddr = clienteAddr;

        pthread_t hilo;
        pthread_create(&hilo, NULL, manejarCliente, args);
        pthread_detach(hilo);
    }

    close(servidorSocket);
    pthread_mutex_destroy(&mutexCuentas);
    sem_destroy(&semaforoClientes);
    return 0;
}

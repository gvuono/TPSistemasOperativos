#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

void enviarMensaje(int sock, const char *mensaje) {
    send(sock, mensaje, strlen(mensaje), 0);
}

int recibirMensaje(int sock, char *buffer, int size) {
    int bytes = recv(sock, buffer, size - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
    }
    return bytes;
}

void extraerCampoJSON(const char* json, const char* campo, char* valor, int maxlen) {
    char* pos = strstr(json, campo);
    if (!pos) {
        strncpy(valor, "NO_ENCONTRADO", maxlen);
        return;
    }
    pos = strchr(pos, ':');
    if (!pos) {
        strncpy(valor, "NO_ENCONTRADO", maxlen);
        return;
    }
    pos++; // Saltar el ':'
    while (*pos == ' ' || *pos == '"') pos++;

    int i = 0;
    while (*pos && *pos != '"' && *pos != ',' && *pos != '}' && i < maxlen - 1) {
        valor[i++] = *pos++;
    }
    valor[i] = '\0';
}

void mostrarMenu() {
    printf("\nOperaciones disponibles:\n");
    printf("1. Consultar saldo\n");
    printf("2. Depositar\n");
    printf("3. Retirar\n");
    printf("4. Transferir\n");
    printf("5. Cerrar sesión\n");
    printf("Seleccione opción: ");
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char cuenta[20];
    char opcion[10], monto[20], destino[20];
    char estado[20], mensaje[100];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect");
        return 1;
    }

    // LOGIN
    int logueado = 0, intentos = 0;
    while (intentos < 2 && !logueado) {
        printf("Ingrese número de cuenta para login: ");
        fgets(cuenta, sizeof(cuenta), stdin);
        cuenta[strcspn(cuenta, "\n")] = 0;

        snprintf(buffer, sizeof(buffer), "{\"operacion\":\"login\",\"cuenta\":\"%s\"}", cuenta);
        enviarMensaje(sock, buffer);

        int recibido = recibirMensaje(sock, buffer, sizeof(buffer));
        if (recibido <= 0) {
            printf("Servidor no responde o se desconectó.\n");
            close(sock);
            return 1;
        }

        extraerCampoJSON(buffer, "estado", estado, sizeof(estado));
        extraerCampoJSON(buffer, "mensaje", mensaje, sizeof(mensaje));

        if (strcmp(estado, "ok") == 0) {
            printf(">> %s\n", mensaje);
            logueado = 1;
        } else {
            printf(">> Error: %s\n", mensaje);
            intentos++;
        }
    }

    if (!logueado) {
        printf("Demasiados intentos fallidos.\n");
        close(sock);
        return 1;
    }

    // OPERACIONES
    while (1) {
        mostrarMenu();
        fgets(opcion, sizeof(opcion), stdin);
        opcion[strcspn(opcion, "\n")] = 0;

        if (strcmp(opcion, "1") == 0) {
            snprintf(buffer, sizeof(buffer), "{\"operacion\":\"consultar\"}");
        } else if (strcmp(opcion, "2") == 0) {
            printf("Monto a depositar: ");
            fgets(monto, sizeof(monto), stdin);
            monto[strcspn(monto, "\n")] = 0;
            snprintf(buffer, sizeof(buffer), "{\"operacion\":\"depositar\",\"monto\":%s}", monto);
        } else if (strcmp(opcion, "3") == 0) {
            printf("Monto a retirar: ");
            fgets(monto, sizeof(monto), stdin);
            monto[strcspn(monto, "\n")] = 0;
            snprintf(buffer, sizeof(buffer), "{\"operacion\":\"retirar\",\"monto\":%s}", monto);
        } else if (strcmp(opcion, "4") == 0) {
            printf("Cuenta destino: ");
            fgets(destino, sizeof(destino), stdin);
            destino[strcspn(destino, "\n")] = 0;
            printf("Monto a transferir: ");
            fgets(monto, sizeof(monto), stdin);
            monto[strcspn(monto, "\n")] = 0;
            snprintf(buffer, sizeof(buffer),
                     "{\"operacion\":\"transferir\",\"destino\":\"%s\",\"monto\":%s}",
                     destino, monto);
        } else if (strcmp(opcion, "5") == 0) {
            snprintf(buffer, sizeof(buffer), "{\"operacion\":\"cerrar\"}");
            enviarMensaje(sock, buffer);
            recibirMensaje(sock, buffer, sizeof(buffer));
            extraerCampoJSON(buffer, "mensaje", mensaje, sizeof(mensaje));
            printf(">> %s\n", mensaje);
            break;
        } else {
            printf("Opción inválida.\n");
            continue;
        }

        enviarMensaje(sock, buffer);
        recibirMensaje(sock, buffer, sizeof(buffer));
        extraerCampoJSON(buffer, "estado", estado, sizeof(estado));
        extraerCampoJSON(buffer, "mensaje", mensaje, sizeof(mensaje));
        printf(">> %s\n", mensaje);
    }

    close(sock);
    return 0;
}

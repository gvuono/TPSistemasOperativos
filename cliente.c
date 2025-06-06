#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void limpiarBufferEntrada() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

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

// Extrae el valor del campo "estado" o "mensaje" del JSON simple
void extraerValor(const char *json, const char *campo, char *valor, int maxlen) {
    char *pos = strstr(json, campo);
    if (!pos) {
        valor[0] = '\0';
        return;
    }
    pos = strchr(pos, ':');
    if (!pos) {
        valor[0] = '\0';
        return;
    }
    pos++; 
    while (*pos == ' ' || *pos == '"' || *pos == '{' || *pos == ',') pos++;
    int i = 0;
    while (*pos && *pos != '"' && *pos != ',' && *pos != '}' && i < maxlen - 1) {
        valor[i++] = *pos++;
    }
    valor[i] = '\0';
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char cuenta[20];
    char opcion[10];
    char estado[20], mensaje[100];
    char ip_str[50];
    int puerto;

    // Pedir IP y puerto al usuario
    printf("Ingrese IP del servidor: ");
    fgets(ip_str, sizeof(ip_str), stdin);
    ip_str[strcspn(ip_str, "\n")] = 0;

    printf("Ingrese puerto del servidor: ");
    scanf("%d", &puerto);
    limpiarBufferEntrada();

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(puerto);

    if(inet_pton(AF_INET, ip_str, &server_addr.sin_addr) <= 0) {
        printf("Direccion IP invalida\n");
        close(sock);
        return 1;
    };

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect");
        close(sock);
        return 1;
    }

    int intentos = 0;
    int logueado = 0;

    while (intentos < 2 && !logueado) {
        printf("Ingrese número de cuenta para login: ");
        fgets(cuenta, sizeof(cuenta), stdin);
        cuenta[strcspn(cuenta, "\n")] = 0;

        // Enviar login
        snprintf(buffer, sizeof(buffer),
            "{\"operacion\":\"login\",\"cuenta\":\"%s\"}", cuenta);
        enviarMensaje(sock, buffer);

        int recibido = recibirMensaje(sock, buffer, sizeof(buffer));
        if (recibido <= 0) {
            printf("Servidor no responde o se desconectó.\n");
            close(sock);
            return 1;
        }

        extraerValor(buffer, "\"estado\"", estado, sizeof(estado));
        extraerValor(buffer, "\"mensaje\"", mensaje, sizeof(mensaje));

        printf("Servidor: %s\n", mensaje);

        if (strcmp(estado, "ok") == 0) {
            logueado = 1;
            break;
        } else {
            intentos++;
            if (intentos == 2) {
                printf("Intentos agotados. Saliendo.\n");
                close(sock);
                return 0;
            }

 
            printf("¿Cuenta no encontrada. Desea crearla? (s/n): ");
            fgets(opcion, sizeof(opcion), stdin);
            if (opcion[0] == 's' || opcion[0] == 'S') {
                float limite;
                printf("Ingrese límite para la nueva cuenta: ");
                scanf("%f", &limite);
                while (getchar() != '\n'); 

                snprintf(buffer, sizeof(buffer),
                    "{\"operacion\":\"crear\",\"cuenta\":\"%s\",\"limite\":%.2f}",
                    cuenta, limite);
                enviarMensaje(sock, buffer);

                recibido = recibirMensaje(sock, buffer, sizeof(buffer));
                if (recibido <= 0) {
                    printf("Servidor no responde o se desconectó.\n");
                    close(sock);
                    return 1;
                }

                extraerValor(buffer, "\"estado\"", estado, sizeof(estado));
                extraerValor(buffer, "\"mensaje\"", mensaje, sizeof(mensaje));
                printf("Servidor: %s\n", mensaje);

                if (strcmp(estado, "ok") == 0) {
                    printf("Cuenta creada exitosamente. Por favor inicie sesión.\n");
                    break;
                } else {
                    printf("No se pudo crear la cuenta. Intente más tarde.\n");
                    close(sock);
                    return 0;
                }
            }
        }
    }

    // Menú
    while (logueado) {
        printf("\n--- Menú ---\n");
        printf("1) Consultar saldo\n");
        printf("2) Depósito\n");
        printf("3) Retiro\n");
        printf("4) Transferencia\n");
        printf("5) Cerrar sesión\n");
        printf("Elija opción: ");
        fgets(opcion, sizeof(opcion), stdin);

        
        if (opcion[0] == '1') {
            snprintf(buffer, sizeof(buffer),
                "{\"operacion\":\"consultar\",\"cuenta\":\"%s\"}", cuenta);
            enviarMensaje(sock, buffer);

            if (recibirMensaje(sock, buffer, sizeof(buffer)) > 0) {
                extraerValor(buffer, "\"estado\"", estado, sizeof(estado));
                extraerValor(buffer, "\"mensaje\"", mensaje, sizeof(mensaje));
                printf("Servidor: %s\n", mensaje);
            }
        }
        else if (opcion[0] == '2') {
            float monto;
            printf("Ingrese monto a depositar: ");
            scanf("%f", &monto);
	    limpiarBufferEntrada();
            snprintf(buffer, sizeof(buffer),
                "{\"operacion\":\"depositar\",\"monto\":%.2f}", monto);
            enviarMensaje(sock, buffer);

            if (recibirMensaje(sock, buffer, sizeof(buffer)) > 0) {
                extraerValor(buffer, "\"estado\"", estado, sizeof(estado));
                extraerValor(buffer, "\"mensaje\"", mensaje, sizeof(mensaje));
                printf("Servidor: %s\n", mensaje);
            }
        }
        else if (opcion[0] == '3') {
            float monto;
            printf("Ingrese monto a retirar: ");
            scanf("%f", &monto);
		limpiarBufferEntrada();
            snprintf(buffer, sizeof(buffer),
                "{\"operacion\":\"retirar\",\"monto\":%.2f}", monto);
            enviarMensaje(sock, buffer);

            if (recibirMensaje(sock, buffer, sizeof(buffer)) > 0) {
                extraerValor(buffer, "\"estado\"", estado, sizeof(estado));
                extraerValor(buffer, "\"mensaje\"", mensaje, sizeof(mensaje));
                printf("Servidor: %s\n", mensaje);
            }
        }
        else if (opcion[0] == '4') {
            char destino[20];
            float monto;
            printf("Ingrese cuenta destino: ");
            scanf("%19s", destino);
		limpiarBufferEntrada();

            printf("Ingrese monto a transferir: ");
            scanf("%f", &monto);
		limpiarBufferEntrada();


            snprintf(buffer, sizeof(buffer),
                "{\"operacion\":\"transferir\",\"destino\":\"%s\",\"monto\":%.2f}", destino, monto);
            enviarMensaje(sock, buffer);

            if (recibirMensaje(sock, buffer, sizeof(buffer)) > 0) {
                extraerValor(buffer, "\"estado\"", estado, sizeof(estado));
                extraerValor(buffer, "\"mensaje\"", mensaje, sizeof(mensaje));
                printf("Servidor: %s\n", mensaje);
            }
        }
        else if (opcion[0] == '5') {
            snprintf(buffer, sizeof(buffer),
                "{\"operacion\":\"cerrar\"}");
            enviarMensaje(sock, buffer);

            if (recibirMensaje(sock, buffer, sizeof(buffer)) > 0) {
                extraerValor(buffer, "\"estado\"", estado, sizeof(estado));
                extraerValor(buffer, "\"mensaje\"", mensaje, sizeof(mensaje));
                printf("Servidor: %s\n", mensaje);
            }
            break;
        }
        else {
            printf("Opción inválida\n");
        }
    }

    close(sock);
    printf("Sesión finalizada.\n");
    return 0;
}

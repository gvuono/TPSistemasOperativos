#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "funcionescliente.h"

#define ARCHIVO "clientes.txt"

int cargarCuentas(Cuenta cuentas[], int max) {
    FILE *file = fopen(ARCHIVO, "r");
    if (!file) return 0;

    int i = 0;
    while (i < max && fscanf(file, "%[^,],%f,%f,%d\n",
                           cuentas[i].cuenta,
                           &cuentas[i].saldo,
                           &cuentas[i].limite,
                           &cuentas[i].enUso) == 4) {
        i++;
    }
    fclose(file);
    return i;
}

int buscarCuenta(Cuenta cuentas[], int total, const char* cuenta) {
    for (int i = 0; i < total; i++) {
        if (strcmp(cuentas[i].cuenta, cuenta) == 0) {
            return i;
        }
    }
    return -1;
}

void actualizarCuentaEnArchivo(Cuenta cuenta) {
    // Reescribe TODO el archivo con la cuenta actualizada
    Cuenta cuentas[100];
    int total = cargarCuentas(cuentas, 100);

    for (int i = 0; i < total; i++) {
        if (strcmp(cuentas[i].cuenta, cuenta.cuenta) == 0) {
            cuentas[i] = cuenta;
            break;
        }
    }

    FILE *file = fopen(ARCHIVO, "w");
    if (!file) return;

    for (int i = 0; i < total; i++) {
        fprintf(file, "%s,%.2f,%.2f,%d\n",
                cuentas[i].cuenta,
                cuentas[i].saldo,
                cuentas[i].limite,
                cuentas[i].enUso);
    }
    fclose(file);
}

void agregarCuentaEnArchivo(Cuenta cuenta) {
    FILE *file = fopen(ARCHIVO, "a");
    if (!file) return;
    fprintf(file, "%s,%.2f,%.2f,%d\n",
            cuenta.cuenta,
            cuenta.saldo,
            cuenta.limite,
            cuenta.enUso);
    fclose(file);
}

#ifndef FUNCIONES_H
#define FUNCIONES_H

typedef struct {
    char cuenta[20];
    float saldo;
    float limite;
    int enUso;
} Cuenta;

int cargarCuentas(Cuenta cuentas[], int max);
int buscarCuenta(Cuenta cuentas[], int total, const char* cuenta);
void actualizarCuentaEnArchivo(Cuenta cuenta);
void agregarCuentaEnArchivo(Cuenta cuenta);

#endif

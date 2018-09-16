/*
 *  usuario/include/servicios.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene los prototipos de funciones de
 * biblioteca que proporcionan la interfaz de llamadas al sistema.
 *
 *      SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef SERVICIOS_H
#define SERVICIOS_H

/* Evita el uso del printf de la bilioteca estándar */
#define printf escribirf

//Mutex
#define NO_RECURSIVO 0
#define RECURSIVO 1

/* Funcion de biblioteca */
int escribirf(const char *formato, ...);

/* Llamadas al sistema proporcionadas */
int crear_proceso(char *prog);
int terminar_proceso();
int escribir(char *texto, unsigned int longi);

// LLAMADAS AL SISTEMA ANADIDAS
//1.5) Prototipo llamada al sistema Obtener ID
int obtener_id_pr ();
//2.3) Prototipo llamada al sistema Dormir
int dormir (unsigned int segundos);
// MUTEX
int crear_mutex (char *nombre, int tipo);
int abrir_mutex(char *nombre);
int lock (unsigned int mutexid);
int unlock (unsigned int mutexid);
int cerrar_mutex (unsigned int mutexid);
//5.4) Leer_caracter
int leer_caracter ();

#endif /* SERVICIOS_H */


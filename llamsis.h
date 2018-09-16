/*
 *  minikernel/kernel/include/llamsis.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene el numero asociado a cada llamada
 *
 * 	SE DEBE MODIFICAR PARA INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef _LLAMSIS_H
#define _LLAMSIS_H

/* Numero de llamadas disponibles */
#define NSERVICIOS 11 //Aumentar cada vez que se a�ade llamada 

#define CREAR_PROCESO 0
#define TERMINAR_PROCESO 1
#define ESCRIBIR 2
#define OBTENER_ID_PR 3 //1.3) Numero llamada Obtener ID
#define DORMIR 4 //2.3) Numero llamada Dormir 
//MUTEX
#define CREAR_MUTEX 5
#define ABRIR_MUTEX 6
#define LOCK 7
#define UNLOCK 8
#define CERRAR_MUTEX 9
#define LEER_CARACTER 10

#endif /* _LLAMSIS_H */


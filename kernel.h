/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H



#include "const.h"
#include "HAL.h"
#include "llamsis.h"
#include <string.h>

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
        int id;				/* ident. del proceso */
        int estado;			/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
        contexto_t contexto_regs;	/* copia de regs. de UCP */
        void * pila;			/* dir. inicial de la pila */
		int t_dormir; // 2.1) AÑADIDA. Guarda el tiempo que debe dormir el proceso
		int n_descriptores; //3) AÑADIDA. Guarda el no. de descriptores abiertos del proceso
		int descriptores[NUM_MUT_PROC]; //3) AÑADIDA. Array de descriptores
		//4) Variables round robin
		unsigned int slice; /*Tiempo de ejecucion que le queda al proceso(Rodaja) !!!!!!!!*/
		BCPptr siguiente;		/* puntero a otro BCP */
		void *info_mem;			/* descriptor del mapa de memoria */
		
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en semáforo, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos= {NULL, NULL};

// 2.2) AÑADIDA. Cola de procesos dormidos/bloqueados
lista_BCPs lista_dormidos = {NULL, NULL};




/*
 *
 * Definición del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;

//3) MUTEX
#define NO_RECURSIVO 0
#define RECURSIVO 1

//Estructura para el tipo mutex
typedef struct {
	char *nombre;
	int tipo; //Recursivo o no recursivo
	int propietario; //Id del proceso que ha hecho lock
	int abierto; //0 si no esta abierto (esta disponible)
	int locked; //Guarda numero de veces que ha sido bloqueado (0 si esta desbloqueado)	
	//Lista propia de cada mutex. Guarda procesos bloqueados porque el mutex ya estaba bloqueado
	lista_BCPs lista_proc_esperando_lock;
} mutex;
//Array para guardar mutex utilizados. Tamaño = numero maximo de mutex
mutex array_mutex[NUM_MUT];
//Variable que almacena el numero de mutex creados
int mutex_creados;
//Lista de procesos bloqueados porque se habian creado el numero maximo de mutex permitidos
lista_BCPs lista_bloq_mutex = {NULL, NULL};

//5) LEER CARACTER POR TECLADO
int buffer[TAM_BUF_TERM]; //5.1) Buffer de caracteres (int), tamaño = 8
int car_leidos; //5.1) Guarda el numero de cracateres leidos para detectar buffer lleno
lista_BCPs lista_bloq_buffer = {NULL, NULL}; //5.1) Cola procesos bloqueados por buffer vacio

/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int obtener_id_pr(); //1.2) Prototipo llamada al sistema Obtener Id
int dormir(unsigned int segundos); //2.3) Prototipo llamada al sistema Dormir
// MUTEX
int crear_mutex (char *nombre, int tipo);
int abrir_mutex(char *nombre);
int lock (unsigned int mutexid);
int unlock (unsigned int mutexid);
int cerrar_mutex (unsigned int mutexid);
// 5.4) Anadir llamada leer_caracter
int leer_caracter ();
/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{obtener_id_pr}, //1.2) Rutina llamada al sistema Obtener ID
					{dormir}, //2.3) Rutina llamada al sistema Dormir
					{crear_mutex},
					{abrir_mutex},
					{lock},
					{unlock},
					{cerrar_mutex},
					{leer_caracter}
				};

#endif /* _KERNEL_H */


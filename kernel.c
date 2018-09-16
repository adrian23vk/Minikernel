// ALUMNOS:
// Adrian Morillas Marcos
// Marta Fernandez de la Mela Salcedo


/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */
#include <string.h>


/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Función que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Función que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	//printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al mínimo el nivel de interrupción mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Función de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	return lista_listos.primero;
}
//4) ROUND ROBIN
/*
 * Funcion que realiza un cambio de proceso
 */
static void change_procces(lista_BCPs *lista_destino) { 
	BCP * p_proc_anterior;
	contexto_t *contexto_aux;
	int level;

	p_proc_anterior = p_proc_actual; //Salvaguardar proc actual
	level=fijar_nivel_int(NIVEL_3); //Inhibir interrupciones reloj al manipular listas
	
	eliminar_primero(&lista_listos);

	//Si se le ha pasado alguna lista como argumento
	if (lista_destino) {
		insertar_ultimo(lista_destino, p_proc_anterior);
	}

	p_proc_actual = planificador(); //Obtener nuevo proc actual
	
	p_proc_actual->slice = TICKS_POR_RODAJA; //Inicializar rodaja
	
	//Si el proceso ha terminado, se libera su pila
	if (p_proc_anterior->estado == TERMINADO) {
		contexto_aux = NULL;
		liberar_pila(p_proc_anterior->pila);
	}
	else {
		//Guardar el contexto para realizar el cambio
		contexto_aux=&(p_proc_anterior->contexto_regs);
	}
	
	printk("CAMBIO CONTEXTO DE %d A %d\n", p_proc_anterior->id, p_proc_actual->id);
	cambio_contexto(contexto_aux, &(p_proc_actual->contexto_regs));

	fijar_nivel_int(level); //Recuperar nivel int original
}



/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
// 4) Modificada para Round Robin
static void liberar_proceso(){
	BCP * p_proc_anterior;

	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;

	//4) Comentado para el Round Robin
	//eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	//p_proc_anterior=p_proc_actual;
	//p_proc_actual=planificador();
	/*
	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));*/

	//4) Cambia el proceso pero sin guardar el estado actual
	printk("Liberado el proceso %d, se va a cambiar de proceso\n", p_proc_actual->id);
	change_procces(NULL);

    return; /* no debería llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
//5.2) Modificado para leer caracter tecleado y guardar en el buffer
static void int_terminal(){
	
	char car;
	BCP* proceso = lista_bloq_buffer.primero;

	car = leer_puerto(DIR_TERMINAL); //Lee en el puerto el caracter introducido
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);

	int nivel_int1 = fijar_nivel_int(NIVEL_2); //Nivel 2 para que no salten int_terminal mientras se escribe
	
	//Si el buffer esta lleno, muestra error e ignora el caracter
	if(car_leidos == TAM_BUF_TERM) {
		printk("El buffer esta lleno. Se ignora caracter introducido\n");
	}
	//Si no esta lleno Y hay algun proceso esperando datos, se mete el caracter en el buffer 
	else if (proceso != NULL) {
		
		buffer[car_leidos] = (int) car;
		//Comprobar que se ha insertado correctamente
		printk("Se ha insertado el caracter i=%d, c=%c\n", buffer[car_leidos], (char)buffer[car_leidos]);
		car_leidos++;
	}

	
	//Comprobar si habia algun proceso bloqueado y despertarlo
	if(proceso != NULL) {
		printk("Habia un proceso bloqueado y se va a desbloquear\n");
		
		int nivel_int = fijar_nivel_int(NIVEL_3);
		//Cambiar el estado
		proceso->estado = LISTO;
		//Quitarlo de la lista dormidos
		eliminar_elem(&lista_bloq_buffer, proceso);
		//Ponerlo el ultimo en la cola de listos
		insertar_ultimo(&lista_listos, proceso);
		fijar_nivel_int(nivel_int);
		printk("Se ha desbloqueado el proceso con id=%d\n", proceso->id);
	}
	else {
		printk("No hay procesos esperando datos\n");
	}

	fijar_nivel_int(nivel_int1);

    return;
}

//4) Round Robin
/*
 * Función auxiliar que actualiza la rodaja y detecta si esta ha terminado
 */
static void actualizar_rodaja() {
	/* Si el proceso no está listo para ejecutar, no se actualiza la
	   rodaja. Está haciendo las labores de proceso nulo */
	if (p_proc_actual->estado == LISTO) {
		p_proc_actual->slice--;
		printk("Al proceso le quedan %d de rodaja\n", p_proc_actual->slice);
		if (p_proc_actual->slice==0) {
			printk("Se va a activar la int sw\n");
			activar_int_SW(); //Funcion dada que activa una int sw cuando la rodaja es 0
		}
	}
}


/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){

	printk("-> TRATANDO INT. DE RELOJ\n");

	//4) Actualizar rodaja round robin
	actualizar_rodaja();

	//2.4) Modificar la rutina para detectar si se cumple el plazo de algun proceso dormido
	//Cada vez que salta una interrupcion hay que recorrer la cola para disminuir el tiempo
	// de dormir que le queda a cada proceso. Posteriormente se comprueba si es 0
	BCP* proceso = lista_dormidos.primero;
		
	while (proceso != NULL) {
		//Disminuir su tiempo
		proceso->t_dormir--;
		printk("Al proceso con id = %d le queda %d\n", proceso-> id, proceso->t_dormir);
		//Si el plazo ha acabado, desbloquear
		//Antes de borrarlo hay que guardar el siguiente proceso al que apunta en la cola dormidos
		//Si se borra despues, apunta a otro que no esta dormido!!
		BCP* proceso_siguiente = proceso->siguiente;
		if(proceso->t_dormir <= 0) {
			//Elevar nivel interrupcion y guardar actual (desconocido)
			int nivel_int = fijar_nivel_int(NIVEL_3); //Inhibir int reloj mientras se manejan listas
			
			printk("El proceso con id = %d despierta\n", proceso->id);
			//Cambiar el estado
			proceso->estado = LISTO;
			//Quitarlo de la lista dormidos
			eliminar_elem(&lista_dormidos, proceso);
			//Ponerlo el ultimo en la cola de listos
			insertar_ultimo(&lista_listos, proceso);

			//Volver al nivel de interrupcion anterior
			fijar_nivel_int(nivel_int);		
		}
		//Pasar al siguiente elemento en la cola de dormidos
		proceso = proceso_siguiente;
	}
		
    return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
//4) Modificada para el ROUND ROBIN
static void int_sw(){

	printk("-> TRATANDO INT. SW\n");

	//Llamar a la funcion de cambio de procesos y contextos
	change_procces(&lista_listos);

	return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	//printk("Creando tarea\n");
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

			
	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	//printk("Imagen creada: %d\n", imagen);
	if (imagen)
	{	
		
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;

		//4) ROUND ROBIN: Inicializar rodaja
		p_proc->slice = TICKS_POR_RODAJA;

		//Rellenar con -1
		for(int i=0; i<NUM_MUT_PROC; i++){
			p_proc->descriptores[i] = -1;
		}
		
		/* lo inserta al final de cola de listos */
		int level = fijar_nivel_int(NIVEL_3);
		insertar_ultimo(&lista_listos, p_proc);
		fijar_nivel_int(level);
		error= 0;
	}
	else {
		error= -1; /* fallo al crear imagen */
	}
	
	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	//3) Antes de liberar proceso hay que cerrar todos sus mutex
	if (p_proc_actual->n_descriptores > 0) {
		printk("Cerrar mutex que ha abierto el proceso\n");
		for(int i = 0; i < NUM_MUT_PROC; i++) {
			printk("%d\n", i);
			cerrar_mutex(i);			
		}
		printk("se ha terminado de cerrar los mutex\n");
	}

	liberar_proceso();

        return 0; /* no debería llegar aqui */
}

/*** LLAMADAS  AÑADIDAS ***/

//1.1) Llamada que devuelve el identificador del proceso que la invoca
int obtener_id_pr () {

	int id = p_proc_actual->id;
	printk("Id del proceso actual: %d\n", id);
	return id;
}

// 2.3) Llamada que permite que un proceso se quede bloqueado un tiempo
int dormir (unsigned int segundos) {
	segundos=(unsigned int)leer_registro(1); //Obtener segundos que va a dormir

	//Elevar nivel interrupcion y guardar actual (desconocido)
	int nivel_int = fijar_nivel_int(NIVEL_3);
	printk("El proceso con id = %d va a dormir %d segundos\n", p_proc_actual->id, segundos);
	//Poner proceso a estado bloqueado
	p_proc_actual->estado=BLOQUEADO;
	//Indicar cuantos ticks debe dormir (TICKS = ticks/segundos)
	p_proc_actual->t_dormir=segundos*TICK;
	BCP* p_proc_dormido = p_proc_actual; //Guardar el proceso que se va a bloquear
	//Quitar de la lista de listos (donde esta el primero) y poner en lista bloqueados
	eliminar_primero(&lista_listos);
	insertar_ultimo(&lista_dormidos, p_proc_dormido);

	//Obtener nuevo proceso actual con planificador y hacer cambio contexto	
	p_proc_actual=planificador();
	//Hay que guardar el contexto del proceso a dormir para poder restaurarlo cuando se desbloquee
	printk("CAMBIO CONTEXTO DE %d A %d\n", p_proc_dormido->id, p_proc_actual->id);
	cambio_contexto(&(p_proc_dormido->contexto_regs), &(p_proc_actual->contexto_regs));

	//Volver al nivel de interrupcion anterior
	fijar_nivel_int(nivel_int);

	return 0;
}


/*** MUTEX ***/
// Funciones AUXILIARES que se utilizan en crear_mutex, abrir_mutex, lock, unlock y cerrar_mutex
int nombre_max (char *nombre) {
	printk("nombre: %s\n", nombre);
	int long_nom = strlen(nombre);
	if (long_nom > MAX_NOM_MUT) {
		printk("El nombre del mutex es demasiado largo\n");
		return -1;
	}
	else {
		return 0;
	}
}

int descriptor_libre () {
	
	int aux = -1;
	int i = 0;

	while((aux == -1) && (i <= NUM_MUT_PROC)) {
		//Si descriptor = -1, no ha sido utilizado
		if(p_proc_actual->descriptores[i] == -1) {
			aux = i;
		}
		
		i++;
	}

	return aux;		
}

int nombres_iguales(char *nombre) { 
	for(int i=0; i<mutex_creados; i++) {
		//Si el mutex existe
		if(array_mutex[i].abierto != 0){
			//Strcmp devuelve 0 si las cadenas son iguales
			if((strcmp(nombre, array_mutex[i].nombre)) == 0) {
				return -1; //Si encuentra nombre igual
			}
		}
	}
	return 0; //Si no encuentra nombre igual
}


int descriptor_mutex () {
	int aux = -1;
	int i=0;
	while((aux == -1) && (i<NUM_MUT)) {
		//Si no ha sido abierto ninguna vez, esta disponible
		if(array_mutex[i].abierto == 0) {
			aux = i; //El descriptor es la posicion en el array
		}
		i++;
	}
	return aux;
}

// 4) Funciones MUTEX
int crear_mutex (char *nombre, int tipo){
	printk("Creando mutex\n");
	nombre=(char *)leer_registro(1);
	tipo=(int)leer_registro(2);

	//Antes de crear el mutex, hay que realizar unas comprobaciones
	//Comprobar que el nombre no excede los caracteres maximos (incluir string.h)
	if (nombre_max(nombre) == -1) {
		return -1;
	}

	//Si hay algun mutex creado, comprobar que no existe ningun otro mutex con el mismo nombre
	if(mutex_creados>0) {
		int nombresig = nombres_iguales(nombre);	
		if(nombresig == -1) {
			printk ("Ya existe un mutex con el mismo nombre\n");
			return -1; //Da error si encuentra nombre igual
		}
	}

	//Comprueba si el proceso tiene descriptores libres
	//Si lo tiene, devuelve su posicion. Si no, da error
	int descriptor_proc = descriptor_libre();
	if(descriptor_proc == -1) {
		printk("El proceso no tiene descriptores libres\n");
		return -1; //Da error si no hay descriptores libres
	}

	//Comprobar si se ha alcanzado el numero maximo de mutex en el sistema
	while(mutex_creados==NUM_MUT) {
		printk("Numero mutex creados = NUM_MUT. Bloquear proceso.\n");
		//Si esto ocurre, hay que bloquear el proceso hasta que se elimine algun mutex
		//Mismos pasos que con dormir ¿?
		int nivel_int = fijar_nivel_int(NIVEL_3);

		//Poner proceso a estado bloqueado
		p_proc_actual->estado=BLOQUEADO;
		//Quitar de la lista de listos (donde esta el primero) y poner en lista bloqueados
		eliminar_primero(&lista_listos);
		insertar_ultimo(&lista_bloq_mutex, p_proc_actual);

		//Obtener nuevo proceso actual con planificador y hacer cambio contexto
		BCP* p_proc_bloq = p_proc_actual; //Guardar el proceso que se va a bloquear
		p_proc_actual=planificador();
		//Hay que guardar el contexto del proceso a bloquear para poder restaurarlo cuando se desbloquee
		printk("CAMBIO CONTEXTO DE %d A %d\n", p_proc_bloq->id, p_proc_actual->id);
		cambio_contexto(&(p_proc_bloq->contexto_regs), &(p_proc_actual->contexto_regs));

		//Volver al nivel de interrupcion anterior
		fijar_nivel_int(nivel_int);
	}
	

	//Ahora ya se puede crear el mutex
	//Obtener descriptor de mutex libre para acceder a el
	int descriptor_mut = descriptor_mutex();

	//Añadir este descriptor al array de descriptores del proceso en la posicion correspondiente
	p_proc_actual->descriptores[descriptor_proc] = descriptor_mut;
	printk("Descriptor_proc %d -> descr_mut = %d\n", descriptor_proc, descriptor_mut);

	//Actualizar variables mutex
	array_mutex[descriptor_mut].nombre = nombre;
	array_mutex[descriptor_mut].tipo = tipo;
	array_mutex[descriptor_mut].abierto++;
	mutex_creados++;

	//Actualizar variables proceso
	p_proc_actual->n_descriptores++;

	printk("Mutex creado correctamente\n");
	printk("MUTEX CREADOS = %d\n", mutex_creados);
	printk("\n");
	return descriptor_proc; //Devuelve el descriptor del mutex creado
}

int abrir_mutex(char *nombre) {
	printk("Abriendo mutex\n");
	nombre=(char *)leer_registro(1);

	//Primero hay que realizar unas comprobaciones
	//Comprobar que el nombre no excede los caracteres maximos (incluir string.h)
	if (nombre_max(nombre) == -1) {
		return -1;
	}
	//Comprobar que existe el mutex (buscar por nombre)
	if((nombres_iguales(nombre)) == 0) {
		//Si no encuentra un nombre igual
		printk("No se ha encontrado un mutex con este nombre\n");
		return -1;
	}
	//Buscar el descriptor en el proceso
	int descriptor_proc = descriptor_libre();
	if(descriptor_proc == -1) {
		printk("El proceso no tiene descriptores libres\n");
		return -1; //Da error si no hay descriptores libres
	}

	//Dado el nombre, hay que encontrar el descriptor del mutex
	int descriptor_mut;
	//Recorre el array de mutex
	for(int i=0; i<mutex_creados; i++) {
		if(strcmp(array_mutex[i].nombre, nombre) == 0) {
			descriptor_mut = i; //cuando encuentra el mutex, guarda el descriptor
		}
	}

	//Asigna descriptor
	p_proc_actual->descriptores[descriptor_proc]=descriptor_mut;
	printk("Descriptor_proc %d -> descr_mut = %d\n", descriptor_proc, descriptor_mut);
	//Actualizar variables proceso
	p_proc_actual->n_descriptores++;

	array_mutex[descriptor_mut].abierto++;

	printk("Mutex abierto correctamente\n");
	printk("\n");
	return descriptor_proc; //Devuelve el descriptor para que el proceso pueda acceder a el
}


int lock (unsigned int mutexid) {
	printk("Haciendo lock\n");
	int desc_proc=(unsigned int)leer_registro(1); //Recibe el descriptor mutex del proceso
	mutexid = p_proc_actual->descriptores[desc_proc];
	int proceso_esperando = 1; //Indica que el proceso aun no ha bloqueado el mutex

	//Si no se ha abierto el mutex, da error
	if(mutexid == -1) {
		printk("El mutex no existe \n");
		return -1;
	}

	printf("desc_proc = %d, mutexid = %d\n", desc_proc, mutexid);
	while (proceso_esperando) {

		//Comprobar si ya esta bloqueado
		if(array_mutex[mutexid].locked > 0) {
			//Si es recursivo y ha sido bloqueado por el mismo proceso
			// se podría volver a bloquear
			if((array_mutex[mutexid].tipo) == RECURSIVO) {
				//Comprobar si ha sido bloqueado por el proceso actual
				if(array_mutex[mutexid].propietario == p_proc_actual->id) {
					//El proceso actual seria el dueño y se puede volver a bloquear
					array_mutex[mutexid].locked++;
					proceso_esperando = 0;
				}
				//Si aux!=1, mutex no ha sido bloqueado por proc actual.
				//Hay que bloquear el proceso --> Cola del mutex!!
				else {
					printk("El mutex ya ha sido bloqueado por otro proceso. Se bloquea el proceso actual.\n");
					int nivel_int = fijar_nivel_int(NIVEL_3);

					//Poner proceso a estado bloqueado
					p_proc_actual->estado=BLOQUEADO;
					//Quitar de la lista de listos (donde esta el primero) y poner en lista bloqueados
					eliminar_primero(&lista_listos);
					insertar_ultimo(&(array_mutex[mutexid].lista_proc_esperando_lock), p_proc_actual);
					
					//Obtener nuevo proceso actual con planificador y hacer cambio contexto
					BCP* p_proc_bloq = p_proc_actual; //Guardar el proceso que se va a bloquear
					p_proc_actual=planificador();
					//Hay que guardar el contexto del proceso a bloquear para poder restaurarlo cuando se desbloquee
					printk("CAMBIO CONTEXTO DE %d A %d\n", p_proc_bloq->id, p_proc_actual->id);
					cambio_contexto(&(p_proc_bloq->contexto_regs), &(p_proc_actual->contexto_regs));

					//Volver al nivel de interrupcion anterior
					fijar_nivel_int(nivel_int);
				} // fin if bloqueado o no por proc actual

			} // fin if recursivo
			//Si no es recursivo, no se puede volver a bloquear. 
			else {
				//Comprobar si ha sido bloqueado por el proceso actual
				if(array_mutex[mutexid].propietario == p_proc_actual->id) {				
					//El proceso actual ha bloqueado el mutex antes -> interbloqueo
					printk("Se esta intentando bloquear un mutex no recursivo ya bloqueado por este proceso\n");
					return -1;
				}
				//Si el mutex lo ha bloqueado otro proceso, hay que bloquear proc actual
				else {
					printk("El mutex ya ha sido bloqueado por otro proceso. Se bloquea el proceso actual.\n");
					int nivel_int = fijar_nivel_int(NIVEL_3);

					//Poner proceso a estado bloqueado
					p_proc_actual->estado=BLOQUEADO;
					//Quitar de la lista de listos (donde esta el primero) y poner en lista bloqueados
					eliminar_primero(&lista_listos);
					insertar_ultimo(&(array_mutex[mutexid].lista_proc_esperando_lock), p_proc_actual);
					
					//Obtener nuevo proceso actual con planificador y hacer cambio contexto
					BCP* p_proc_bloq = p_proc_actual; //Guardar el proceso que se va a bloquear
					p_proc_actual=planificador();
					//Hay que guardar el contexto del proceso a bloquear para poder restaurarlo cuando se desbloquee
					printk("CAMBIO CONTEXTO DE %d A %d\n", p_proc_bloq->id, p_proc_actual->id);
					cambio_contexto(&(p_proc_bloq->contexto_regs), &(p_proc_actual->contexto_regs));

					//Volver al nivel de interrupcion anterior
					fijar_nivel_int(nivel_int);
				} // fin if mutex bloqueado o no por proc actual
			} // fin if no recursivo
		} //fin if mutex bloqueado
		//Si el mutex no esta ya bloqueado, se bloquea
		else {
			array_mutex[mutexid].locked++;
			proceso_esperando = 0;
		} // fin if bloqueado
	} // fin while

	//Si no se han producido errores, se asigna el proceso actual como propietario del mutex
	array_mutex[mutexid].propietario = p_proc_actual->id;
	printk("Lock realizado sobre mutex con id=%d\n", mutexid);
	printk("\n");

	return 0;
}
int unlock (unsigned int mutexid) {
	printk("Haciendo unlock\n");
	//mutexid=(unsigned int)leer_registro(1);

	int desc_proc=(unsigned int)leer_registro(1); //Recibe el descriptor mutex del proceso
	mutexid = p_proc_actual->descriptores[desc_proc];

	//Si aun no ha sido abierto, da error
	if(array_mutex[mutexid].abierto == 0) {
		printk("El mutex no ha sido abierto.\n");
		return -1;
	}

	//Comprobar si ya esta bloqueado
	if(array_mutex[mutexid].locked > 0) {
		//Si es recursivo y ha sido bloqueado por el mismo proceso
		// se puede desbloquear
		if((array_mutex[mutexid].tipo) == RECURSIVO) {
			//Comprobar si ha sido bloqueado por el proceso actual
			if(array_mutex[mutexid].propietario == p_proc_actual->id) {
				//El proceso actual seria el dueño y se puede desbloquear
				array_mutex[mutexid].locked--;
				//Como es recursivo, se puede haber bloqueado varias veces
				//Comprobamos si sigue bloqueado antes de cederlo a otro proceso
				if(array_mutex[mutexid].locked == 0) {
					//Si hay algun proceso esperando el mutex por lock, se le desbloquea --> cola mutex!
					if(((array_mutex[mutexid].lista_proc_esperando_lock).primero) != NULL) {
						int nivel_int = fijar_nivel_int(NIVEL_3);

						BCP* proc_esperando = (array_mutex[mutexid].lista_proc_esperando_lock).primero;
						proc_esperando->estado = LISTO;
						eliminar_primero(&(array_mutex[mutexid].lista_proc_esperando_lock)); 
						insertar_ultimo(&lista_listos, proc_esperando);

						fijar_nivel_int(nivel_int);
						printk("Se ha desbloqueado el proceso %d\n", proc_esperando->id);
					} //fin if proceso esperando
					//Si se ha desbloqueado, hay que "eliminar" el dueño
					array_mutex[mutexid].propietario = -1;
				} //fin if mutex desbloqueado
			} //fin if bloqueado o no por proc actual
			//Si no lo ha bloqueado este proceso, da error
			else {
				printk("Este proceso no ha bloqueado el mutex\n");
				return -1;
			} 
		} //fin if recursivo
		//Si no es recursivo, solo se puede desbloquear una vez
		else {
			//Comprobar si ha sido bloqueado por el proceso actual
			if(array_mutex[mutexid].propietario == p_proc_actual->id) {
				//El proceso actual seria el dueño y se desbloquear
				array_mutex[mutexid].locked--;
				//Si se ha desbloqueado, hay que "eliminar" el dueño
				array_mutex[mutexid].propietario = -1;
				//Si hay algun proceso esperando el mutex por lock, se le desbloquea --> Cola mutex!
				if(((array_mutex[mutexid].lista_proc_esperando_lock).primero) != NULL) {
					int nivel_int = fijar_nivel_int(NIVEL_3);

					BCP* proc_esperando = (array_mutex[mutexid].lista_proc_esperando_lock).primero;
					proc_esperando->estado = LISTO;
					eliminar_primero(&(array_mutex[mutexid].lista_proc_esperando_lock)); 
					insertar_ultimo(&lista_listos, proc_esperando);

					fijar_nivel_int(nivel_int);
					printk("Se ha desbloqueado el proceso %d\n", proc_esperando->id);
				} //fin if proceso esperando
			} //fin if mutex bloqueado por proceso actual
			//Si no lo ha bloqueado el proceso actual, da error
			else {
				printk("Este proceso no ha bloqueado el mutex\n");
				return -1;
			}
		} //fin if no recursivo
	} //fin if bloqueado
	//Si no estaba bloqueado, da error
	else {
		printk("Este mutex no esta bloqueado.\n");
		return -1;
	}

	printk("Unlock realizado correctamente sobre el mutex con id=%d\n", mutexid);
	return 0;
}


int cerrar_mutex (unsigned int mutexid) {
	printk("Cerrando mutex\n");
	unsigned int registro=(unsigned int)leer_registro(1);
	//Esto se hace para evitar problemas con la llamada a cerrar desde terminar_proceso
	if(registro < 16) {
		mutexid = registro;
	}
	
	//Primero se comprueba si existe el mutex que se quiere cerrar
	int desc_mut = p_proc_actual->descriptores[mutexid];
	printk("desc_mut = %d\n", desc_mut);
	if(array_mutex[desc_mut].nombre == NULL) {
		printk("No se ha encontrado el descriptor del mutex. No se puede cerrar.\n");
		return -1;
	}

	//Una vez encontrado, se libera
	p_proc_actual->descriptores[mutexid] = -1;
	p_proc_actual->n_descriptores--;
	printk("Descriptores usados: %d\n", p_proc_actual->n_descriptores);

	//Si se intenta cerrar un mutex que esta bloqueado por el mismo proceso, hay que desbloquearlo
	if(array_mutex[desc_mut].propietario == p_proc_actual->id) {
		printk("Desbloquear el mutex implicitamente\n");
		array_mutex[desc_mut].locked = 0;

		//Comprobar si hay algun proceso esperando al mutex por lock
		//Si lo hay, se desbloquea --> cola mutex
		//Hay que desbloquear todos los procesos bloqueados porque se va a eliminar el mutex
		while((array_mutex[desc_mut].lista_proc_esperando_lock).primero != NULL) {
			printk("Hay un proceso esperando el mutex por lock\n");
			int nivel_int = fijar_nivel_int(NIVEL_3);

			BCP* proc_esperando = (array_mutex[desc_mut].lista_proc_esperando_lock).primero;
			proc_esperando->estado = LISTO;
			eliminar_primero(&(array_mutex[desc_mut].lista_proc_esperando_lock)); 
			insertar_ultimo(&lista_listos, proc_esperando);

			fijar_nivel_int(nivel_int);
			printk("Se ha desbloqueado ese proceso %d\n", proc_esperando->id);
		}
	
	}
	array_mutex[desc_mut].abierto--;

	printk("Mutex %d cerrado correctamente\n", desc_mut);
	
	//Si no hay otros procesos que hayan abierto el mutex, este desaparece
	if(array_mutex[desc_mut].abierto <= 0) {
		printk("El mutex %d desaparece porque no hay otros procesos que lo hayan abierto\n", desc_mut);
		mutex_creados--;
		
		//Comprobar si hay algun proceso esperando al mutex porque habia el numero maximo
		//Si lo hay, se desbloquea --> cola global!!
		//Hay que desbloquear todos los procesos bloqueados porque se va a eliminar el mutex
		while(lista_bloq_mutex.primero != NULL) {
			printk("Hay algún proceso bloqueado por numero maximo de mutex\n");
			int nivel_int = fijar_nivel_int(NIVEL_3);

			BCP* proc_esperando = lista_bloq_mutex.primero;
			proc_esperando->estado = LISTO;
			eliminar_primero(&lista_bloq_mutex); 
			insertar_ultimo(&lista_listos, proc_esperando);

			fijar_nivel_int(nivel_int);
			printk("Se ha desbloqueado el proceso %d\n", proc_esperando->id);
		}
	}

	printk("\n");

	return 0;
}

//5.3) Llamada al sistema que lee y devuelve caracteres del buffer
int leer_caracter () {
	//Elevar a nivel int 3 para evitar que salte int terminal antes de tiempo!!
	int nivel_int1 = fijar_nivel_int(NIVEL_3);
	int car;

	//Bloquear el proceos mientras que el buffer esta vacio
	//Se hace con while para asegurar que solo un proceso lee el caracter --> pista libro
	while(car_leidos == 0) {
		printk("No hay caracteres, se bloquea el proceso %d\n", p_proc_actual->id);
		int nivel_int2 = fijar_nivel_int(NIVEL_3);

		//Poner proceso a estado bloqueado
		p_proc_actual->estado=BLOQUEADO;
		//Quitar de la lista de listos (donde esta el primero) y poner en lista bloqueados
		eliminar_primero(&lista_listos);
		insertar_ultimo(&lista_bloq_buffer, p_proc_actual);

		//Obtener nuevo proceso actual con planificador y hacer cambio contexto
		BCP* p_proc_bloq = p_proc_actual; //Guardar el proceso que se va a bloquear
		p_proc_actual=planificador();
		//Hay que guardar el contexto del proceso a bloquear para poder restaurarlo cuando se desbloquee
		printk("CAMBIO CONTEXTO DE %d A %d\n", p_proc_bloq->id, p_proc_actual->id);
		cambio_contexto(&(p_proc_bloq->contexto_regs), &(p_proc_actual->contexto_regs));

		//Volver al nivel de interrupcion anterior
		fijar_nivel_int(nivel_int2);
	}
	//Cuando sale de este bucle es que hay algun caracter disponible, asi que lo lee
	car = buffer[0];

	//!!! Hay que borrar este caracter y reajustar el buffer
	//Como lee el primero del array, hay que mover cada elemento a la izquierda
	car_leidos--;
	for(int i=0; i<= car_leidos; i++) {
		buffer[i] = buffer[i+1];
	}

	fijar_nivel_int(nivel_int1); //Restaurar nivel de interrupcion original

	return car;

}

/*
 *
 * Rutina de inicialización invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();

	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	
	return 0;
}

#include <nSystem.h>
#include "fifoqueue.h"
#include "transbordo.h"
#include "test-transbordo.c"
#define PARGUA 1
#define CHACAO 2

m = nMakeMonitor();	
colaPargua = MakeFifoQueue();
colaChacao = MakeFifoQueue();

typedef struct {
	int p; /* numero transbordador */
	int v; /* vehiculo transportado */
	int origen; /* 1 = Pargua, 0 = Chacao */
} Transbordador;

void inicializar(int p) {
	
	for(int i = 0; i < p; i++) {
		Transbordador *t = (Transbordador)nMalloc(siseof(*t));
		t->p = i;
		t->v = -1; // transbordador vacio
		t->origen = PARGUA;
		PushObj(colaPargua, t);
	}
}

void transbordoAChacao(int v){
	nEnter(m);
	while (EmptyFifoQueue(colaPargua))
 		nWait(m); 
 	nNotifyAll(m); 
	nExit(m);
	haciaPargua(t->i, v); // viaje a paralelizar
}

void transbordoAPargua(int v) {
	nEnter(m);
	while (EmptyFifoQueue(colaChacao))
 		nWait(m); 
 	nNotifyAll(m); 
	nExit(m);
	haciaChacao(t->i, v); // viaje a paralelizar
}

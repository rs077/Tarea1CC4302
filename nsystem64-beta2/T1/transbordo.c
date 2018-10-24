#include <nSystem.h>
#include "fifoqueues.h"
#include "transbordo.h"

typedef struct {
  int idFerry; // identificador del ferry
} Ferry; // estructura para el ferry

typedef struct {
  nMonitor m; // monitor general
  nCondition waitPargua; // condicion para vehiculos en espera en Pargua
  FifoQueue colaVehiculosPargua; // cola para vehiculos en Pargua
  FifoQueue colaFerrysPargua; // cola para ferrys en Pargua
  nCondition waitChacao; // condicion para vehiculos en espera en Chacao
  FifoQueue colaVehiculosChacao; // cola para vehiculos en Chacao
  FifoQueue colaFerrysChacao; // cola para ferrys en Chacao
} FerryCtrl; // estructura para el control de ferrys

FerryCtrl *fctrl;

FerryCtrl *makeFerryCtrl() { // funcion para crear la estructura control de ferrys
	FerryCtrl *f = nMalloc(sizeof(FerryCtrl)); // se pide espacio en memoria para crear la estructura
	(*f).m = nMakeMonitor(); // se crear el monitor
	(*f).waitPargua = nMakeCondition((*f).m); // se crea la condicion de espera en Pargua
	(*f).colaVehiculosPargua = MakeFifoQueue(); // se crea la cola para los vehiculos en Pargua
	(*f).colaFerrysPargua = MakeFifoQueue(); // se crea la cola para los ferrys en Pargua
	(*f).waitChacao = nMakeCondition((*f).m); // se crea la condicion de espera en Chacao
	(*f).colaVehiculosChacao = MakeFifoQueue(); // se crea la cola para los vehiculos en Chacao
	(*f).colaFerrysChacao = MakeFifoQueue(); // se crea la cola para los ferrys en Chacao
	return f; 
}

void inicializar(int p) { // funcion que inicializa y encola los ferrys a la cola de ferrys de Pargua
  fctrl = makeFerryCtrl(); // se crea la estructura de control de ferrys
  for (int i = 0; i < p; i++) { 
    Ferry *ferry = nMalloc(sizeof(ferry));
    (*ferry).idFerry = i; // se asignan numeros con valores que van de 0 a (p-1) a cada ferry
    PutObj((*fctrl).colaFerrysPargua, ferry); // se encolan los ferrys en Pargua
  }
}

void finalizar() {
  nFree(fctrl); // se libera el espacio pedido por la estructura
}

void transbordoAChacao(int v) {
  nEnter((*fctrl).m); // se entra a la seccion critica
  // si la cola de ferrys en Pargua esta vacia o la cola de vehiculos en Pargua no lo esta
  if (EmptyFifoQueue((*fctrl).colaFerrysPargua) || !EmptyFifoQueue((*fctrl).colaVehiculosChacao)) {
  	// se pone el vehiculo a la cola 
  	PutObj((*fctrl).colaVehiculosPargua, &v);
  }
  // mientras no haya ferrys en Pargua
  while (EmptyFifoQueue((*fctrl).colaFerrysPargua)) {
  	// si hay ferrys en Chacao y no hay vehiculos en esperando, se manda un ferry vacio a Pargua
    if(!EmptyFifoQueue((*fctrl).colaFerrysChacao) && EmptyFifoQueue((*fctrl).colaVehiculosChacao)) {
    	Ferry *ferry = GetObj(fctrl->colaFerrysChacao); // se desencola el ferry de Chacao
    	haciaPargua(ferry->idFerry,-1); // se envia el ferry vacio 
    	PutObj((*fctrl).colaFerrysPargua, ferry); // se encola el ferry en Pargua
    	nSignalCondition((*fctrl).waitPargua); // se avisa que llego un ferry a Pargua a los vehiculos
    }
    else { // si no
    	nWaitCondition((*fctrl).waitPargua); // el vehiculo se pone en espera
    }
  }
  // si hay ferrys en Pargua y vehiculos en la cola, hace transbordo a Chacao
  Ferry *ferry = GetObj(fctrl->colaFerrysPargua);
  nExit((*fctrl).m); // fin seccion critica
  haciaChacao(ferry->idFerry,v); // se envia el ferry con un vehiculo
  nEnter((*fctrl).m); // inicio seccion critica
  PutObj((*fctrl).colaFerrysChacao, ferry);  // el ferry llega a Chacao y se encola
  nSignalCondition((*fctrl).waitChacao); // se avisa que llego un ferry a Chacao a los vehiculos
  DeleteObj((*fctrl).colaVehiculosPargua, &v); // se elimina el vehiculo de la cola
  nExit((*fctrl).m); // fin seccion critica
  return;
}

void transbordoAPargua(int v) {
  nEnter((*fctrl).m); // se entra a la seccion critica
  // si la cola de ferrys en Chacao esta vacia o la cola de vehiculos en Chacao no lo esta
  if (EmptyFifoQueue((*fctrl).colaFerrysChacao) || !EmptyFifoQueue((*fctrl).colaVehiculosChacao)) {
  	// se pone el vehiculo a la cola 
  	PutObj((*fctrl).colaVehiculosChacao, &v);
  }
  // mientras no haya ferrys en Pargua
  while (EmptyFifoQueue((*fctrl).colaFerrysChacao)){
  	// si hay ferrys en Pargua y no hay vehiculos en esperando, se manda un ferry vacio a Chacao
  	if(!EmptyFifoQueue((*fctrl).colaFerrysPargua) && EmptyFifoQueue((*fctrl).colaVehiculosPargua)) {
    	Ferry *ferry = GetObj((*fctrl).colaFerrysPargua); // se desencola el ferry de Pargua
    	haciaChacao(ferry->idFerry,-1);
    	PutObj((*fctrl).colaFerrysChacao, ferry); // se encola el ferry en Chacao
    	nSignalCondition((*fctrl).waitChacao); // se avisa que llego un ferry a Chacao a los vehiculos
    }

    else{ // sino
    	nWaitCondition((*fctrl).waitChacao); // el vehiculo se pone en espera
    }
  }
  // si hay ferrys en Pargua y vehiculos en la cola, hace transbordo a Pargua
  Ferry *ferry = GetObj(fctrl->colaFerrysChacao);
  nExit((*fctrl).m); // fin de seccion critica
  haciaPargua(ferry->idFerry,v); // se envia el ferry con un vehiculo
  nEnter((*fctrl).m); // inicio de seccion critica
  PutObj((*fctrl).colaFerrysPargua, ferry);  // el ferry llega a Pargua y se encola
  nSignalCondition((*fctrl).waitPargua); // se avisa que llego un ferry a Pargua a los vehiculos
  DeleteObj((*fctrl).colaVehiculosChacao, &v); // se elimina el vehiculo de la cola
  nExit((*fctrl).m); // fin de seccion critica
  return;
}
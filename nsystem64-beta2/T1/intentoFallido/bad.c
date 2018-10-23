#include <nSystem.h>
#include "fifoqueues.h"
#include "transbordo.h"

int nFerrys; //aun no se ua

typedef struct {
  	int idFerry;
} Ferry;

typedef struct {
	nCondition w;
  int idVehiculo;
} Vehiculo;

typedef struct {
  nMonitor m;
  FifoQueue colaVehiculos;
  FifoQueue colaFerrys;
} FerryCtrl;

FerryCtrl *ctrlFerrysChacao;
FerryCtrl *ctrlFerrysPargua;

FerryCtrl *makeFerryCtrl() {
	FerryCtrl *f = nMalloc(sizeof(FerryCtrl));
	f->m = nMakeMonitor();
	f->colaVehiculos = MakeFifoQueue();
	f->colaFerrys = MakeFifoQueue();
	return f;
}

void inicializar(int p) {
  nFerrys = p; //aca solo se asigna este valor, cheqear despues
  ctrlFerrysChacao = makeFerryCtrl();
  ctrlFerrysPargua = makeFerryCtrl();
  for (int i = 0; i < p; i++) {
    Ferry *ferry = nMalloc(sizeof(ferry));
    ferry->idFerry = i;
    PutObj(ctrlFerrysPargua->colaFerrys, ferry);
  }
}

void finalizar() {
  nFree(ctrlFerrysChacao);
  nFree(ctrlFerrysPargua);
  
}

void esperarFerry(FerryCtrl *f, int v) {
  Vehiculo *vehiculo = nMalloc(sizeof(vehiculo));
  vehiculo->idVehiculo = v;
  vehiculo->w = nMakeCondition(f->m);
  PutObj(f->colaVehiculos, vehiculo);
  nWaitCondition(vehiculo->w);
  nDestroyCondition(vehiculo->w);
}

void notificarCola(FerryCtrl *f) {
  Vehiculo *vehiculo= GetObj(f->colaVehiculos);
  if (vehiculo == NULL){
    return;
  }
  if (!EmptyFifoQueue(f->colaFerrys)){
    nSignalCondition(vehiculo->w);
  }
  else {
    PushObj(f->colaVehiculos, vehiculo);
  }
}

void notificarLlegada(FerryCtrl *f) {
  Vehiculo *vehiculo = GetObj(f->colaVehiculos);
  if (vehiculo == NULL)
    return;
  nSignalCondition(vehiculo->w);
}

void transbordoAChacao(int v) {
  nEnter(ctrlFerrysPargua->m);
  if (!EmptyFifoQueue(ctrlFerrysPargua->colaVehiculos)){
    esperarFerry(ctrlFerrysPargua, v);
  }
  while (!EmptyFifoQueue(ctrlFerrysPargua->colaFerrys)){
    if (EmptyFifoQueue(ctrlFerrysChacao->colaVehiculos) && EmptyFifoQueue(ctrlFerrysPargua->colaFerrys)) {
      Ferry *ferry = GetObj(ctrlFerrysChacao->colaFerrys);
      haciaPargua(ferry->idFerry,-1);
      PutObj(ctrlFerrysPargua->colaFerrys, ferry);
      return;
    }
    else {
      esperarFerry(ctrlFerrysPargua, v);
    }
  }
  Ferry *ferry = GetObj(ctrlFerrysPargua->colaFerrys);
  notificarCola(ctrlFerrysPargua);
  nExit(ctrlFerrysPargua->m);
  haciaChacao(ferry->idFerry,v);
  nEnter(ctrlFerrysChacao->m);
  PutObj(ctrlFerrysChacao->colaFerrys, ferry);
  notificarCola(ctrlFerrysChacao);
  nExit(ctrlFerrysChacao->m);
  nEnter(ctrlFerrysPargua->m);
  notificarLlegada(ctrlFerrysPargua);
  nExit(ctrlFerrysPargua->m);
  return;
}

void transbordoAPargua(int v) {
  nEnter(ctrlFerrysChacao->m);
  if (!EmptyFifoQueue(ctrlFerrysChacao->colaVehiculos)){
    esperarFerry(ctrlFerrysChacao, v);
  }
  while (!EmptyFifoQueue(ctrlFerrysChacao->colaFerrys)){
    if (EmptyFifoQueue(ctrlFerrysPargua->colaVehiculos) && EmptyFifoQueue(ctrlFerrysChacao->colaFerrys)) {
      Ferry *ferry = GetObj(ctrlFerrysPargua->colaFerrys);
      haciaChacao(ferry->idFerry,-1);
      PutObj(ctrlFerrysChacao->colaFerrys, ferry);
      return;
    }
    else {
      esperarFerry(ctrlFerrysChacao, v);
    }
  }
  Ferry *ferry = GetObj(ctrlFerrysChacao->colaFerrys);
  notificarCola(ctrlFerrysChacao);
  nExit(ctrlFerrysChacao->m);
  haciaPargua(ferry->idFerry,v);
  nEnter(ctrlFerrysPargua->m);
  PutObj(ctrlFerrysPargua->colaFerrys, ferry);
  notificarCola(ctrlFerrysPargua);
  nExit(ctrlFerrysPargua->m);
  nEnter(ctrlFerrysChacao->m);
  notificarLlegada(ctrlFerrysChacao);
  nExit(ctrlFerrysChacao->m);
  return;
}
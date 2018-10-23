#include <nSystem.h>
#include "fifoqueues.h"
#include <stdio.h>

typedef struct {
  nMonitor m;
  FifoQueue q;
  int *shipsarray;
  int ships;
} ShipControl;

typedef struct {
  int id;
  nCondition w;
} Vehicle;

int nships;

ShipControl *makeSControl(int n) {
    ShipControl *sc = (ShipControl *)nMalloc(sizeof(*sc));
    sc->m= nMakeMonitor();
    sc->q= MakeFifoQueue();
    sc->shipsarray = (int *)nMalloc(sizeof(int)*nships);
    sc->ships= n;
    return sc;
}

ShipControl *scPargua;
ShipControl *scChacao;

void inicializar(int p) {
  nships = p;
  scPargua = makeSControl(nships);
  scChacao = makeSControl(0);
  // Inicializa el array de 1's en Pargua para los barcos
  for (int i = 0; i < p; i++) {
    scPargua->shipsarray[i] = 1;
    scChacao->shipsarray[i] = 0;
  }
}

void finalizar() {
  nFree(scPargua);
  nFree(scChacao);
}

void esperarBarco(ShipControl *sc, int v) {
  Vehicle vehicle;
  vehicle.id = v;
  vehicle.w = nMakeCondition(sc->m);
  PutObj(sc->q, &vehicle);
  nWaitCondition(vehicle.w);
  nDestroyCondition(vehicle.w);
}

void llamarBarco(ShipControl *scOrigen, ShipControl *scDestino, void (*haciaDestino)(), int v) {
  // Solo llamará un barco si no hay fila de espera y estan todos los barcos en el origen
  if (EmptyFifoQueue(scOrigen->q) && scOrigen->ships == nships) {
    for (int i = 0; i < nships; i++) {
      if (scOrigen->shipsarray[i]) {
        scOrigen->shipsarray[i] = 0;
        scOrigen->ships--;
        haciaDestino(i,-1);
        scDestino->shipsarray[i] = 1;
        scDestino->ships++;
        return;
      }
    }
  }
  // Si no, el vehiculo debe agregarse a la cola
  else
    esperarBarco(scDestino, v);
}

void notificarCola(ShipControl *sc) {
  Vehicle *vehicle= (Vehicle*)GetObj(sc->q);
  if (vehicle == NULL)
    return;
  if (sc->ships > 0)
    nSignalCondition(vehicle->w);
  else {
    PushObj(sc->q, vehicle);
  }
}

void notificarLlegada(ShipControl *sc) {
  Vehicle *vehicle= (Vehicle*)GetObj(sc->q);
  if (vehicle == NULL)
    return;
  nSignalCondition(vehicle->w);
}

void transbordoAChacao(int v) {
  nEnter(scPargua->m);
  // En caso de que haya fila, debe esperar
  if (!EmptyFifoQueue(scPargua->q))
    esperarBarco(scPargua, v);
  // Si no hay barcos, se llama un barco de Chacao a Pargua
  while (!scPargua->ships)
    llamarBarco(scChacao, scPargua, haciaPargua, v);
  // Revisa los barcos disponibles para iniciar el viaje
  for (int i = 0; i < nships; i++) {
    if (scPargua->shipsarray[i]) {
      scPargua->shipsarray[i] = 0;
      scPargua->ships--;
      // Hace la notificacion al que le toca en la cola
      notificarCola(scPargua);
      nExit(scPargua->m);
      // Viaje largo a Chacao
      haciaChacao(i,v);
      nEnter(scChacao->m);
      scChacao->shipsarray[i] = 1;
      scChacao->ships++;
      // Hace la notificacion al que le toca en la cola en Chacao y despues
      // a Pargua por si se cumple la condicion para llamar barcos
      notificarCola(scChacao);
      nExit(scChacao->m);
      nEnter(scPargua->m);
      notificarLlegada(scPargua);
      nExit(scPargua->m);
      return;
    }
  }
}

void transbordoAPargua(int v) {
  nEnter(scChacao->m);
  // En caso de que haya fila, debe esperar
  if (!EmptyFifoQueue(scChacao->q))
    esperarBarco(scChacao, v);
  // Si no hay barcos, se llama un barco de Pargua a Chacao
  while (!scChacao->ships)
      llamarBarco(scPargua, scChacao, haciaChacao, v);
  // Revisa los barcos disponibles para iniciar el viaje
  for (int i = 0; i < nships; i++) {
    if (scChacao->shipsarray[i]) {
      scChacao->shipsarray[i] = 0;
      scChacao->ships--;
      notificarCola(scChacao);
      nExit(scChacao->m);
      // Hace la notificacion al que le toca en la cola en Chacao y despues a Pargua
      // Viaje largo a Pargua
      haciaPargua(i,v);
      nEnter(scPargua->m);
      scPargua->shipsarray[i] = 1;
      scPargua->ships++;
      // Hace la notificacion al que le toca en la cola en Pargua y despues
      // a Chacao por si se cumple la condicion para llamar barcos
      notificarCola(scPargua);
      nExit(scPargua->m);
      nEnter(scChacao->m);
      notificarLlegada(scChacao);
      nExit(scChacao->m);
      return;
    }
  }
}

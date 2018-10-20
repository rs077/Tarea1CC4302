#include <nSystem.h>
#include "transbordo.h"

void inicializar(int p) {

}
void transbordoAChacao(int v){

}
void transbordoAPargua(int v) {

}

typedef struct {
	nMonitor m;
	FifoQueue q;
	int readers, writing;
} Ctrl;
#define READER 1
#define WRITER 2
typedef struct {
 int kind; /* READER || WRITER */
	nCondition w;
} Request;

Ctrl *makeCtrl() {
	Ctrl *c= (Ctrl)nMalloc(sizeof(*c));
	c->m= nMakeMonitor();
	c->q= MakeFifoQueue();
	c->readers= 0;
	c->writing= FALSE;
	return c;
}

void wakeup(Ctrl *c) {
	Request *pr= (Request*)GetObj(c->q);
	if (pr==NULL)
		return;
	if (pr->kind==READER && !c->writing)
		nSignalCondition(pr->w);
	else if (pr->kind=WRITER &&
		c->readers==0 && !c->writing)
		nSignalCondition(pr->w);
 else /* se devuelve al comienzo de q */
	PushObj(c->q, pr);
}
void await(Ctrl *c, int kind) {
	Request r;
	r.kind= kind;
	r.w= nMakeCondition(m);
 PutObj(c->q, &r); /* al final de q */
	nWaitCondition(r.w);
	nDestroyCondition(r.w);
}

void enterRead(Ctrl *c) {
	nEnter(c->m);
	if (c->writing ||
 !EmptyFifoQueue(c->q)) /* A */
		await(c, READER);
		c->readers++;
 wakeup(c); /* C */
		nExit(c->m);
	}
	void exitRead(Ctrl *c) {
		nEnter(c->m);
		c->readers--;
		if (c->readers==0)
			wakeup(c);
		nExit(c->m);

		void enterWrite(Ctrl *c) {
			nEnter(m);
			if (c->readers>0 || c->writing ||
 !EmptyFifoQueue(c->q)) /* B */
				await(c, WRITER);
				c->writing= TRUE;
				nExit(c->m);
			}

			void exitWrite(Ctrl *c) {
				nEnter(c->m);
				c->writing= FALSE;
				wakeup(c);
				nExit(c->m);
			}
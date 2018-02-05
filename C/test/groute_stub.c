#include <sys/ipc.h>
#include <sys/shm.h>
#include "groute_stub.h"
#include "groute.h"

extern int g_scen; 
extern int g_case; 

int stub_shmget(key_t key, size_t size, int shmflg)
{
	if(g_scen==2 && g_case==1 && key == GROUTE_INTF_SHM_KEY) {
		return -1;
	}

	if(g_scen==2 && g_case==2 && key == GROUTE_ROUTE_SHM_KEY) {
		return -1;
	}

	if(g_scen==2 && g_case==3 && key == GROUTE_INTF_SHM_KEY) {
		return 123;
	}

	if(g_scen==2 && g_case==4 && key == GROUTE_ROUTE_SHM_KEY) {
		return 123;
	}

	if(g_scen==21 && g_case==1 && key == GROUTE_INTF_SHM_KEY) {
		return -1;
	}

	if(g_scen==21 && g_case==2 && key == GROUTE_ROUTE_SHM_KEY) {
		return -1;
	}

	if(g_scen==21 && g_case==3 && key == GROUTE_INTF_SHM_KEY) {
		return 123;
	}

	if(g_scen==21 && g_case==4 && key == GROUTE_ROUTE_SHM_KEY) {
		return 123;
	}

	if(g_scen==23 && g_case==1 && key == GROUTE_INTF_SHM_KEY) {
		return -1;
	}

	if(g_scen==23 && g_case==2 && key == GROUTE_ROUTE_SHM_KEY) {
		return -1;
	}

	if(g_scen==23 && g_case==3 && key == GROUTE_INTF_SHM_KEY) {
		return 123;
	}

	if(g_scen==23 && g_case==4 && key == GROUTE_ROUTE_SHM_KEY) {
		return 123;
	}

	return shmget(key, size, shmflg);
}

void *stub_shmat(int shmid, const void *shmaddr, int shmflg)
{
	if(g_scen==2 && g_case==3 && shmid == 123) {
		return (void *)-1;
	}

	if(g_scen==2 && g_case==4 && shmid == 123) {
		return (void *)-1;
	}

	if(g_scen==21 && g_case==3 && shmid == 123) {
		return (void *)-1;
	}

	if(g_scen==21 && g_case==4 && shmid == 123) {
		return (void *)-1;
	}

	if(g_scen==23 && g_case==3 && shmid == 123) {
		return (void *)-1;
	}

	if(g_scen==23 && g_case==4 && shmid == 123) {
		return (void *)-1;
	}

	return shmat(shmid, shmaddr, shmflg);
}

int stub_shmdt(const void *shmaddr)
{
	return shmdt(shmaddr);
}

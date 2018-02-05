#ifndef GROUTE_STUB_H
#define GROUTE_STUB_H
#include <stdint.h>
#include <stddef.h>

int stub_shmget(key_t key, size_t size, int shmflg);
void *stub_shmat(int shmid, const void *shmaddr, int shmflg);
int stub_shmdt(const void *shmaddr);
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <errno.h>
/*3 tipologie di utilizzo della memoria condivisa*/
#include <sys/shm.h>
/*
Data structure: msqid_ds
Creazione/Apertura: msgget(...)
Chiusura: (none) || semctl(semid,0{non usato se macro == IPC_RMID},IPC_RMID)
Operazioni di controllo: msgctl(...)
Operazioni di utilizzo: msgsnd(...),msgrcv(...)
*/
#include <sys/sem.h>
/*  
Data structure: semid_ds
Creazione/Apertura: semget(...)
Chiusura: (none)
Operazioni di controllo: semstl(...)
Operazioni di utilizzo: semop(...)
*/
#include <sys/msg.h>
/*
Data structure: shiid_ds
Creazione/Apertura: shmget(...)
Chiusura: dhmdt(...)
Operazioni di controllo: shmtl(...)
Operazioni di utilizzo: Accesso nella locazione
*/





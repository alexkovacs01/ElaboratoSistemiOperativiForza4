#include "all_lib.h"
#include "my_own_library_final.h"
#include "../inc/fifo.h"
#include <signal.h>

struct myMsg {
    long mtype; /* tipo di messaggio {obbligatorio} */ 
    int PlayerNumber;   // -> numero del giocatore
};

void handleStartSingal(int signal);
void errExit(const char *msg);
void semOp (int semid, unsigned short sem_num, short sem_op);
void handleEndGameSingal(int signal);


int gameStarted = 0;
int playerTurn = 0; // 0 per giocatore 1, 1 per giocatore 2
int end_flag = 0;

int main(int argc, char *argv[]) {

    char *username;

    username = argv[1];

    printf("Welcome %s\n", username);

    /*MSGQ*/

    // chiave della msgq
    key_t msgq_key = 92;

    // creazione msgq
    int msgq_id = msgget(msgq_key, IPC_EXCL | S_IRUSR | S_IWUSR);

    // vefiica 
    if (msgq_id == -1)
        errExit("Errore nella creazione della msgq\n");

    struct myMsg mymsg;

    // 9 è mytpe {tipo di player}
    if(msgrcv(msgq_id,&mymsg,sizeof(mymsg)-sizeof(long),9,IPC_NOWAIT) == -1)  {
        if (errno == ENOMSG)    // -> posso captare varie flag 
            errExit("Partita piena\n");
        else 
            errExit("Errore nella ricezione del messaggio della msgq\n");
    }
        
    /*FIFO*/
    char pathFIFO[] = "/tmp/myfifo";

    printf("[DEBUG] [CLIENT] Start client...\n");
	struct bufFIFO buf;
	buf.int1 = 0;
	buf.int2 = 0;
	buf.messageType = 1;
	buf.pidSender = getpid();
	buf.messageType = 1; // For request connection
	buf.str1[0] = 'A';
	char name[strlen(argv[1])];
    strcpy(name,argv[1]);
	strcpy(buf.str1, name);
	
	printf("[DEBUG] [CLIENT] message send, pid: %d, messageType: %d, str1: %s", buf.pidSender, buf.messageType, buf.str1);
	writeFIFO(pathFIFO, &buf, sizeof(buf));

    signal(SIGUSR1,handleStartSingal);

    while(!gameStarted)
        pause();    // -> Mi metto in attesa per l'inizio della partita 


    printf("[DEBUG] [CLIENT] Game started!\n");

    // prendo la chiave della memoria condivisa 
    key_t shm_key = 90;

    // accedo quindi alla memoria cndivisa
    int shm_id = shmget(shm_key,0,0);

    if (shm_id == -1)
        errExit("Errore nel collegamento con la memoria condivisa lato client\n");

    // creo il puntatore a questa zona di memoria per poi poterla modificare 
    int *shm_ptr = shmat(shm_id,NULL,0);

    if (shm_ptr == (void *)-1) 
        errExit("Errore nella connessione con la memoria convisa lato client\n");

    int mat_copy[5][5];

    for(int i = 0; i < 5; i++) 
        for(int j = 0; j < 5; j++)
            mat_copy[i][j] = shm_ptr[i * 5 +j];


    // accedo al set di semafori per sincronizzare la partita 
    key_t sem_key = 91;
    int sem_id = semget(sem_key,3,0);
    if (sem_id == -1)
        errExit("Errore nell'acquisizione dei semafori da parte del client\n");

    // Indici dei semafori nel set
    int serverSemaphore = 0;
    /*
    int player1Semaphore = 1;
    int player2Semaphore = 2;
    */

    // Esegui le operazioni di gioco per entrambi i giocatori
    // Puoi utilizzare un ciclo o una logica appropriata per gestire i turni dei giocatori

    while(1) {


        // blocco il semaforo del giocatore corrent
        semOp(sem_id,mymsg.PlayerNumber,-1);

        /*
        // problema, si impuntano entrambi sullo stesso semaforo
        if (playerTurn == 0) {
            // acquisisci il semaforo per il primo giocatore
            printf("Debug->acquisisco il semaforo del primo giocatore\n");
            semOp(sem_id,player1Semaphore,-1);
        }
        else {
            // acquisici il semaforo per il secondo giocatore
            printf("Debug->acquisisco il semaforo del scondo giocatore\n");
            semOp(sem_id,player2Semaphore,-1);
        }
        */

        // aggiorno la copia della mia tabella
        for(int i = 0; i < 5; i++) 
            for(int j = 0; j < 5; j++)
                mat_copy[i][j] = shm_ptr[i*5+j];

        printf("Ho acquisito almeno un semaforo\n");

        if (signal(SIGUSR2,handleEndGameSingal) == SIG_ERR)
            errExit("Errore nella ricezione del sengale\n");

        if (end_flag == 1)  // se la partita è finita interrompo
            exit(0);    //fine dei giochi

        /*operazione del giocatore*/

        //printf("<Client> tocca giocatore: [%i]\n", playerTurn+1); -> deprecato
        printf("\n<Client> ecco il campo di gioco: \n");
        // queste dimensioni me le devo far passare dal server 
        print_table(mat_copy,5,5);
        // modifica della matrice in memoria condivisa 
        // il turno+1 coincide col simbolo per la vittoria
        printf("<Client> inserisci: \n");
        insert_in_table(mat_copy,mymsg.PlayerNumber,mymsg.PlayerNumber,0/*no vs bot{1x gicare contro il bot}*/,5,5);

        // aggiorno la tabella in memoria condivisa
        for(int i = 0; i < 5; i++) 
            for(int j = 0; j < 5; j++)
                shm_ptr[i*5+j] = mat_copy[i][j];
        

        /*
        printf("DEBUG -> Rilascio del semaforo corrente per consentire all'altro di giocare\n");
        // rilascia il semaforo corrente per consentire all'altro giocatore di giocare
        if (playerTurn == 0)
            semOp(sem_id,player1Semaphore,1);
        else
            semOp(sem_id,player2Semaphore,1);
        */
        
        // sblocco il server che esegue l'operazione di controllo della vittoria 
        semOp(sem_id,serverSemaphore,1);

        /*
        // Passa al turno successivo
        playerTurn = (playerTurn + 1) % 2;  
        */

        //exit(0);
    }
    



    return 0;
}


void handleStartSingal(int signal) {
    if(signal == SIGUSR1)
        gameStarted = 1;
}

void handleEndGameSingal(int signal) {
    if(signal == SIGUSR2) {
        end_flag = 1;
    }  
}

void errExit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// da rendere come esterna dato che la uso sia nel client che nel server
void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1)
        errExit("SemOp failed\n");
}
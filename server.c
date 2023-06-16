#include "all_lib.h"
#include <sys/sem.h>
#include "my_own_library_final.h"
#include "fifo.h"
#include <signal.h>

struct myMsg {
    long mtype; /* tipo di messaggio {obbligatorio} */ 
    int PlayerNumber;   // -> numero del giocatore
    int dim1;   // -> numero righe;
    int dim2;   // -> numero colonne
    char token1;
    char token2;
    pid_t server_pid;
};

/*funzione generica che mi gestisce qualsiasi tipologia di errore incontrata durante il gioco*/
void errExit(const char *msg);
void waitForClient(char* pathFIFO, unsigned short client_nr, pid_t* pid, char* name);
void semOp (int semid, unsigned short sem_num, short sem_op);
void handleCtrlClient(int sig);
void handlerCtrlC(int sig);
void handleBotOption(int sig);

int end_game = 0, cont_ctrlc = 0, bot_optz = 0;
pid_t client_pid[2];

int main(int argc, char *argv[]) {

    //per linux altrimenti non funzionano bene i segnali 
    siginterrupt(SIGINT,1);
    siginterrupt(SIGUSR2,2);
    /* cosi facendo evito che ke sys call bloccanti vengano riavviate se si sono sbloccate a causa di un interrupt*/

    // controllo per uso minimo di parametri
    if (argc != 5) {
        printf("UTILIZZO SCORRETTO\n");
        printf("Inserisca ./F4server dim1 dim2 token1 token2\n");
        exit(-1);
    }

    // definiso il segnale per l'abbandono di un giocatore
    if(signal(SIGUSR2,handleCtrlClient) == SIG_ERR)
        errExit("Errore nel cambio del segnale(client)\n");

    int dim[2];         // dimensione passata come parametro

    /*./F4server row column token1 token2*/
    dim[0] = atoi(argv[1]);
    dim[1] = atoi(argv[2]);

    /*controllo se i parametri inseriti sono corretti altrimenti messaggio e chiusura {richiesto dal testo}*/
    if ((dim[0] < 5) || (dim[1] < 5)) {
        errExit("Dimensione minima non rispettata\n");
    }
    if (((strlen(argv[3])) > 1)  || (strlen(argv[4]) > 1)) {
        errExit("Devi inserire un unico carattere\n");
    }

    /*prototipo per matrice con dimensioni massime*/
    if ((dim[0] > 20) || (dim[1] > 20)) {
        errExit("Dimensione massima non rispettata\n");
    }

    /*predisposizione della partita*/

    /*creazione della msgq*/
    // creo la chiave a mano 
    key_t msgq_server_key = 92;  // chiave per la msgqueue

    // creo l'id della msgq
    int msgq_server_id = msgget(msgq_server_key, IPC_CREAT | S_IRUSR | S_IWUSR);

    if (msgq_server_id == -1)
        errExit("Errore nella creazione della msgq1\n");

    // creo due messaggi 
    struct myMsg mymsg1,mymsg2;

    // imposto il numero del giocatore all'intero dei miei 2 messaggi
    mymsg1.mtype = 9;   // 9 -> categoria di tipo di player {mi servirà per leggere messaggi di tipo 9[tipo di player]}
    mymsg2.mtype = 9;
    mymsg1.PlayerNumber = 1;
    mymsg2.PlayerNumber = 2;    
    mymsg1.dim1 = dim[0];
    mymsg2.dim1 = dim[0];
    mymsg1.dim2 = dim[1];
    mymsg2.dim2 = dim[1];
    mymsg1.token1 = *argv[3];
    mymsg2.token1 = *argv[3];
    mymsg1.token2 = *argv[4];
    mymsg2.token2 = *argv[4];
    mymsg1.server_pid = getpid();
    mymsg2.server_pid = getpid();
    
    // invio i messaggio alla msgq e controllo se l'invio va a buon fine 
    if(msgsnd(msgq_server_id,&mymsg1,sizeof(mymsg1)-sizeof(long),0) == -1)
        errExit("Errore nell'invio della msgq\n");

    // devo togliere il long per non inviare troppo e successivamente rischiare di leggere info. sbagliata 
    if(msgsnd(msgq_server_id,&mymsg2,sizeof(mymsg2)-sizeof(long),0) == -1)
        errExit("Errore nell'invio della msgq\n");

    /*creazione della memoria condivisa*/
    // creo la chiave a mano
    key_t shm_server_key = 90;  // 90 chiave per la memoria condivisa

    // creo la memoria condivisa    -> dim[0] == rows && dim[1] == columns
    int shm_server_id = shmget(shm_server_key, sizeof(int)*dim[0]*dim[1], IPC_CREAT | S_IRUSR | S_IWUSR);
    
    /*creo il puntatore al segmento di memoria condivisa che sarà la matrice di gioco*/
    int *shm_table = shmat(shm_server_id,NULL,0);

    //controllo che l'operazione sia andata buon fine
    if (*shm_table == (int) -1)
        errExit("Errore nella creazione della connessione con la memoria condivisa[lato server]\n");
    
    /*creazione dei semafori*/
    // creo la chiave a mano 
    key_t sem_server_key = 91;   // 91 chiave per i semafori 

    // creo il set di semafori (3 semafori in quanto server e 2 client)
    int sem_server_id = semget(sem_server_key,3,IPC_CREAT | S_IRUSR | S_IWUSR);

    if (sem_server_id == -1) 
        errExit("Errore nella creazione del set di semaofri\n");

    /*inizializzo il set di semafori*/
    // creo l'array di valori (x poter sfruttare il campo array della semunum)
    unsigned short initial_sem_values[3] = {0,0,0};

    //  creao la mia union semnum
    union semnum {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;

    arg.array = initial_sem_values;

    // aggiorno il valore dei semafori grazie alla flag SETALL {SETVALL x unico sem}
    if(semctl(sem_server_id,0/*ignored*/,SETALL,arg) == -1)
        errExit("Errore nel inizializzazione dei semafori <server>\n");

    /*a questo punto ho i miei semafori inizializzati protni ad essere usati e la mia mia
    memoria condivisa pronta a essere utilizzata al meglio*/

    /*creo la matrice*/
    int game_matrix[20][20];
    
    //inizializzo la matrice
    inizialize_table(game_matrix,dim[0],dim[1]);

    //copio i dati della matrice nella memoria condivisa 
    for(int i = 0; i < dim[0]; i++) 
        for(int j = 0; j < dim[1]; j++)
            shm_table[i*dim[1]+j] = game_matrix[i][j];

    if(signal(SIGINT,handlerCtrlC) == SIG_ERR)
        errExit("Errore nel cambio del del segnale\n");

    /*il server deve attendere la connessione dei giocatori*/                                                                                              

    char pathFIFO[] = "/tmp/myfifo";
	createFIFO(pathFIFO);


    printf("[SERVER] Starting server...\n");
	
	char client_name[2][50];

    //ridefinisco il segnale 
    if (signal(SIGUSR1,handleBotOption) == SIG_ERR) {
        errExit("Errore nella ridefinizione del segnale bot\n");
    }

	for(unsigned short client_nr = 0; client_nr<2; client_nr++) {

        if (end_game != 1)
            waitForClient(pathFIFO, client_nr, &client_pid[client_nr], client_name[client_nr]);
        // se si sconnette un giocatore muoiono tutti (in fase di connessione)
        else 
            break;
	}

    int sem_server = 0; 
    int win = 0;
    int playerTurn = 1;     // -> parte il primo giocatore, il server cambierà il turno cedendo semaforo corretto
    
    if (end_game != 1) {

        printf("[SERVER] User 1, pid: %d, name: %s\n", client_pid[0], client_name[0]);
        printf("[SERVER] User 2, pid: %d, name: %s\n", client_pid[1], client_name[1]);
        
        printf("<Server> la connessione è avvenuta con successo\n");
        
        /*arbitraggio della partita*/

        printf("<Server> sto avviando la partita...\n");
        // avvio la partita mandando il segnale al client
        kill(client_pid[0],SIGUSR1);

        if (bot_optz == 1)
            kill(client_pid[1],SIGTERM);    // non funziona con ISGUSR1 nel pc del lab
        else 
            kill(client_pid[1],SIGUSR1); 
        
        /*
        if(signal(SIGUSR2,handleCtrlClient) == SIG_ERR)
            errExit("Errore nel cambio del segnale(client)\n");
        */

        printf("<Server> partita avviata con successo!\n");

    }
    
    while(!end_game) {

        // sblocco un giocatore 
        semOp(sem_server_id,playerTurn,+1);

        if (cont_ctrlc == 1) {
            semOp(sem_server_id,playerTurn,+1);
        }

        semOp(sem_server_id,sem_server,-1);

        // cotnrollo se la partita è terminata in quanto ho preso due SIGINT
        if(end_game == 1) 
            break;

        // controllo se qualcuno ha vinto aggiornando la matrice copiandola dalla memoria condivisa
        for(int i = 0; i < dim[0]; i++) 
            for(int j = 0; j < dim[1]; j++)
                game_matrix[i][j] = shm_table[i * dim[1] +j];

        // la check_win vuole il player turn -> attualmente verifico se c'è una vittoria con entrambi i simboli 
        win = check_win(game_matrix,1/*symbol_player*/,dim[0],dim[1]);

        if (win != 1) 
            win = check_win(game_matrix,2/*symbol_player*/,dim[0],dim[1]);
        
        if (win == 1) {
            // finita la partita quindi rimuovo tutto 
            printf("<Server> uno dei due giocatori ha vinto, fine del gioco...\n");
            // invio il segnale ai due giocatori che la partita si è conclusa 
            kill(client_pid[0],SIGUSR2);
            kill(client_pid[1],SIGUSR2);

            /*rimuovere le connessioni create*/
            printf("<Server> sta eliminando le connessioni...\n");
            if(shmdt(shm_table) == -1)
                errExit("Errore nel eliminazione della connessione\n");

            /*eliminare la memoria condivisa*/
            printf("<Server> sta eliminando la memoria condivisa...\n");
            if(shmctl(shm_server_id,0/*ignored*/,IPC_RMID) == -1)
                errExit("Errore nell'eliminazione della memoria condivisa\n");

            /*eliminare il set di semafori*/
            printf("<Server> sta eliminando il set di semafori...\n");
            if(semctl(sem_server_id,0,IPC_RMID,NULL) == -1)
                errExit("Errore nella rimozione del set dei semaofri nel server\n");
        
            /*eliminare la msq*/
            printf("<Server> sta elimnando la msgq...\n");
            if(msgctl(msgq_server_id,0,IPC_RMID) == -1)
                errExit("Errore nella rimozione della msgq\n");
            
            /*eliminare la fifo*/
            printf("<Server> sta eliminando la fifo...\n");
            if(remove("/tmp/myfifo") == -1)
            errExit("Errore nella rimozione della fifo\n");
        
            exit(0);    // fine del gioco

        }
        else if (win != 1) {
            printf("<Server> Nessuno ha ancora vinto quindi aspetterò ancora un po...\n");
            // se siamo qui non ha vinto nessuno quindi la partita deve continuare 
            playerTurn = (playerTurn % 2) + 1; // cambio turno
        }
    }

    // qui arriviamo solo se end_game == -1
    // mando il segnale ai giocatori che la partita è finita
    kill(client_pid[0],SIGUSR2);
    kill(client_pid[1],SIGUSR2);

    //rimuovere le connessioni create
    printf("<Server> sta eliminando le connessioni...\n");
    if(shmdt(shm_table) == -1)
        errExit("Errore nel eliminazione della connessione\n");

    //eliminare la memoria condivisa
    printf("<Server> sta eliminando la memoria condivisa...\n");
    if(shmctl(shm_server_id,0/*ignored*/,IPC_RMID) == -1)
        errExit("Errore nell'eliminazione della memoria condivisa\n");

    //eliminare il set di semafori
    printf("<Server> sta eliminando il set di semafori...\n");
    if(semctl(sem_server_id,0,IPC_RMID,NULL) == -1)
        errExit("Errore nella rimozione del set dei semaofri nel server\n");
    
    /*eliminare la msq*/
    printf("<Server> sta elimnando la msgq...\n");
    if(msgctl(msgq_server_id,0,IPC_RMID) == -1)
        errExit("Errore nella rimozione della msgq\n");

    /*eliminare la fifo*/
    printf("<Server> sta eliminando la fifo...\n");
    if(remove("/tmp/myfifo") == -1)
        errExit("Errore nella rimozione della fifo\n");

    return 0;
}

void errExit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void waitForClient(char* pathFIFO, unsigned short client_nr, pid_t* pid, char* name) {

    struct bufFIFO buf; 
    readFIFO(pathFIFO, &buf, sizeof(buf),&end_game);
	strcpy(name, buf.str1);
	*pid = buf.pidSender;  
    
}

void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    /* devo fare così altrimenti la semop va avanti se fallisce*/
    int ret;
    do {
        ret = semop(semid, &sop, 1);
    } while (ret == -1 && errno == EINTR && end_game == 0);   
}

void handlerCtrlC(int sig) {

    if (sig == SIGINT && cont_ctrlc == 0) {
        printf("<Server> ATTENZIONE SE PREMI ANCORA Ctrl+C LA PARTITA SI CHIUDE\n");
        cont_ctrlc++;
    }
    else if (sig == SIGINT && cont_ctrlc == 1) {
        end_game = 1;
        printf("<Server> la partita è stata forzatamente chiusa\n");
    }
}

void handleCtrlClient(int sig) {
    if (sig == SIGUSR2) {
        end_game = 1;
        printf("<Server> rimozione per abbandono di un giocatore\n");
    }
}

void handleBotOption(int sig) {
    if (sig == SIGUSR1) {
        
        bot_optz = 1; // flag

        pid_t pid;

        pid = fork();

        if (pid == -1) {
            errExit("Fork error\n");
        }
        else if (pid == 0) {
            // figlio che esegue un client
            execl("F4client","./F4client","*", NULL);   
            perror("Execl failed\n");            
            _exit(0);
        }      
        
    }
}




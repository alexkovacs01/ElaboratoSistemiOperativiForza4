#include "all_lib.h"
#include "my_own_library_final.h"
#include "../inc/fifo.h"
#include <signal.h>
#include <time.h>

struct myMsg {
    long mtype; /* tipo di messaggio {obbligatorio} */ 
    int PlayerNumber;   // -> numero del giocatore
    int dim1;   // -> righe tabelle
    int dim2;   // -> colonne tabelle 
    char token1;
    char token2;
    pid_t server_pid;
};


void handleStartSingal(int signal);
void errExit(const char *msg);
void semOp (int semid, unsigned short sem_num, short sem_op);
void handleEndGameSingal(int signal);
void handleAlarm(int sig);

int gameStarted = 0;
int playerTurn = 0; // 0 per giocatore 1, 1 per giocatore 2
int end_flag = 0;
pid_t copy_server_pid;

int main(int argc, char *argv[]) {
    
    // controllo che l'inserimento avvenga come da specifiche 
    if (argc > 3) {
        printf("UTILIZZO SCORRETTO\n");
        printf("Inserisca ./F4client nomegiocatore\n");
        exit(-1);
    }

    // time_t tempo;   // mi serve per capire quanto tempo sia passato 

    char *username;
    char *auto_bot;
    username = argv[1];
    auto_bot = argv[2];

    /*MSGQ*/
    // chiave della msgq
    key_t msgq_key = 92;

    // creazione msgq
    int msgq_id = msgget(msgq_key, IPC_EXCL | S_IRUSR | S_IWUSR);

    // vefiica 
    if (msgq_id == -1)
        errExit("Partita non esitente\n");

    struct myMsg mymsg;

    // 9 è mytpe {tipo di player}
    if(msgrcv(msgq_id,&mymsg,sizeof(mymsg)-sizeof(long),9,IPC_NOWAIT) == -1)  {
        if (errno == ENOMSG)    // -> posso captare varie flag 
            errExit("Partita piena\n");
        else 
            errExit("Errore nella ricezione del messaggio della msgq\n");
    }

    int dim1 = mymsg.dim1;
    int dim2 = mymsg.dim2;

    copy_server_pid = mymsg.server_pid;

    /*significa che voglio giocare contro un bot*/
    if (auto_bot != NULL && (strcmp(auto_bot,"*") == 0)) {  // per eseguirlo allora dobbiamo scrivere \* nel client 
        printf("Ho inviato il segnale al server per botgame\n");
        // informo il server di questa cosa (questa cosa succede all'inizio della partita)
        kill(copy_server_pid,SIGUSR1);  // Uso SIGUSR1 per dire di giocare contro il bot
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

    char name[strlen(argv[1])];
    strcpy(name,argv[1]);
	strcpy(buf.str1, name);
    

	printf("[DEBUG] [CLIENT] message send, pid: %d, messageType: %d, str1: %s", buf.pidSender, buf.messageType, buf.str1);
	
    signal(SIGUSR1,handleStartSingal);
    signal(SIGTERM,handleStartSingal);  // utilizzo non consono
    
    writeFIFO(pathFIFO, &buf, sizeof(buf));

    //signal(SIGUSR2,handleAspetta);

    // sono il bot
    if (getppid() == copy_server_pid)
        signal(SIGINT,SIG_IGN);
    else if ((signal(SIGINT,handleEndGameSingal) == SIG_ERR))
            errExit("Errore nella ricezione del sengale\n");
    
    while(!gameStarted){
        pause();    // -> Mi metto in attesa per l'inizio della partita 
    }

    printf("[DEBUG] [CLIENT] Game started!\n");
    printf("Welcome %s\n", username);
    
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

    int mat_copy[20][20];

    // provo inizializzando anche qui la matrice 
    inizialize_table(mat_copy,20,20);   // la inizializzo tutta
    
    // accedo al set di semafori per sincronizzare la partita 
    key_t sem_key = 91;
    int sem_id = semget(sem_key,3,0);
    if (sem_id == -1)
        errExit("Errore nell'acquisizione dei semafori da parte del client\n");

    // Indici dei semafori nel set
    int serverSemaphore = 0;
    
    if (signal(SIGUSR2,handleEndGameSingal) == SIG_ERR)
            errExit("Errore nella ricezione del sengale\n");
    /*
    if (signal(SIGINT,handleEndGameSingal) == SIG_ERR)
            errExit("Errore nella ricezione del sengale\n");
    */
    
    int ins_flag = 0;   // -> controllo se non si è verificato un pareggio

    while(1) {

        // blocco il semaforo del giocatore corrente
        semOp(sem_id,mymsg.PlayerNumber,-1);

        // aggiorno la copia della mia tabella
        for(int i = 0; i < dim1; i++) 
            for(int j = 0; j < dim2; j++)
                mat_copy[i][j] = shm_ptr[i*dim2+j];

        /*operazione del giocatore*/
        printf("\n<Client> ecco il campo di gioco: \n");
        
        // stampo le matrici a video
        // print_table(mat_copy,dim1,dim2); // -> matrice con valori veri
        print_fake_table(mat_copy,dim1,dim2,1,2,mymsg.token1,mymsg.token2);
        
        // modifica della matrice in memoria condivisa 
        // il turno+1 coincide col simbolo per la vittoria
        printf("<Client> inserisci: \n");

        /*controllo se sono stato eseguito dal server o meno*/
        if (getppid() == copy_server_pid) {
            //printf("GIOCO CONTRO BOT\n");
            ins_flag = insert_in_table(mat_copy,mymsg.PlayerNumber,mymsg.PlayerNumber,1/*vs bot*/,dim1,dim2);
        } else {


            alarm(5);   // dopo 5 sec vieni disconnesso

            signal(SIGALRM,handleAlarm);

            ins_flag = insert_in_table(mat_copy,mymsg.PlayerNumber,mymsg.PlayerNumber,0/*no vs bot{1x gicare contro il bot}*/,dim1,dim2);
            
            alarm(0);

            /*  // versione che non utilizza allarm e non disconnette in automatico il giocatore, ricordati di deccomentare il tempo
            tempo = time(0);

            ins_flag = insert_in_table(mat_copy,mymsg.PlayerNumber,mymsg.PlayerNumber,0 no vs bot{1x gicare contro il bot} ,dim1,dim2);

            // se l'inserimento del giocatore è durato troppo allora lo sconnetto
            if ((tempo - (time(0))) < -5) {
                printf("<Client> sei stato sconnesso per time-out\n");
                kill(getpid(),SIGINT);  // mi chiudo 
            }
            */
            
        }

        // se l'ins_flag == -1 si è verificato un pareggio bisognerà mandare kill al server
        
        if (ins_flag == -1) {
            printf("<Client> Si è verificato un pareggio, sconnetto il giocatore\n");
            kill(copy_server_pid,SIGUSR2);
        }    

        // aggiorno la tabella in memoria condivisa
        for(int i = 0; i < dim1; i++) 
                for(int j = 0; j < dim2; j++)
                    shm_ptr[i*dim2+j] = mat_copy[i][j];
        
        // sblocco il server che esegue l'operazione di controllo della vittoria 
        semOp(sem_id,serverSemaphore,1);

    }
    
    return 0;
}

void handleStartSingal(int signal) {

    gameStarted = 1; // inizio della partita

}

void handleEndGameSingal(int signal) {
    if(signal == SIGUSR2) {
        printf("<Server> cari giocatori partita finita\n");
        exit(0);
    }  
    else if (signal == SIGINT) {
        // devo comunque terminare qui la partita ma inviare anche al server che uno dei due ha abbandonato
        end_flag = 1;
        printf("<Client> hai abbandonato la partita, hai perso!\n");
        fflush(stdout);
        kill(copy_server_pid,SIGUSR2);
        exit(0);
    }
}

void errExit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// da rendere come esterna dato che la uso sia nel client che nel server
void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1) {
        if (errno == EINTR) {
            kill(copy_server_pid,SIGINT);   // mando l'interrupt anche al server
            exit(0);
        } 
        else if (errno == EINVAL) {   // per il primo giocatore
            exit(0);
        }
        else {
            errExit("SemOp failed(nel client)\n");
        }
    }   
}

void handleAlarm(int sig) {
    printf("<Client> sei stato sconnesso per time-out!!!\n");
    kill(getpid(),SIGINT); // mi chiudo
}
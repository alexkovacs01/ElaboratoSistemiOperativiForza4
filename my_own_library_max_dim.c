#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "my_own_library_final.h"

/*l'idea è quella di passare come parametri la dimensione effettiva della matrice*/

void inizialize_table(int mat[n][m], int dim1, int dim2) {
    for(int i = 0; i < dim1; i++) 
        for(int j = 0; j < dim2; j++)
            mat[i][j] = 3;     // non posso inizializzare la matrice a 0 se no si bugga 
}   

void print_table(int mat[n][m],int dim1, int dim2) {
    for(int i = 0; i < dim1; i++) {
        for(int j = 0; j < dim2; j++) {
            printf("[%i]", mat[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_fake_table(int mat[n][m],int dim1,int dim2,int player1,int player2,char token1,char token2) {

    for (int i = 0; i < dim1; i++) {
        for (int j = 0; j < dim2; j++) {
            if (mat[i][j] == player1){
                // print token1
                printf("[%c]", token1);
            }
            else if(mat[i][j] == player2) { // -> else crea problemi per matrice vuota 
                // print token2
                printf("[%c]", token2);
            }
            else {
                printf("[ ]");
            }
        }
        printf("\n");
    }


}

int check_win(int mat[n][m], int symbol_player, int dim1, int dim2) {

    int win;
        
    win = check_vertical_win(mat,symbol_player, dim1, dim2);
    
    if (win != 1) {
        win = check_orizzontal_win(mat,symbol_player, dim1, dim2);
    } 
    
    if (win == 1) 
        printf("#GIOCATORE %i#\n", symbol_player);

    return win;


}
/*funziona*/
int check_vertical_win(int mat[n][m],int symbol_player, int dim1, int dim2){
   
    int row, cont;
    // int column; ->unused 
    
    // check for row
    for(int i = 0; i < dim1; i++) {
        //cont = 0;
        for(int j = 0; j < dim2; j++) {
            cont = 0;
            if (mat[i][j] == symbol_player) {
                row = i;
                while ((mat[row][j] == symbol_player) && (row < dim1)) {
                    //printf("%i->\n", symbol_player);
                    cont++; // ho trovato un simbolo
                    // qui mi pare strano che mi segna sempre come colum 0
                    //printf("--->[%i][%i]->cont:%i ,%i\n",row,j,cont, mat[row][j]);
                    if (cont == 4) {
                        // hai vinto; 
                        printf("->%i\n", cont);
                        return 1;
                    }
                    row++;      
                }
            }
        }
    }
    
    return -1;  // Nessuna vittoria verticale

}
/*funziona*/
int check_orizzontal_win(int mat[n][m],int symbol_player, int dim1, int dim2) {
    int cont, column; 
    
    // check for row
    for(int i = 0; i < dim1; i++) {
        //cont = 0;
        for(int j = 0; j < dim2; j++) {
            cont = 0;
            if (mat[i][j] == symbol_player) {
                column = j;
                while ((mat[i][column] == symbol_player) && (column < dim2)) {
                    //printf("%i->\n", symbol_player);
                    cont++; // ho trovato un simbolo
                    // qui mi pare strano che mi segna sempre come colum 0
                    //printf("--->[%i][%i]->cont:%i ,%i\n",row,column,cont, mat[row][j]);
                    if (cont == 4) {
                        // hai vinto; 
                        printf("->%i\n", cont);
                        return 1;
                    }
                    column++;      
                }
            }
        }
    }
    
    return -1;
}

int check_parity(int mat[n][m], int symbol_player, int dim2) {
    int cont = 0;
    for(int j = 0; j < dim2; j++) {
        if ((mat[0][j] == 1) || (mat[0][j] == 2)) {
            cont++;
            if (cont == 5)
                return -1;
        }
    }
    return 1;
}

int insert_in_table(int mat[n][m], int player, int symbol_player, int bot , int dim1, int dim2) {

    int column, choose, parity;

    printf("Inserisci il numero della colonna che desideri riempire\n");
    do {
        if (bot == 0) {
            /*non permetto di inserire in colonne non esistenti*/
            do {
                scanf("%i", &column);
            } while ((column < 0 || column > dim2-1));
        }
        else{   // è stata scelta l'opzione di giocare contro il bot
            srand(time(NULL));
            column = rand() %5 +1;
            bot--;
        }

        /*controllo la scelta che è stata fatta se non eccede i limiti*/
        choose = is_aveilable_column(mat,column,symbol_player,dim1);
        /*se eccede chiaramente stampo avideo*/
        if (choose == -1) 
            printf("La colonna che hai scelto è gia piena\n");
        parity = check_parity(mat,symbol_player,dim2);
        if (parity == -1)
            return -1;
    } while(choose != 1);   /*se eccede i limiti ripeto l'inserimento*/

    /*qui bisogna stare attendi se dim1 > dim2 ecc -> questo if non è servito a cazzo niente*/
    /*inserisco all'interno della matrice*/
    if (dim2 > dim1) {
        for (int i = dim1-1; i >= 0; i--) {    // ecco il bug vado a dire n-1 ovvero dimensione righe-1
            if (mat[i][column] == 3 || mat[i][column] == 0) { // bug con l'inserimento se qui a 0
                mat[i][column] = symbol_player;
                printf("Debug pos:[%i][%i]\n", i,column);
                break;
            }
        }
    }else {
        for (int i = dim1-1; i >= 0; i--) {    // ecco il bug vado a dire n-1 ovvero dimensione righe-1
            if (mat[i][column] == 3) { // bug con l'inserimento se qui a 0
                mat[i][column] = symbol_player;
                break;
            }
        }
    }

    return 1;
}

int is_aveilable_column(int mat[n][m], int column, int symbol_player, int dim1) {
    int cont = 0; 
    /*scansiono solo le righe tenendo la colonna ferma verificando che siano occupate*/
    for(int i = 0; i < dim1; i++) { 
        if ((mat[i][column] == 1) || (mat[i][column] == 2)) {
            /*ciascun spazio occupato lo conto quando è del tutto occupato ritorno -1*/
            cont++;
            //printf("AVDEBUG->%i,col:%i\n", cont, column);
            if (cont == dim1)
                return -1;
        }
    }
    return 1;
}



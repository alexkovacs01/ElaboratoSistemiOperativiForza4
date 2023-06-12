#define n 20
#define m 20

void inizialize_table(int mat[n][m] ,int dim1 ,int dim2);
void print_table(int mat[n][m],int,int);
void print_fake_table(int mat[n][m],int,int,int,int,char,char);
int insert_in_table(int mat[n][m], int,int,int,int,int);
int check_win(int mat[n][m],int,int,int);
int check_vertical_win(int mat[n][m],int,int,int);
int check_orizzontal_win(int mat[n][m],int,int,int);
int check_parity(int mat[n][m], int,int);
int is_aveilable_column(int mat[n][m],int,int,int);


#include <mpi.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#define CANDIDATE 1 //kandydat
#define ELECTED 2 //wybrany

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	int size,rank;
	char separator[] = "\033[0;36m================================================================\033[0;m \n";

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Request request;
	MPI_Status status;

    	char processor_name[MPI_MAX_PROCESSOR_NAME];
    	int name_len;
    	MPI_Get_processor_name(processor_name, &name_len);

	//bufory do wysylania i odbierania UID
	int uid, recv;

	//kazdy proces generuje unikalny UID - dlatego +rank, bo kazdy proces musi generowac rozny seed
	srand(time(NULL)+rank);
	uid= rand() %1000000;

	printf("%s[%s] - [Proces %d] : Generuje \033[1;34mUID= %d\033[0;m. Zaczynam swoja prace..\n",separator,processor_name,rank,uid);

	//kazdy proces wysyla swoje UID do nastepnika
	//aby zachowac topologie pierscienia i nie przekroczyc zakresu procesu stosuje modulo
	MPI_Send(&uid, 1, MPI_INT, (rank+1)%size, CANDIDATE , MPI_COMM_WORLD); 
	printf("%s[%s] - [Proces %d] : Wysylam uid= %d do ziomeczka o id --> %d \n",separator,processor_name,rank, uid, (rank+1)%size);

	while(1){
		//aby procesy nie odbieraly informacji zwrotnej o dotarciu wyslanego komunikatu stosuje komunikacje asynchroniczna!
		//czyli ta funkcja skonczy sie natychmiast, wiec proces moze kontynuowac swoje obliczenia
		MPI_Irecv(&recv,1,MPI_INT, (rank==0 ? size-1 : rank-1), MPI_ANY_TAG, MPI_COMM_WORLD, &request);
		//tutaj proces oczekuje na odebranie wiadomosci, nie przejdzie dalej dopoki nie odbierze
		MPI_Wait(&request, &status);
		printf("%s[%s] - [Proces %d] : Odebralem liczbe %d od procesu %d \n",separator,processor_name, rank ,  recv ,(rank==0 ? size-1 : rank-1) );
			
		//jezeli proces bierze udzial w wyborach czyli status kandydat - poszukiwania lidera
		if (status.MPI_TAG == CANDIDATE) {
                    if (recv>uid) {
				MPI_Send(&recv, 1, MPI_INT, (rank+1)%size, CANDIDATE , MPI_COMM_WORLD);
				
                      		printf("%s[%s] - [Proces %d] : \033[1;32m[Recv>UID] ==> Recv=%d, UID= %d, czyli wysylam dalej do %d ...\033[0;m\n", separator,processor_name,rank, recv, uid, (rank+1)%size);		
                    }

		    else if (recv==uid) {
				printf("%s[%s] - [Proces %d] : \033[1;33m[Recv=UID] Oho!! Wlasnie dotarla do mnie wiadomosc ze jestem procesem o najwiekszym UID!\033[0;m\n",separator,processor_name,rank);

				//jestem liderem wiec wysylam teraz wiadomosc do nastepnika ze to ja wygralem elekcje - zmiana tagu kolejnego procesu na ELECTED
				MPI_Send(&recv, 1, MPI_INT, (rank+1)%size, ELECTED , MPI_COMM_WORLD);
				
				printf("%s[%s] - [Proces %d] : \033[1;33mLIDER WYSYLA DO %d info o tym ze on wygral i jest liderem\033[0;m\n",separator,processor_name,rank,(rank+1)%size);

				//wiadomosc o tym ze wygralem powraca do lidera ktory ustawia rowniez stan elected i oglasza sie zwyciezca
				MPI_Recv(&recv,1,MPI_INT, (rank==0 ? size-1 : rank-1), ELECTED , MPI_COMM_WORLD, &status);
				
                                 printf("%s[%s] - [Proces %d] : \033[1;33m OHO WIADOMOSC DO MNIE POWROCILA CZYLI WSZYSCY JUZ WIEDZA ZE JESTEM LIDEREM!\033[0;m\n",separator,processor_name,rank);
						break;
                                 
                   }
		   else if (recv< uid) {
			printf("%s[%s] - [Proces %d] : \033[1;31m[Recv<UID] - Recv=%d, UID= %d ->> Nie robie nic... \033[0;m\n", separator,processor_name ,rank, recv, uid); // nic nie robimy, jest to wazne bo jezeli kilka procesow na raz rozpoczyna elekcje to moze dojsc do zakleszczenia                         
			}                    

		}
		//status = wybrany - czyli znaleziono lidera
	 if (status.MPI_TAG == ELECTED) {
			printf("%s[%s] - [Proces %d] : Otrzymalem wlasnie informacje zwrotna o tym ze %d wygral xd\n", separator ,processor_name,rank , recv);
                     	printf("%s[%s] - [Proces %d] : No coz w takim razie rozsylam tego newsa dalej do %d\n", separator ,processor_name, rank , (rank+1)%size);
			MPI_Send(&recv, 1, MPI_INT, (rank+1)%size, ELECTED , MPI_COMM_WORLD);
                     break;
               }

	}
	    
    MPI_Finalize();
}

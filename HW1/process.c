#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <signal.h>

#include <time.h>

#define MAX_BUTTON 9
#define KEY_PRESS 1
#define KEY_RELEASE 0

#define FND_START 12
#define STR_START 16
int mode1swbutton(int* swbutton, struct tm *tm_ptr, unsigned char* fndData);
void mode2swbutton(int* swbutton, unsigned char* fndData, int *number);
void modesInit(int mode, struct tm *tm_ptr, unsigned char* fndData, int *ledstat, int *dotMatrix);
void mode1Init(struct tm **tm_ptr);
void mode2Init(unsigned char *fndData);
void mode3swbutton(int* swbutton, unsigned char *string, int *strIndex, int* dotMatrix, int *cnt);

int main(){
  int inputProcId, outputProcId;
  key_t key;
  int shmid;
  int *shmaddr = NULL;
	char shmidChar[256];

  key = ftok("/data", 1);
  shmid = shmget(key, 1024, IPC_CREAT|0644);
	sprintf(shmidChar, "%d", shmid);
  if( shmid == -1 ){
    perror("shmget");
    exit(1);
  }

  if( (inputProcId = fork()) < 0 ){
    perror("fork error");
    exit(1);
  }
  else if( inputProcId == 0 ){ /* -MARK: input Process */
		char *argv[] = {"./reader", shmidChar, NULL};
		execv(argv[0], argv);
  }
  else{
    int processingChild, status;
		int mode = 1;
		struct tm *tm_ptr;

		unsigned char fndData[4]={0};
		unsigned char string[32] = {"Hello           World          "};
		int strIndex = 0;
		int i;
		int ledstat=0, dotMatrix=0;
		

		mode1Init(&tm_ptr);

    shmaddr = (int*)shmat(shmid, (int*)NULL, 0);
		/*for(i=0; i<4; i++){
			shmaddr[ FND_START +i] = fndData[i];
		}
		for(i=0; i<32; i++){
			shmaddr[ STR_START +i] = string[i];
		}*/


    if( (outputProcId = fork()) < 0 ){
      perror("fork error");
      exit(1);
    }
    else if( outputProcId == 0 ){ /* -MARK: output Process */
			char *argv[] = {"./writer", shmidChar, NULL};
			execv(argv[0], argv);
    }

		/* -MARK: main Process */
    while(1){
      int pressedKey = shmaddr[0];
			int swbutton[9];
      usleep(400000);
			for(i=0; i<MAX_BUTTON; i++)
				swbutton[i] = shmaddr[1+i];

			printf("press %d %d %d\n ", pressedKey, mode, ledstat);
      if( pressedKey == 158 ){
				/* Quit */
        break;
      }
      else if( pressedKey == 116 ){
      }
      else if( pressedKey == 115 ){
				/* Mode Change + */
				mode = (mode+1)%4;
				modesInit(mode, tm_ptr, fndData, &ledstat, &dotMatrix);
      }
      else if( pressedKey == 114 ){
				/* Mode Change - */
				mode--;
				if( mode < 0 )
					mode =  4;
				modesInit(mode, tm_ptr, fndData, &ledstat, &dotMatrix);
      }

			if( mode == 1 ){
				ledstat = mode1swbutton(swbutton, tm_ptr, fndData);
			}
			else if( mode == 2 ){
				mode2swbutton(swbutton, fndData, &ledstat);
			}
			else if( mode == 3 ){
				mode3swbutton(swbutton, string, &strIndex, &dotMatrix, &ledstat);
			}

			shmaddr[10] = mode;
			shmaddr[11] = ledstat;
			for(i=0; i<4; i++){
				shmaddr[ FND_START +i] = fndData[i];
			}
			for(i=0; i<32; i++){
				shmaddr[ STR_START +i] = string[i];
			}
			shmaddr[48] = dotMatrix;
    }

    //prctl(PR_SET_PDEATHSIG, SIGTERM);
		kill(inputProcId, SIGTERM);
		kill(outputProcId, SIGTERM);

    /* waiting for running child*/
    while( (processingChild = wait(&status)) > 0 );
  }
	shmdt((int*)shmaddr);
	shmctl(shmid, IPC_RMID, (struct shmid_ds*)NULL);
	return 0;
}


int mode1swbutton(int* swbutton, struct tm *tm_ptr, unsigned char* fndData){
	static int isChange = 0;
	if( swbutton[0] == KEY_PRESS ){
		isChange = !isChange;
	}

	if( isChange == 1 ){
		if( swbutton[1] == KEY_PRESS ){
			mode1Init(&tm_ptr);
		}
		if( swbutton[2] == KEY_PRESS ){
			tm_ptr->tm_hour++;
		}
		if( swbutton[3] == KEY_PRESS ){
			tm_ptr->tm_min++;
		}
		if(tm_ptr->tm_min >= 60 )
			tm_ptr->tm_hour++;
		if( tm_ptr->tm_hour >= 24 )
			tm_ptr->tm_hour = tm_ptr->tm_hour % 24;
		
	}
	fndData[0] = tm_ptr->tm_hour/10; fndData[1] = (tm_ptr->tm_hour)%10;
	fndData[2] = tm_ptr->tm_min/10; fndData[3] = (tm_ptr->tm_min)%10;

	return isChange;
}

void mode1Init(struct tm **tm_ptr){
	time_t t = time(NULL);
	*tm_ptr = localtime(&t);
}

void mode2swbutton(int* swbutton, unsigned char* fndData, int *number){
	int divider = *number;
	if(*number == 0 ){
		printf("divider is zeor\n");
		return ;
	}

	if( swbutton[0] == KEY_PRESS ){
		int origin=0;

		origin += fndData[1] * divider * divider;
		origin += fndData[2] * divider;
		origin += fndData[3];

		if( *number == 10 )
			*number = 8;
		else if( *number == 8 )
			*number = 4;
		else if( *number == 4)
			*number = 2;
		else
			*number = 10;

		divider = *number;
		origin %= (divider*divider*divider);
		fndData[1] = origin/(divider*divider);
		origin %= (divider*divider);
		fndData[2] = origin/divider;
		origin %= divider;
		fndData[3] = origin;
	}
	if( swbutton[1] == KEY_PRESS ){
		fndData[1] = fndData[1]%(*number);
	}
	if( swbutton[2] == KEY_PRESS ){
		fndData[2] = fndData[2]+1;
		fndData[1] += fndData[2]/(*number);
		fndData[1] = fndData[1]%(*number);
		fndData[2] = fndData[2]%(*number);
	}
	if( swbutton[3] == KEY_PRESS ){
		fndData[3]++;
		fndData[2] += fndData[3]/(*number);
		fndData[1] += fndData[2]/(*number);
		fndData[1] = fndData[1]%(*number);
		fndData[2] = fndData[2]%(*number);
		fndData[3] = fndData[3]%(*number);
	}
}

void mode2Init(unsigned char *fndData){
	memset(fndData, 0, sizeof(unsigned char)*4);
}

void modesInit(int mode, struct tm *tm_ptr, unsigned char* fndData, int *ledstat, int *dotMatrix){
	*ledstat = 0;
	memset(fndData, 0, sizeof(unsigned char)*4);
	if( mode == 1)
		mode1Init(&tm_ptr);
	else if( mode == 2 ){
		//mode2Init(fndData);
		*ledstat = 10;
	}
	else if( mode == 3 ){
		*dotMatrix = 1;
	}
}

void mode3swbutton(int* swbutton, unsigned char *string, int *strIndex, int* dotMatrix, int *cnt){
	int index1=-1, index2=-1;
	int i;
	static int clickCnt = 0;
	static int clickedBefore = -1;

	char charTable[9][3] = 
	{{'.','Q','Z'}, {'A', 'B', 'C'}, {'D', 'E', 'F'},
		{'G','H','I'}, {'J','K','L'}, {'M','N','O'},
		{'P','R','S'}, {'T','U','V'}, {'W','X','Y'}};

	for(i=0; i<9; i++){
		if( swbutton[i] != 0 ){
			if( index1 == -1 )
				index1 = i;
			else
				index2 = i;
		}
	}

	//printf("ins1 %d ins2 %d\n", index1, index2);

	if( index1 == 1 && index2 == 2 ){
		/* pressed sw(2), sw(3) */
		/* clear string					*/
	}
	else if( index1 == 4 && index2 == 5 ){
		/* pressed sw(5), sw(6) */
		/* change input mode, character or number */
		if( *dotMatrix == 1 )
		 	*dotMatrix = 10;
		else
			*dotMatrix = 1;
	}
	else if( index1 == 7 && index2 == 8 ){
		/* pressed sw(8), sw(9) */
		/* make space 					*/
	}
	else{
		if( index1 != -1 ){
			char c;

			if( index1 != clickedBefore ){
				clickedBefore = index1;
			}
			else {
				clickCnt++;
				clickCnt = clickCnt % 3;
			}
		
			c	= charTable[index1][clickCnt];
			string[*strIndex] = c;
			(*cnt)++;
			(*strIndex)++;
		}
	}



}

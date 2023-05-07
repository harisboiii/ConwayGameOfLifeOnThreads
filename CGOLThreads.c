#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

//default dimensions of the board 
#define DWIDTH	80
#define DHEIGHT	25

//define renew time in us
#define RTIME	500000 

//boards dimension values
unsigned int width;
unsigned int height;

//number of threads
unsigned int nthreads;

//array for thread ids
int *t_ids;

//barrier variable
pthread_barrier_t barr;

//program has two arrays in order to save each state
//the needed memory is allocated in main due to dimension values
int **array1;
int **array2;

//pointers to handle arrays (current and next states, and temp for swap).
int **curptr, **nextptr, **temp;

//flag to specify default dimension mode
unsigned int dflag;

//input filename
char *filename;

//struct variables to measure execution time
struct timeval t_start, t_end;

//init board with zeroes if default mode, or with random 0,1.
void initialize_board (int **current, unsigned int dflag);

//read the initial state of the board from a file in default mode
void read_file (int **current, char *file_name);

//bounds remain unchanged we copy to the next state array.
void copy_region (int **current, int **next);

//function to return the number of alive neighbors of the i,j cell.
int adjacent_to (int **current, int i, int j);

//takes two pointers for the arrays
void play (int **current, int **next, int start, int finish);

//threads' entry function (calculate raw bounds and play game).
void *entry_function(void *ptr);

//print the state of the board to standard output
void print (int **current);

//argument check
int arg_check (int argc, char *argv[]);

//print call usage
void print_help (void);

int main (int argc, char *argv[])
{	
	int i;
	dflag=1;
	height=0;
	width=0;
	filename=NULL;

	//default number of threads is 1
	nthreads=1;

	//call argument check
	if(arg_check(argc, argv)!=0)
		return -1;

	//allocate the two arrays due to the given dimensions
	array1 = (int **)malloc(width*sizeof(int *));
	if(array1==NULL)
	{
		printf("Problem in memory allocation of array1!\n");
		printf("Program exited.\n");
		return -1;
	}
	for(i = 0; i < width; i++)
	{
		array1[i] = (int *)malloc(height*sizeof(int));
		if(array1[i]==NULL)
		{
			printf("Problem in memory allocation of array1!\n");
			printf("Program exited.\n");
			return -1;
		}
	}

	array2 = (int **)malloc(width*sizeof(int *));
        if(array2==NULL)
        {
                printf("Problem in memory allocation of array2!\n");
                printf("Program exited.\n");
                return -1;
        }
        for(i = 0; i < width; i++)
        {       
                array2[i] = (int *)malloc(height*sizeof(int));
                if(array2[i]==NULL)
                {
                        printf("Problem in memory allocation of array2!\n");
                        printf("Program exited.\n");
                        return -1;
                }
        }

	//init pointers to show to the relevant arrays
	curptr=array1;
	nextptr=array2;

	//init current array with zeroes or random values by default flag
	initialize_board(curptr, dflag);
	
	//if we have default dimensions read input state from file
	if(dflag)
		read_file (curptr, filename);
	else if(!dflag)
	{
	//if user input dimensions check if they are too big to print in console
		if((width > 100) || (height > 50))
		{
			printf("WARNING!!!! Too large board to print in screen.\n");
			printf("Maximum allowed dimensions is 100 x 50.\n");
			printf("Program exited!\n\n");
			return -1;
		}
	}

	//copy the unchanged region cells
	copy_region(curptr, nextptr);
	
	//create an array with threads given the input number
	pthread_t thr[nthreads];

	//allocate memory for the thread ids
	t_ids = malloc(nthreads * sizeof(int));
	if(t_ids==NULL)
	{
		printf("Problem in memory allocation of ids array!\n");
		printf("Program exited.\n");
		return -1;
	}

	//barrier initialization
    	if(pthread_barrier_init(&barr, NULL, nthreads))
    	{
        	printf("Could not create a barrier\n");
        	return -1;
    	}

	//create the threads
	for(i = 0; i < nthreads; i++)
    	{
		t_ids[i]=i;
        	if(pthread_create(&thr[i], NULL, &entry_function, (void *)&t_ids[i]))
        	{
            		printf("Could not create thread %d\n", i);
            		return -1;
        	}
    	}

	//master thread waits for the execution of all threads
	for(i = 0; i < nthreads; i++)
    	{
        	if(pthread_join(thr[i], NULL))
        	{
            		printf("Could not join thread %d\n", i);
            		return -1;
        	}
    	}
	return 0;
}

void initialize_board (int **curptr, unsigned int dflag)
{
    //if default flag init with zeroes, else with random 0,1 values
	int i, j;

	if(dflag)
	{
		for (i=0; i<width; i++) for (j=0; j<height; j++) 
			curptr[i][j] = 0;
	}
	else
	{
		for (i=0; i<width; i++) for (j=0; j<height; j++) 
			curptr[i][j] = rand() % 2;
	}
}

void read_file (int **curptr, char *name) 
{
    //default mode read initial state from file
	FILE	*f;
	int	i, j;
	char	s[100];

	f = fopen (name, "r");
	for (j=0; j<height; j++) 
	{

		fgets (s, 100, f);

		for (i=0; i<width; i++) 
		{
			curptr[i][j] = s[i] == 'x';
		}
	}
	fclose (f);
}

void copy_region (int **curptr, int **nextptr)
{
	int i,j;
	for(j=0; j<height; j++)
		for(i=0; i<width; i++)
			if((i==0)|(j==0)|(j==height-1)|(i==width-1))
				nextptr[i][j]=curptr[i][j];
}

int adjacent_to (int **curptr, int i, int j) 
{
	int row, col, count;

	count = 0;

	//examine all the neighbors
	for (row=-1; row<=1; row++) 
		for (col=-1; col<=1; col++)
		{
			//exclude current cell from count
			if (row || col)
				if (curptr[i+row][j+col]) count++;		
			//we don't need to keep counting if the number is >3 (no change in behaviour)		
			if(count>3)
			{//break nested loops
				break;
				break;
			}
		}		
	return count;
}

void play (int **curptr, int **nextptr, int start, int finish) 
{
	int i, j, alive;

	//exclude board region and apply for each cell the game's rules
	for (i=1; i<width-1; i++) for (j=start; j<finish; j++) 
	{
		alive = adjacent_to (curptr, i, j);
		if (alive == 2) nextptr[i][j] = curptr[i][j];
		if (alive == 3) nextptr[i][j] = 1;
		if (alive < 2) nextptr[i][j] = 0;
		if (alive > 3) nextptr[i][j] = 0;
	}
}

void print (int **curptr) 
{
	int i, j;

	for (j=0; j<height; j++) 
	{

		for (i=0; i<width; i++) 
		{
			printf ("%c", curptr[i][j] ? 'x' : ' ');
		}
		printf ("\n");
	}
}

void *entry_function(void *t_id)
{
	int *thread_id=(int*)t_id;

	//calculate the array bounds that each thread will process
	int bound = height / nthreads;
	int start = *thread_id * bound;
	int finish = start + bound;

	int i,bn;

	//exclude extern cells
	if(*thread_id==0) start++;
	if(*thread_id==nthreads-1) finish=height-1;

	//play the game for 100 rounds
	for (i=0; i<100; i++)
	{	
		play (curptr, nextptr, start, finish);
	
		//use of the pthread_barrier_wait subroutine to synchronize multiple threads
        //at a designated barrier. The calling thread is blocked until the required 
        //number of threads have also called the subroutine, at which point the barrier 
        //is reset to its most recent state. The constant PTHREAD_BARRIER_SERIAL_THREAD 
        //is returned to one thread, while 0 is returned to the others.

    		bn = pthread_barrier_wait(&barr);
    		if(bn != 0 && bn != PTHREAD_BARRIER_SERIAL_THREAD)
    		{
        		printf("Could not wait on barrier\n");
        		exit(-1);
   		}
		
		//thread with ID=0 is responsible to swap the pointers and
		//print the board's status in each round.

		if(bn==PTHREAD_BARRIER_SERIAL_THREAD)
		{
			if(i==0)
				{
					system("clear");
					print (curptr);
					printf("------------------------------");
					printf("Board's Initial State");
					printf("-----------------------------\n");
					printf("   \"Set console dimensions at least %dx%d to have a full view of the board.\"",width,height+4);
					printf("\n\t\tRenew every %d us  ",RTIME);
					printf("Hit ENTER to start play!\n");
					getchar();  
					system("clear");
				}	
				else
				{	//swap pointers
					temp=curptr;
					curptr=nextptr;
					nextptr=temp;
					
						print (curptr);
						printf("----------------------------");
						printf("Board's state in round: %d",i);
						printf("---------------------------\n");
						usleep(RTIME);
						system("clear");	
					}
		}

		//ensure that the pointers have been swapped before go to next round
		bn = pthread_barrier_wait(&barr);
    		if(bn != 0 && bn != PTHREAD_BARRIER_SERIAL_THREAD)
    		{
        		printf("Could not wait on barrier\n");
        		exit(-1);
   		}
	}   //end of 100 play rounds for loop
	return 0;
}

int arg_check(int argc, char *argv[])
{
	int opt_char;
	if(argc == 1)
	{
		print_help();
		return -1;
	}
	while ((opt_char = getopt(argc, argv, "n:h:w:f:b")) != -1)
        {
                switch(opt_char)
                {
                        case 'n':
				nthreads=atoi(optarg);
                                break;
                        case 'h':
				height=atoi(optarg);
                                break ;
                        case 'w':
				width=atoi(optarg);
                                break ;
                        case 'f':
				filename=optarg;
                                break ;
			case 'b':
                print_help();
                break;
                }
        }

	if(height!=0 && width==0)
	{
		printf("Give width dimension too!\n");
		printf("Or leave default.\n");
		return -1;
	}
	else if(width!=0 && height==0)
	{
		printf("Give length dimension too!\n");
		printf("Or leave default.\n");
		return -1;
	}
	//if all correct given disable default flag
	else if(width!=0 && height!=0)
	{
		dflag=0;
		if(filename!=NULL)
		{
			printf("\nWARNING!\n");
			printf("Dimensions arguments have overscale default input file\n");
			printf("Hit ENTER to continue.\n");
			getchar();
		}
	}
	//if default flag has not been disabled,
	//assign default dimension to variables
	if(dflag)
	{
		width=DWIDTH;
		height=DHEIGHT;
		
		//in default mode we check for input file for the initial state
		if(filename==NULL)
		{
			printf("In default mode give input file too!\n");
			printf("Program exited!\n\n");
			return -1;
		}
	}
	return 0;
}

//print argument usage
void print_help()
{
        printf("\nUsage:\t ./GOLPT OPTIONS\n");
	printf("\tOPTIONS:\n");
	printf("\t\t-n  Number Of Threads\n");
        printf("\t\t-h  Set Board Height(Raws)\n");
        printf("\t\t-w  Set Board Width(Columns)\n");
	printf("\t\t-f  Input File\n");
}

// Assignment 2
// Q1. Distributed Prime Generation

// Objective
// In this assignment, we will use signal and pipes to implement a distributed prime checking.

// Group Details
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define RAND_LIMIT 30000

// Global Variables

int l[2000],counter,n,k;
int pid[2000],fd1[2000][2],fd2[2000][2],mpid,_mpid;
int num;


// Function to split the generated number by space
char** lexer(char  buffer[],int * cnt)
{
    int i = 1,count  = 0,start = 0,f = 0;
    char ** a = (char**)malloc(1024*sizeof(char*));
    if(buffer[0] != ' ')
        start = 0;
    while(1)
    {
        if((buffer[i] == ' ' || buffer[i] == '\n') && buffer[i-1] != ' ')
        {
            a[count++] = (char*)malloc(256*sizeof(char));
            strncpy(a[count-1],buffer + start,i - start);
            a[count-1][i-start] = '\0';
        }
        else if(buffer[i] != ' ' && buffer[i-1] == ' ')
        {
            start = i;
        }
        if(buffer[i] == '\0')
            break;
        i++;
    }
    a[count] = NULL;
    *cnt  = count;
    return a;
}

// Signal Handler Function handling the available signal sent by the child
void available(int signum,siginfo_t * info, void * old)
{
    int idx = info->si_value.sival_int;
    printf("Process %d is Available now\n",pid[idx]);
    int i;
    char val[2000];
    char ss[2000];
    strcpy(ss,"");
    for(i = 0; i<k; i++)
    {
        sprintf(val,"%d",rand()%30001);
        strcat(ss,val);
        strcat(ss," ");
    }
    ss[strlen(ss)] = '\0';
    printf("Numbers sent to Process %d: %s\n",pid[idx],ss);
    write(fd1[idx][1],ss,2000);
}

// Signal Handler Function handling the busy signal sent by the child
void busy(int signum,siginfo_t * info, void * old)
{
    int idx = info->si_value.sival_int;
    printf("Process %d is busy now\n",pid[idx]);
}


// Prime Number checker
bool isPrime(int number)
{
    int i ;
    if(number == 0 || number ==1)return false;
    for( i = 2 ; i*i <= number ; i++)
        if(number%i == 0)return false;
    return true;
}

// Insert the Number detected as prime in the l array if not there already
void tryInsert(int number)
{
    int i;
    for(i=0; i<counter; i++)
        if(l[i] == number)return ;
    l[counter++] =  number;

}

// main function
int main(int argc, char *argv[])
{
    counter = 0;
    int flag = 0,idx;
    mpid = getpid();
    _mpid = getpid();

    if(argc<3){
        printf("Format: ./a.out N k\nTry Again\n");
        return 0;
    }
    // Get n and k from command line
    n = atoi(argv[1]);
    k = atoi(argv[2]);

    union sigval value;

    // Install Signal handler for handling AVAILABLE signal
    struct sigaction act1;
    act1.sa_flags = SA_SIGINFO;
    act1.sa_sigaction = &available;

    if (sigaction(SIGUSR1, &act1, NULL) == -1)
    {
        perror("sigusr1: sigaction");
        return 0;
    }


    // Install Signal handler for handling BUSY signal
    struct sigaction act2;
    act2.sa_flags = SA_SIGINFO;
    act2.sa_sigaction = &busy;

    if (sigaction(SIGUSR2, &act2, NULL) == -1)
    {
        perror("sigusr2: sigaction");
        return 0;
    }


    int i;

    // Generate k child processes
    for(i= 0; i<k; i++)
    {
        pipe(fd1[i]);
        //fd1p[0] = fd1[0];
        //fd1p[1] = fd1[0];

        pipe(fd2[i]);
        //fd2p[0] = fd2[0];
        //fd2p[1] = fd2[0];

        pid[i] = fork();

        if(pid[i] == 0)
        {
            flag = 1;
            idx = i;
            i = k;
            printf("Entered process %d\n",getpid());

            //setpgid(getpid(),_mpid);

            close(fd1[idx][1]);
            close(fd2[idx][0]);
            sleep(1);
            value.sival_int = idx;
        }
        else if(pid[i]>0)
        {
            close(fd1[i][0]);
            close(fd2[i][1]);
            fcntl(fd2[i][0], F_SETFL, O_NONBLOCK);
            sleep(1);


        }

    }

    if(flag == 1)
    {
        // In a child process
        int ii;
        char ** arr;
        while(1)
        {
            usleep(500000);
            char *ss = (char *)malloc(2000*sizeof(char));
            // Send AVAILABLE signal to parent
            if(sigqueue(mpid,SIGUSR1,value) != 0) printf("Available signal could not be sent\n");

            // Read the integers sent by main through the pipe
            read(fd1[idx][0],ss,2000);
            // Parse them
            arr = lexer(ss,&num);
            // Send BUSY signal to parent
            if(sigqueue(mpid,SIGUSR2,value) != 0) printf("Busy signal could not ne sent\n");

            // Check Primality for the integers recieved
            for( ii = 0; ii<num; ii++)
            {
                if(isPrime(atoi(arr[ii])))
                {
                    char uu[2000];
                    strcpy(uu,(arr[ii]));
                    strcat(uu," ");
                    write(fd2[idx][1],uu,2000);
                }
            }
            ss = NULL;
        }

    }
    else
    {
        // In Parent
        int jj,cvt,tt;
        char in[2000],**arr;
        int * a;
        while(1)
        {
            for( jj = 0; jj<k; jj++)
            {
                // Receive primes from child
                cvt = read(fd2[jj][0],in,2000);
                arr = lexer(in,&num);
                for(tt = 0; tt<num; tt++)
                {
                    tryInsert(atoi(arr[tt]));
                    if(counter == 2*k)
                    {
                        int pp;
                        // Kill the child processes if counter == 2*k
                        printf("numPrime == 2*k --- Killing all the child processes\n\n");
                        for(i=0; i<k; ++i)
                        {
                            kill(pid[i],SIGTERM);
                        }

                        // Print the primes
                        printf("Numbers detected as prime are: ");
                        for(pp =0; pp<2*k; pp++)
                            printf("%d ",l[pp]);
                        printf("\n");

                        // return
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}

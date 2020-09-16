/***********************************************************************************************************************************
Exam of September 2020:Write a C program that sorts sequences of words. The sequences are stored into a file, one sequence per line,
as a list of words separated by a space character. The main process receives, as input from the user:
1. the name of the file that contains the sequences,
2. the name of an output file, and
3. the number N of parallel computations that are expected.
The main process has to generate N processes, then repeat the following steps until all lines in the input file have
been read and processed:
a. read N lines from the input file and send them to the N processes (one per process). In case only M
lines are read from the file, N-M processes will not receive a new line to sort; this scenario must not
impact the computation.
b. wait for the N lines to be sorted;
c.
write them into the output file respecting the original reading order;
Each process has to:
a. sort the words in the received line,
b. send it back to the main process, and
c.
wait for a new line or for the end of the computation.
As soon as all lines have been sorted and written into the output files, the main process must stop all processes,
and it has to wait for the next set of inputs, i.e., new input and output files and new N number of parallel
computations. The full process is repeated until the user enters a 0 as new N.
The online code must include all processes and necessary synchronization structures and algorithms. You are
allowed to avoid all the file operations that are not fundamental for the synchronization.
*************************************************************************************************************************************/
/*Author: Masoud Soltanian.
this code is tested and works perfectly fine :) 
***************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h> 
#define LINELENGTH 500

int main()
{
    
    char line[LINELENGTH];
    // char sortedLines[N][LINELENGTH];
    pid_t parentPid=getpid();
    int i;//used in loops 
    FILE *fpIn;
    FILE *fpOut;
    //pid_t child[N];
    char fpInName[20], fpOutName[20];
    while(1)
    {
        int N=0;//number of processe
        int M=0;//number of lines
        fprintf(stdout,"please enter input input file name, out put file name and number of processes:\n");
        //read the input
        scanf("%s %s %d",fpInName,fpOutName,&N);
        pid_t child[N];
        if (N==0)
            break;//if user enters zero
        fpIn= fopen(fpInName,"r");
        fpOut=fopen(fpOutName,"w");
        char lines[N][LINELENGTH];// it will contain all the lines of input file
        char sortedLines[N][LINELENGTH];// it will contail all sorted lines coming from child processes in order to write in output file
        //read from Input file
        while(fgets(lines[M],LINELENGTH,fpIn) != NULL)
        {
            lines[M][strlen(lines[M])-1]='\0';
            M++;
            //lines[M++];
        }
        M--;
        int parentToChild[N][2];
        int childToParent[N][2];
        int generalPipe[2];
        pipe(generalPipe);
        for(int i=0;i<N;i++)
        {
            pipe(parentToChild[i]);
            pipe(childToParent[i]);
        }
    
        for(int i=0;i<N;i++)
        {
            child[i]=fork();
            if(child[i]==0)
                break; // child exits from the cycle of child generation
        }
        if(getpid()== parentPid) //we are in parent process
        {
            for(i=0; i<N; i++)
            {
                close(parentToChild[i][0]);//closing "read" from parentToChild pipe
                close(childToParent[i][1]);//closing "write" on childToParent pipe
            }
            close(generalPipe[0]);
            //first of all parent should send the array of pids of childs to all children to make them aware of their index for communication on pipe
            for(i=0; i<N; i++)
            {
                write(generalPipe[1],child, sizeof(pid_t)*N); 
            }
            //writing each line to each process through dedicated pipe to each process
            for(i=0; i<M; i++)
            {
                write(parentToChild[i][1],lines[i], sizeof(char)*LINELENGTH); 
            }
            //here parent waits for each child to do its job
            //here parent should read through childToparent pipe from each child, the ordered line
            //read(); 
            for(i=0; i<M; i++)
            {
                read(childToParent[i][0],sortedLines[i], sizeof(char)*LINELENGTH);
                fprintf(stdout,"%s\n",sortedLines[i]); 
            }
            // Now parent will write to the output file line by line
            for(i=0; i<M; i++)
            {
                fputs(sortedLines[i],fpOut);
                fprintf(fpOut,"\n");
            }
            fclose(fpIn);
            fclose(fpOut);
            for(int i=0;i<M;i++)// to reap the all childs
            {
                wait(NULL);
            }
            if(N>M)// if number of lines is less than number of processes according to the question assumption
            {
                for(i=M+1; i<=N; i++)
                    kill(child[i], SIGKILL);
            }
        
        }
        else//we are in child processes
        {
            //here we send whole child[] array to children in order to make each child aware of its index to be able to communicate on its own pipe with parent
            close(generalPipe[1]);
            read(generalPipe[0],child, sizeof(pid_t)*N);
            
            // close(parentToChild[indexInchild][1]);
            // close(childToParent[indexInchild][0]);
            for(i=0; i<N; i++)
            {
                close(parentToChild[i][1]);//closing "write" to parentToChild pipe
                close(childToParent[i][0]);//closing "read" from childToParent pipe
            }
            pid_t myPid=getpid();
            int myIndex;
            for(i=0; i<N; i++)//child: To find who I am in the child[N] pid array and take out my index for communication with parent on my dedicated pipe
            {

                if(myPid == child[i])
                {
                    myIndex=i;
                }
            }
            //read(pareantTochild[indexInchild][0], str, sizeof(str));// this comes from EXAM
            read(parentToChild[myIndex][0],line, sizeof(char)*LINELENGTH);//child number i reads its line from ites dedicated line
            printf("[%d] child of [%d] read:----->>> %s\n",myPid,getppid(), line);
            char word[100][30];// I assumed each line contains 100 word each with maximum 30 character length
            int count=0;
            //here I seperated all words putting each of them in a row of "words" array, ready to sort!
            char *str;
            strcpy(word[count],strtok(line," "));
            word[count][strlen(word[count])]= ' ';
            word[count][strlen(word[count])+1]= '\0';
            count++;
            while((str=strtok(NULL," "))!=NULL)//here when it encounters NULL it issues logical error if we use strcpy
            {
                strcpy(word[count],str);
                word[count][strlen(word[count])]= ' ';
                word[count][strlen(word[count])+1]= '\0';
                count++;
            }
            //hrere sort function
            char temp[30]; // temp word for swap
            for(i=0;i<count;i++)
            {
                for(int j=i+1;j<count;j++)
                {
                    if(strcmp(word[i],word[j])>0)
                    {
                        strcpy(temp,word[i]);
                        strcpy(word[i],word[j]);
                        strcpy(word[j],temp);
                    }
                }
            }
            //now I move sorted words back to the form of line
            i=0;
            strcpy(line,word[i]);
            for(i=1; i<=count; i++)
            {
                strcat(line,word[i]);
            }
            //write to the parent through childToParent[][1] the sorted line
            //write();
            write(childToParent[myIndex][1], line, sizeof(char)*LINELENGTH);
            return 0;    
        }
    }
return 0;
}

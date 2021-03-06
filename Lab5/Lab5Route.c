/*
Lab5Route.c
Created by: Chris Nelson on Friday, March 1, 2019

Compile by: gcc -o lab5 Lab5Route.c -lpthread -std=C99
Run by: ./lab5 {machine ID (0-3)} {num of machines} {cost file} {machines file}
ex: ./lab5 0 4 cost machines

1)router ID = argv[1]⟶ tells what node the program is being run
2)num of nodes = argv[2]
3)cost = argv[3]
4)machine = argv[4]
*/

#define numNodes 4

#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> //sleep
#include <time.h> //random
#include <pthread.h> //threads library
#include <limits.h> //int_max


void printTable(void);

/* define Machine struct */
typedef struct{
    char name[50];
    char ip[50];
    int port;
}Machine;


/* Global Variables */
int cost_matrix[numNodes][numNodes];
Machine linux_machines[numNodes];
int myID;
pthread_mutex_t myMutex;


/* network globals */
int sock;
struct sockaddr_in listen_addr;
struct sockaddr_in recAddr;
socklen_t addr_size;







/********************/
/* FUNCTIONS START */
/*******************/
int min(int x, int y){
    if(x<y)
        return x;
    else
        return y;
}

/* Link State Thread - Thread 3 */
void* linkState(void* args){
    
    int srcNode;
    int leastDistanceArray[numNodes];
    memset(leastDistanceArray, INT_MAX, sizeof(leastDistanceArray));
    bool visited[numNodes];
    while (true){
        for(srcNode=0; srcNode < numNodes; ++srcNode){
            //memset(visited, false, sizeof(visited)); //should work because bools are ints in 'C'
            //memcpy(leastDistanceArray, cost_matrix[srcNode], sizeof(cost_matrix[srcNode]));
            for(int i =0; i< numNodes; ++i){
                visited[i]= false;
                leastDistanceArray[i]=cost_matrix[srcNode][i];
            }
            visited[srcNode]=true;
            for(int a=0; a<numNodes-1; ++a){
                int minIndex;
                int minCost = INT_MAX;
                for(int b=0; b<numNodes; ++b){
                    if(!visited[b] && leastDistanceArray[b] < minCost){
                        minCost = leastDistanceArray[b];
                        minIndex = b;
                    }
                }
                visited[minIndex]=true;
                for(int u_node=0; u_node<numNodes; ++u_node){
                    if(!visited[u_node]){
                        leastDistanceArray[u_node] = min(leastDistanceArray[u_node], leastDistanceArray[minIndex]+cost_matrix[minIndex][u_node]);
                    }
                }
                
            }
            printf("Printing least distance array\n");
            printf("%d %d %d %d\n", leastDistanceArray[0], leastDistanceArray[1], leastDistanceArray[2], leastDistanceArray[3]);
        }
        sleep((rand()%11+10));
    }
    
     
}

/* Recieve Info Thread - Thread 2 */
void* recieveInfo(void* b){
    int pack2[3];
    while(true){
        recvfrom(sock,pack2, sizeof(pack2),0, NULL, NULL);
        pthread_mutex_lock(&myMutex); //lock
        cost_matrix[pack2[0]][pack2[1]] = pack2[2];
        cost_matrix[pack2[1]][pack2[0]] = pack2[2];
        pthread_mutex_unlock(&myMutex); //unlock
        printTable();
    }
    
}

/* print out table */
void printTable(void){
    pthread_mutex_lock(&myMutex); //lock
    int i;
    for(i=0; i<4; ++i){
        printf("%d %d %d %d\n", cost_matrix[i][0], cost_matrix[i][1], cost_matrix[i][2], cost_matrix[i][3]);
    }
    pthread_mutex_unlock(&myMutex);    //unlock
}

/* Main Function */
int main(int argc, char *argv[]){
    pthread_mutex_init(&myMutex, NULL);
    myID = atoi(argv[1]); //sets the program number of the program being run1

    /* Fill cost_matrix */
    FILE* fp = fopen(argv[3], "r"); //open cost file for reading
    int i;
    for(i=0; i<numNodes; ++i){
        fscanf(fp, "%d %d %d %d", &cost_matrix[i][0], &cost_matrix[i][1], &cost_matrix[i][2], &cost_matrix[i][3]);
    }
    fclose(fp);

    printTable();

    /* Fill Machine structs */
    FILE* mp = fopen(argv[4], "r");   //open machines file for reading
    for(i=0; i<numNodes; ++i){
        fscanf(mp, "%s %s %d", linux_machines[i].name, linux_machines[i].ip, &linux_machines[i].port);
    }
    fclose(mp);
/*----------------------------------------------------------------------------------------*/

/* Network setup */
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(linux_machines[myID].port);   //port this node will listen on
    listen_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    memset((char*)listen_addr.sin_zero, '\0', sizeof(listen_addr.sin_zero));
    addr_size = sizeof(listen_addr);
    int e1,e2;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    e2 = bind(sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
    if(sock==-1 || e2==-1){
        perror("creating socket or binding had an issue");
    }

    srand(time(NULL));  //seed random
    /* setup threads */
    pthread_t thr2, thr3;
    pthread_create(&thr3, NULL, linkState , NULL);
    pthread_create(&thr2, NULL, recieveInfo, NULL);

/*----------------------------------------------------------------------*/
    
    //input: change cost table from command line
    while(true){
    printf("please enter an update in the form:\nmachine_num new_cost\n\n");
    int pack[3]; //update array packet
    scanf("%d %d", &pack[1], &pack[2]);
    pack[0] = myID;
    pthread_mutex_lock(&myMutex); //lock
    cost_matrix[pack[0]][pack[1]] = pack[2];
    cost_matrix[pack[1]][pack[0]] = pack[2];
    pthread_mutex_unlock(&myMutex); //unlock
    //end input
    recAddr.sin_family = AF_INET;
    //send updates
    for(int i=0; i<numNodes; ++i){
        if(i == myID){
            continue;
        }
        
        recAddr.sin_port = htons (linux_machines[i].port);
        inet_pton (AF_INET, linux_machines[i].ip, &recAddr.sin_addr.s_addr);
        //memset (recAddr.sin_zero, '\0', sizeof (recAddr.sin_zero));
        addr_size = sizeof recAddr;
        sendto (sock, pack, sizeof(pack), 0, (struct sockaddr *)&recAddr, addr_size); //
    }
    sleep(10);
    }
    
    

    return 0;
}

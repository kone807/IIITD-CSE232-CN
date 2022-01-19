#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <dirent.h>
#include <netdb.h>
#include <stdlib.h>

#define PORT 8010

// struct to store arguments to be passed
// to the thread function
typedef struct pthread_arg_t {
    int socket_fd;
}pthread_arg_t;

// struct to store process details
struct proc_data
{
	int pid;
	char name[128];
	int user_time;
	int kernel_time;
};

// comparator to sort in decreasing order of total CPU time
int compare(const void *p, const void *q)
{

	struct proc_data p1 = *(const struct proc_data*)p;
	struct proc_data p2 = *(const struct proc_data*)q;

	return p1.user_time+p1.kernel_time < p2.user_time+p2.kernel_time;
}


// function called when a new thread is created
void *pthread_function(void *arg) {

    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int sockfd = pthread_arg->socket_fd;
    
    char buffer[512]; int n=0;
	FILE *fp, *sfp;

	while(1)
	{
		// receive n from client
		bzero(buffer,512);
		read(sockfd, buffer, sizeof(buffer));
		int len = atoi(buffer);

		if (strncmp("exit", buffer, 4) == 0) {
            printf("SERVER EXIT!\n");
            break;
        }

		printf("READING DATA FOR %s PROCESSES\n",buffer);

		/* --- get top n processes start */

		struct proc_data proc[512];
	
		// access /proc folder using open system call
	   struct dirent *dir;
	   DIR *dp = opendir("/proc");

	   if(dp==NULL)
	   {
	   	printf("directory not found\n");
	   	exit(1);
	   }

	   int i=0;
	   while((dir=readdir(dp))!=NULL)
	   {
	   	 int pid = atoi(dir->d_name);
	   	 if(pid!=0)
	   	 {
	   	 	char path[512];
	   	 	snprintf(path,sizeof(path),"/proc/%s/stat",dir->d_name);
	   	 	FILE *fp;
	   	 	fp = fopen(path,"r");
	   	 	if(!fp)
	   	 	{
	   	 		printf("Error in opening file\n");
	   	 		exit(1);
	   	 	}
	   	 	char name[128]; int user_time, kernel_time;

	   	 	fscanf(fp, "%d %s %*c %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %d %d",&pid, name, &user_time, &kernel_time);

	   	 	strcpy(proc[i].name,name);
	   	 	proc[i].pid=pid; proc[i].user_time=user_time; proc[i].kernel_time=kernel_time;
	   	 	
	   	 	fclose(fp);
	   	 	i++;
	   	 	
	   	 }
	   }
	   closedir(dp);
	   // sorting struct of {pid,pname,user_time,kernel_time}
	   qsort((void*)proc, i, sizeof(proc[0]),compare);
	  
  		/* --- get top n processes end */
    	
		// send data of top n processes to client
		 bzero(buffer,512);

		 for(int i=0; i<len; i++)
		 {
		 	char pid[100], user_time[100], kernel_time[100], total_time[100];
		 	snprintf(pid,100,"%d",proc[i].pid);
		 	snprintf(user_time,100,"%d",proc[i].user_time);
		 	snprintf(kernel_time,100,"%d",proc[i].kernel_time);
		 	snprintf(total_time,100,"%d",proc[i].user_time+proc[i].kernel_time);
		 	
		 	strcat(buffer,strcat(pid," "));
		 	strcat(buffer,strcat(proc[i].name," "));
		 	strcat(buffer,strcat(user_time," "));
		 	strcat(buffer,strcat(kernel_time," "));
		 	strcat(buffer,strcat(total_time,"\n"));
		 }

		 // write to client
		
		 write(sockfd,buffer,512);

		 char file_name[] = "proc_data_server";
		 char thread_id[10];
		 int tid = pthread_self();
		 sprintf(thread_id,"%d",tid);
		 strcat(file_name,thread_id);
		 strcat(file_name,".txt");
		 sfp = fopen(file_name,"a");
		 if(!sfp)
		 {
		 	printf("Error in opening file\n");
		 	exit(1);
		 }
		 fputs(buffer,sfp);
		 fclose(sfp);
		 
		printf("DATA SENT SUCCESSFULLY TO CLIENT\n");

		// read data sent by client, of top process
		bzero(buffer,512);
		read(sockfd, buffer, sizeof(buffer));
		printf("TOP PROCESS RECEIVED FROM CLIENT:\n");
		printf("p_id,	p_name	, user_time, kernel_time, total_time\n");
		printf("%s\n",buffer);
	}

	// close
	close(sockfd);

	if(n==0)
		printf("client disconnected\n");
    return NULL;
}

int main()
{

	pthread_attr_t pthread_attr;
    pthread_arg_t *pthread_arg;
    pthread_t pthread;

	// socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd==-1)
	{
		printf("socket not created\n");
		exit(1);
	}
	else
		printf("socket created successfully\n");

	// bind
	struct sockaddr_in serv_addr, cli_addr;
	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int bind_res=bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if(bind_res!=0)
	{
		printf("socket binding failed\n");
		exit(1);
	}
	else
		printf("socket successfully binded\n");

	// listen
	int listen_res = listen(sockfd,0); // 0 tells connections to be queued 
	if(listen_res!=0)
	{
		printf("socket listening failed\n");
		exit(1);
	}
	else
		printf("server is listening now!\n");


	if (pthread_attr_init(&pthread_attr) != 0) 
	{
        printf("thread could not be initialized!\n");
        exit(1);
    }
    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) 
    {
        printf("thread could not be detached!\n");
        exit(1);
    }

    // accept connection and create thread for each client connection
    while(1)
    {
    	pthread_arg = (pthread_arg_t *)malloc(sizeof *pthread_arg);

        if (!pthread_arg) 
        {
            printf("memory allocation error!\n");
            continue;
        }

        
        int len = sizeof(cli_addr);
		int connfd = accept(sockfd,(struct sockaddr*)&cli_addr, &len);

		if(connfd<0)
		{
			printf("server accept request failed\n");
			exit(1);
		}

		else
			printf("server has accepted the client\n");
        
        pthread_arg->socket_fd = connfd;
        
        // create thread for every new client
        if (pthread_create(&pthread, &pthread_attr, pthread_function, (void *)pthread_arg) != 0) {
            
            printf("error creating thread\n");
            free(pthread_arg);
            continue;
        }
    }

    return 0;
}



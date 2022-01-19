#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <netdb.h>
#include <stdlib.h>

#define PORT 8010


// function returning the first row having 
// most time consuming CPU process
void getTopProcess(char buffer[])
{
	int i=0;
	while(buffer[i]!='\n' && buffer[i]!='\0')
		i++;
	buffer[i]='\0';
}

void transfer_data_client(int sockfd)
{

	char buffer[512]; 

	while(1)
	{
		// get num_proc as input from client side
		bzero(buffer,sizeof(buffer));
		printf("ENTER NUMBER OF PROCESSES: ");
		int n=0;
		while((buffer[n++]=getchar())!='\n');
		if(n-1>=0)
		buffer[n-1]='\0';

		// close socket connection if client types "exit"
		if (strncmp("exit", buffer, 4) == 0) {
			write(sockfd,buffer,sizeof(buffer));
            printf("CLIENT EXIT!\n");
            break;
        }

		// send num_proc to server
		write(sockfd, buffer, sizeof(buffer));

		// read data of processes sent by server
		bzero(buffer,sizeof(buffer));
		read(sockfd,buffer,sizeof(buffer));
		printf("PROCESSES DATA FROM SERVER:\n");
		printf("p_id,	p_name	, user_time, kernel_time, total_time\n");
		printf("%s\n",buffer);

		// saving data to local file
		char file_name[] = "proc_data_client";
		char process_id[10];
		int pid = getpid();
		sprintf(process_id,"%d",pid);
		strcat(file_name,process_id);
		strcat(file_name,".txt");

		FILE *fp = fopen(file_name,"a");
		if(!fp)
		{
			printf("Error in opening file\n");
			exit(1);
		}
		fputs(buffer,fp);
		fclose(fp);

		// write data of top_most process to server
		getTopProcess(buffer);
		write(sockfd,buffer,sizeof(buffer));
		bzero(buffer,sizeof(buffer));
	}
}

int main()
{
	// socket, connect, write, read, close

	// socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd==-1)
	{
		printf("socket not created\n");
		exit(1);
	}
	else
		printf("socket created successfully\n");

	// connect
	struct sockaddr_in serv_addr, cli;
	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int conn_res = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if(conn_res!=0)
	{
		printf("connection with server failed\n");
		exit(1);
	}
	else
		printf("connection with the server is on\n");

	// helper function to help with data transfer
	transfer_data_client(sockfd);
	// close
	close(sockfd);
	return 0;
}


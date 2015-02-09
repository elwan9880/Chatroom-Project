#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

int n;

void error(const char *msg) {
	perror(msg);
	exit(0);
}

void recvmessage(int sockfduser) {//接收server訊息的thread function，所有接收動作都在這裡完成
	char recvbuffer[256];//怕跟main的buffer互相干擾，所以另外做出recvbuffer
	while (1) {
		bzero(recvbuffer, 256);
		n = recv(sockfduser, recvbuffer, 255, 0); //recv
		if (n < 0)
			error("ERROR reading from socket");
		printf("\n%s\n", recvbuffer);
	}
}

int main(int argc, char *argv[]) {
	int sockfd, portno;
	char buffer[256];
	char clientname[256];//存自己的暱稱
	pthread_t thread1;
	pthread_attr_t attr;
	struct sockaddr_in serv_addr;
	if (argc < 3) {
		fprintf(stderr, "usage %s serverip port\n", argv[0]);
		exit(0);
	}
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //return a file descriptor for new socket,or -1 for errors.
	if (sockfd < 0)
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //connect
		error("ERROR connecting");
	//------------------------接收server的LIST----------------------------
	bzero(buffer, 256);
	n = recv(sockfd, buffer, 255, 0); //recv
	printf("%s\n", buffer);
	//-------------------------------------------------------------------

	//-----------------------輸入暱稱並等候server做檢查----------------------
	while (1) {
		printf("SERVER: Please enter your nickname: ");
		bzero(buffer, 256);
		fgets(buffer, 255, stdin);
		buffer[strlen(buffer) - 1] = '\0';
		bzero(clientname, 256);
		strcpy(clientname, buffer);
		n = send(sockfd, buffer, strlen(buffer), 0); //send欲想要用的使用者名子
		if (n < 0)
			error("ERROR writing to socket");
		bzero(buffer, 256);
		n = recv(sockfd, buffer, 255, 0);//recv server指令
		printf("SERVER: %s\n", buffer);
		if (strcmp(buffer, "OK") == 0) {
			break;
		}
		if (strcmp(buffer, "FAIL") == 0) {
			printf("SERVER: Your nickname is used by another person.\n");
		}
	}
	//-------------------------------------------------------------------

	//--------------------------開啟接收訊息的thread------------------------
	(void) pthread_attr_init(&attr);
	pthread_create(&thread1, &attr, (void*) &recvmessage, (int*) sockfd);
	//-------------------------------------------------------------------

	//-------------------------將傳訊息的功能寫在main裏----------------------
	while (1) {
		printf("%s: ", clientname);
		bzero(buffer, 256);
		fgets(buffer, 255, stdin);
		buffer[strlen(buffer) - 1] = '\0';
		n = send(sockfd, buffer, strlen(buffer), 0); //send

		//--------------------------輸入CLOSE=>關閉程式-------------------
		if (strncmp(buffer, "CLOSE", 5) == 0) {
			printf("SERVER: Terminate the client.\n");
			break;//跳出去關閉此client的socket
		}
		//--------------------------------------------------------------

		//------------------------輸入LIST=>接收使用者名單------------------
		//此功能由recvmessage接收server訊息
		//--------------------------------------------------------------

		//------------------------輸入SEND=>接收使用者名單------------------
		//此功能由recvmessage接收server訊息
		//--------------------------------------------------------------

		sleep(1);//
	}
	close(sockfd); //close
	return 0;
	//---------------------------------------------------------------------
}

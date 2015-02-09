#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <ctype.h>

char buffer[256];
int n;
int i = 0;
int j = 0;
int newsockfd[100];//client連上的socket，用array存，裏面的號碼為使用者編號，最多可供100個使用者使用
int usernum = 0; //現在存在的使用者數，不包括CLOSE的使用者數
int maxnumuser = 0; //包括已經CLOSE的使用者數
char clientname[100][256];//client的名稱，使用二維陣列存，前面表示使用者號碼，後面是存字元用

void error(const char *msg) {
	perror(msg);
	exit(1);
}

void usertalk(int clientnuminfunction) {//clientnuminfunction為此thread使用者的編號
	while (1) {
		int buffernamelength = 0;//client名子的長度
		char buffername[256];//SEND指令中傳訊息的使用者的名子
		int determine = 0;
		int targetnum;//SEND指令中收訊息的使用者編號的buffer
		char msg[256];//SEND指令中放傳送訊息的buffer
		char targetmsg[256];//SEND指令中最後傳給收訊息使用者的buffer
		bzero(buffer, 256);
		n = recv(newsockfd[clientnuminfunction], buffer, 255, 0); //recv使用者指令
		if (n < 0)
			error("ERROR reading from socket");

		//--------------------------收到CLOSE=>關閉程式------------------------
		if (strncmp(buffer, "CLOSE", 5) == 0) {
			printf("Terminate %s's service.\n", clientname[clientnuminfunction]);
			usernum--;//現在使用者-1
			bzero(clientname[clientnuminfunction], 256);//把此使用者現在用的clientname buffer清光
			break;//跳出去關閉此client的socket
		}
		//-------------------------------------------------------------------

		//------------------------收到LIST=>傳送使用者名單-----------------------
		else if (strncmp(buffer, "LIST", 4) == 0) {
			sprintf(buffer, "SERVER: ONLINE %d", usernum);
			for (i = 0; i <= maxnumuser; i++) {//將所有使用者一個一個黏起來(還有空白)
				if (strlen(clientname[i]) != 0) {//因為前面的號碼可能為空的(有人CLOSE)，所以要跳過空白的clientname
					strcat(buffer, " ");
					strcat(buffer, clientname[i]);
				}
			}
			n = send(newsockfd[clientnuminfunction], buffer, strlen(buffer), 0); //send最後LIST結果
		}
		//-------------------------------------------------------------------

		//------------------------收到SEND=>傳值給其他使用者---------------------
		else if (strncmp(buffer, "SEND ", 5) == 0) {
			bzero(buffername, 256);
			for (j = 5;; j++) {//除去SEND ，從第五個開始算(算目標的名子長度及存取目標名子)
				if (buffer[j] == ' ') {//如果讀到空白就跳出(名子跟訊息中間的空白)
					break;
				} else {
					buffernamelength++;//存目標名子長度
					buffername[j - 5] = buffer[j];//將目標名子存到buffername裏(buffername要從0開始存)
				}
			}
			for (i = 0; i <= maxnumuser; i++) {//比對LIST內有沒有目標的名子
				if (strcmp(buffername, clientname[i]) == 0) {
					determine = 1;//determine=1及為有此人
					targetnum = i;//此時的i就是目標的編號
					break;
				}
			}
			if (determine == 1) {//代表有此人，此人編號為targetnum

				bzero(msg, 256);
				for (i = 6 + buffernamelength; i <= strlen(buffer); i++) {//找出msg，初始位置是從i = 6 + buffernamelength(SEND加兩個空白加名子長度)
					msg[i - 6 - buffernamelength] = buffer[i];//放進msg裏
				}
				bzero(targetmsg, 256);
				sprintf(targetmsg, "SERVER: MSG ");
				strcat(targetmsg, clientname[clientnuminfunction]);
				strcat(targetmsg, " ");
				strcat(targetmsg, msg);//上面五行是把"SERVER: MSG ",clientname,msg全部黏到targetmsg

				n = send(newsockfd[clientnuminfunction], "SERVER: SEND OK", 15,
						0);//傳SENK OK給傳訊息的人
				n = send(newsockfd[targetnum], targetmsg, strlen(targetmsg), 0);//傳targetmsg給目標人
			} else {//沒有此人的情況
				n = send(newsockfd[clientnuminfunction], "SERVER: SEND FAIL",
						17, 0);//傳SEND FAIL給傳訊息的人
			}
		}
		//-------------------------------------------------------------------

		//-----------------------其他狀況=>回傳指令錯誤--------------------------
		else {
			n = send(newsockfd[clientnuminfunction], "SERVER: Command Error",
					21, 0);
		}
		//-------------------------------------------------------------------

	}
	close(newsockfd[clientnuminfunction]); //close
}

int main(int argc, char *argv[]) {
	int sockfd, portno;
	int clientnum = 0;//當下的使用者的編號，從0開始算
	pthread_t thread1;
	pthread_attr_t attr;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //return a file descriptor for new socket,or -1 for errors.
	if (sockfd < 0)
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]); //Convert string to integer.
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY; //serv_addr.sin_addr.s_addr =inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, //bind
			sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	listen(sockfd, 2); //listen

	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd[clientnum] = accept(sockfd, //accept
				(struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd[clientnum] < 0)
			error("ERROR on accept");

		//--------------------server傳送使用者名單-----------------------------
		bzero(buffer, 256);
		sprintf(buffer, "SERVER: ONLINE %d", usernum);
		for (i = 0; i < clientnum; i++) {//同上面usertalk，把LIST黏起來傳出去
			if (strlen(clientname[i]) != 0) {
				strcat(buffer, " ");
				strcat(buffer, clientname[i]);
			}
		}
		n = send(newsockfd[clientnum], buffer, strlen(buffer), 0); //send
		//------------------------------------------------------------------

		//-----------------------server檢查暱稱-------------------------------
		while (1) {
			bzero(buffer, 256);
			int determine = 0;
			n = recv(newsockfd[clientnum], buffer, 255, 0);
			for (i = 0; i < clientnum; i++) {//逐號碼檢查有沒有重複
				if (strcmp(buffer, clientname[i]) == 0) {
					determine = 1;//determine=1代表有重複的人
					break;
				}
			}
			if (determine == 1) {//此名稱已經被使用
				n = send(newsockfd[clientnum], "FAIL", 4, 0); //send FAIL
			} else if (determine == 0) {//此名稱沒有被使用
				n = send(newsockfd[clientnum], "OK", 2, 0); //send OK
				strcpy(clientname[clientnum], buffer);//存入clientname裏
				break;//跳出
			}
		}
		//-------------------------------------------------------------------

		(void) pthread_attr_init(&attr);
		pthread_create(&thread1, &attr, (void*) &usertalk, (int*) clientnum);
		clientnum++; //當下的使用者的編號，下面兩參數可參照開頭注解
		usernum++;
		maxnumuser++;
	}
	close(sockfd);
	return 0;

}

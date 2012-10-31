#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/file.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#define ERROR -1
#define MAX_CLIENTS 5
#define MAX_DATA 1024
#define STDIN 0
int portC;
int sock;
struct sockaddr_in server;
struct sockaddr_in client;
struct sockaddr_in sin;
struct in_addr in;
struct hostent *he;
struct stat st;
unsigned int getlen;
size_t sockaddr_len = sizeof(struct sockaddr_in);
int data_len;
char data[MAX_DATA];
fd_set master; // master file descriptor list
fd_set read_fds;// temp file descriptor list for select()
int fdmax;
char temp[100];
int noofclients = 0;
int sockfd;
char cmd[50];
struct sock_detail {
	int arr_sock;
	char host_name[50];
	int entryno;
};
struct sock_detail clin[10];
void ClientConnx(int total, const char *newh);
void HandleData(int new, int k);
void GetMyIP();
void HandleInput(char com[100]);
void ShowList();
void Terminate(int no);
void Addsock(int socknew, const char *hostn);
void Download(char com[100]);
void Upload(int chi, const char *filen);

void ClientConnx(int total, const char *newh) {
	int portno;
	int f = 1;
	int z;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int flag = 0;
	portno = total;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	server = gethostbyname(newh);
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");

	}
	FD_SET(sockfd,&read_fds);
	if (sockfd > fdmax) {
		fdmax = sockfd;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr,
			server->h_length);
	serv_addr.sin_port = htons(portno);

	if ((strcmp(newh, temp)) == 0) {
		printf("You are trying to connect to your own host");
		flag = 1;
	}
	for (z = 1; z <= noofclients; z++) {
		printf("%d-%s\n", clin[z].entryno, clin[z].host_name);

	}
	for (f = 1; f <= noofclients; f++) {
		if ((strcmp(newh, clin[f].host_name)) == 0) {

			printf("You are already connected\n");
			flag = 1;

			break;
		}
	}
	if (flag == 0) {
		if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
				< 0) {
			perror("ERROR connecting");
		} else {
			Addsock(sockfd, newh);
			printf("Connected....\n");
		}
	}

}

void HandleData(int new, int ent) {

	int cont;
	char buffer[1024] = "";
	char fname[20];
	int fd;
	double size = 0;
	char *a;
	char *bi;
	int counter = 0;
	char ord[30];
	char toke[50] = "";
	char buffer1[1024] = "";
	FILE *p = NULL;
	data_len = recv(new, data, sizeof(data), 0);
	strcpy(ord, data);
	if (strcmp(ord, "error") == 0) {
		printf("No such file on the server\n");
	} else if ((strncmp(ord, "u", 1)) == 0) {
		printf("Downloading ....\n");
		time_t start, end;
		double dif = 0;
		char fname1[50];
		double newsize = 0;
		int total = 0;
		int b = 0;
		char sizec[50];
		double rate = 0;
		bi = strtok(ord, "$");
		bi = strtok(NULL, "$");
		strcpy(fname1, bi);
		bi = strtok(NULL, "$");
		strcpy(sizec, bi);
		newsize = atoi(sizec);
		bzero(buffer1, sizeof(buffer1));
		p = fopen(fname1, "a");
		if (p == NULL) {
			printf("Error in opening a file..");
		}
		time(&start);
		while (total < newsize) {
			b = recv(new, buffer1, sizeof(buffer1), 0);

			fwrite(buffer1, 1, b, p);
			total = total + b;
			bzero(buffer1, sizeof(buffer1));

		}
		time(&end);
		dif = difftime(end, start);
		rate = ((double) newsize / (double) dif) * 8;
		fclose(p);
		printf(
				"File created successfully with rate %f Bytes/sec and %f size and %f is diff\n",
				rate, newsize, dif);
		FD_CLR(new,&read_fds);

	} else if ((strncmp(ord, "d", 1)) == 0) {
		printf("Uploading....\n");
		//		recv(new, data, sizeof(data), 0);
		//		send(new, "upload", sizeof("upload"), 0);
		sleep(1);
		time_t start, end;
		double dif = 0;
		double rate = 0;
		char sizechar[10];
		a = strtok(ord, "$");
		a = strtok(NULL, "$");
		strcpy(fname, a);
		stat(fname, &st);
		size = st.st_size;
		sprintf(sizechar, "%f", size);
		strcpy(toke, "u$");
		strcat(toke, fname);
		strcat(toke, "$");
		strcat(toke, sizechar);
		if ((fd = open(fname, O_RDONLY)) < 0) {
			perror("404: File Open Failed");
			send(new, "error", sizeof("error"), 0);

		} else {
			send(new, toke, sizeof(toke), 0);
			printf("fname is %s\n", fname);

			//sleep(1);

			time(&start);
			while ((cont = read(fd, buffer, sizeof(buffer))) > 0) {
				send(new, buffer, cont, 0);
				counter = counter + cont;
				bzero(buffer, sizeof(buffer));
			}
			time(&end);
			dif = difftime(end, start);
			rate = ((double) size / (double) dif) * 8;
			printf(
					"Data sent successfully with speed %f Bytes/sec and size is  %f and %f is diff\n",
					rate, size, dif);
			FD_CLR(new,&read_fds);
		}
	} else if (data_len == 0) {
		printf("insaide terminate\n");

		Terminate(ent);
	}
}
void HandleInput(char com[100]) {
	char delims[] = " \n";
	char tdownchk[100] = "";
	char *result = NULL;
	int i = 1;
	char currhost[20];
	int chi;
	char filen[30];
	char *args[i];
	int total;
	strcpy(tdownchk, com);
	result = strtok(com, delims);
	args[0] = strdup(result);
	if ((strcmp(result, "connect")) == 0) {
		while (result != NULL) {
			result = strtok(NULL, delims);

			args[i] = result;
			i++;

		}
		if (args[1] != NULL && args[2] > 0 && args[3] == NULL) {
			strcpy(currhost, args[1]);
			total = atoi(args[2]);
			ClientConnx(total, currhost);
		} else
			printf("Invalid Input\n");

	} else if ((strcasecmp(args[0], "help")) == 0) {
		printf("-Connect\n");
		printf("-Myport\n");
		printf("-MyIP\n");
		printf("-List\n");
		printf("-Terminate\n");
		printf("-Download\n");
		printf("-Upload\n");
		printf("-Exit\n");
	} else if ((strcasecmp(args[0], "myport")) == 0) {
		printf("Port is-%d\n", portC);
	} else if ((strcasecmp(args[0], "myip")) == 0) {
		GetMyIP();
	} else if ((strcasecmp(args[0], "list")) == 0) {
		ShowList();
	} else if ((strcasecmp(args[0], "terminate")) == 0) {
		args[1] = strtok(NULL, delims);
		int no = atoi(args[1]);
		if (clin[no].entryno != 0) {
			Terminate(no);
		}
	} else if ((strcasecmp(args[0], "download")) == 0) {
		Download(tdownchk);

	} else if ((strcasecmp(args[0], "upload")) == 0) {
		while (result != NULL) {
			result = strtok(NULL, delims);

			args[i] = result;
			i++;
			if (args[0] != NULL && args[2] != NULL && args[1] > 0 && args[3]
					== NULL) {
				chi = atoi(args[1]);
				strcpy(filen, args[2]);
				Upload(chi, filen);
			}

		}
	} else if (strcasecmp(args[0], "creator") == 0) {

		printf("NAME-Ameya Sawant\n");

	} else if (strcasecmp(args[0], "exit") == 0) {

		exit(0);
	}
	else{
		printf("Invalid Input\n");
	}
	fflush(stdout);
}

void GetMyIP() {
	char buffer[16];

	size_t buflen = 16;
	assert(buflen >= 16);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(sock != -1);

	const char* googip = "8.8.4.4       ";
	int googport = 53;
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(googip);
	serv.sin_port = htons(googport);

	int err = connect(sock, (struct sockaddr*) &serv, sizeof(serv));
	assert(err != -1);

	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	err = getsockname(sock, (struct sockaddr*) &name, &namelen);
	assert(err != -1);
	const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, buflen);
	printf("The IP address of your sytem is :%s \n", buffer);
	fflush(stdout);
	assert(p);
}

void ShowList() {
	int k;
	if (noofclients == 0) {
		printf("You are not connected to any client\n");
	} else {
		for (k = 1; k <= noofclients; k++) {
			if (clin[k].entryno != 0) {
				printf("%d-Connected to %s\n", clin[k].entryno,
						clin[k].host_name);
			}
		}
	}
}

void Terminate(int no) {
	int k;

	FD_CLR(clin[no].arr_sock,&read_fds);
	close(clin[no].arr_sock);
	clin[no].entryno = 0;
	clin[no].arr_sock = 0;
	for (k = no; k < noofclients; k++) {
		clin[k] = clin[k + 1];
		clin[k + 1].entryno = 0;
	}
	noofclients--;
	printf("Terminated\n");

}
void Addsock(int socknew, const char *hostn) {
	int i;
	for (i = 1; i < 5; i++) {
		if (clin[i].arr_sock == 0) {
			clin[i].arr_sock = socknew;
			strcpy(clin[i].host_name, hostn);
			clin[i].entryno = i;
			FD_SET(clin[i].arr_sock, &read_fds);
			noofclients++;
			break;
		}
	}
}

void Download(char com[20]) {
	int choice;
	int n;
	int i = 1;
	char delims[] = " \n";
	char *result = NULL;
	char tok[50];
	char filen[30];
	char *args[10];
	int new[10];
	int chi;
	int total = 0;
	int counter = 0;
	result = strtok(com, delims);
	args[0] = result;
	while (result != NULL) {
		result = strtok(NULL, delims);
		if (result != NULL) {
			if (i % 2 == 0) {
				args[counter] = result;
				counter++;
			} else {
				new[counter] = atoi(result);
			}
			i++;
		}
	}
	while (total < counter) {
		chi = new[total];
		strcpy(filen, args[total]);
		strcpy(tok, "d$");
		strcat(tok, filen);
		n = send(clin[chi].arr_sock, tok, sizeof(tok), 0);
		choice = chi;
		if (n < 0)
			error("ERROR writing to socket");
		total++;

	}
}

void Upload(int chi, const char *filen) {
	char fname[20];
	char buffer[1024];
	char toke[50];
	char sizechar[10];
	int n, cont;
	int counter = 0;
	int fd;
	int size = 0;
	strcpy(fname, filen);
	printf("Uploading....\n");
	stat(fname, &st);
	size = st.st_size;
	sprintf(sizechar, "%d", size);
	strcpy(toke, "u$");
	strcat(toke, fname);
	strcat(toke, "$");
	strcat(toke, sizechar);
	write(clin[chi].arr_sock, toke, sizeof(toke));

	if ((fd = open(filen, O_RDONLY)) < 0) {
		perror("404: File Open Failed");
	}
	while ((cont = read(fd, buffer, sizeof(buffer))) > 0) {
		send(clin[chi].arr_sock, buffer, cont, 0);
		counter = counter + cont;
		bzero(buffer, sizeof(buffer));
	}
	printf("Data has been uploaded\n");
	if (n < 0)
		error("ERROR writing to socket");
}

int main(int argc, char **argv) {
	int new;
	int fd;
	char host[1024];
	int i;
	char in[100];
	int bytes_read = 100;
	int nos;
	if (argv[1] != NULL && argv[2] == NULL) {
		printf("Remote File Sharing System........");
		portC = atoi(argv[1]);
		int yes = 1;
		gethostname(temp, sizeof(temp));
		FD_ZERO(&master);
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
			perror("server socket: ");
			exit(-1);
		}
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		server.sin_family = AF_INET;
		server.sin_port = htons(portC);
		server.sin_addr.s_addr = INADDR_ANY;
		bzero(&server.sin_zero, 8);
		if ((bind(sock, (struct sockaddr *) &server, sockaddr_len)) == ERROR) {
			perror("bind : ");
			exit(-1);
		}
		if ((fd = listen(sock, MAX_CLIENTS)) == ERROR) {
			perror("listen");
			exit(-1);
		}
		FD_SET (0, &master);
		FD_SET(sock, &master);
		fdmax = sock;
		printf("Remote File Sharing System........");
		while (1) {
			FD_ZERO(&read_fds);
			FD_SET (0, &read_fds);
			FD_SET(sock, &read_fds);
			for (i = 0; i < 5; i++) {
				if (clin[i].arr_sock != 0) {
					FD_SET(clin[i].arr_sock,&read_fds);
					if (clin[i].arr_sock > fdmax) {
						fdmax = clin[i].arr_sock;
					}
				}
			}
			if ((nos = select(fdmax + 1, &read_fds, NULL, NULL, NULL)) == -1) {
				perror("Unable to execute the select call");
			}

			else {
				if (FD_ISSET(0, &read_fds)) {
					fflush(stdin);
					fgets(in, bytes_read, stdin);

					HandleInput(in);
				}

				else if (FD_ISSET(sock, &read_fds)) {
					new = accept(sock, (struct sockaddr *) &client,
							&sockaddr_len);
					printf("New Client connected......\n");
					getnameinfo((struct sockaddr *) &client, sizeof client,
							host, sizeof host, NULL, 0, 0);
					Addsock(new, host);

				}

				else {
					for (i = 0; i < 5; i++) {
						if (clin[i].arr_sock != 0) {
							if (FD_ISSET(clin[i].arr_sock, &read_fds)) {
								HandleData(clin[i].arr_sock, i);
							}
						}
					}

				}
			}
		}
	} else {
		printf("Enter data in proper format\n");
	}
	return 0;
}

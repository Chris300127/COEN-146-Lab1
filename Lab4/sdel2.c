
/********************
 * COEN 146, UDP example, server
 ********************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include "tfv2.h"

/********************
 * main
 ********************/

int main (int argc, char *argv[]) {
	int sock, nBytes;
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	int i;
  	int pak_checksum;
	int local_checksum;
	int state = 0; //state of pgrm

	FILE *outFile;
	PACKET *s_pak = malloc(sizeof(PACKET));

    if (argc != 2)
    {
        printf ("need the port number\n");
        return 1;
    }

	// init
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons ((short)atoi (argv[1]));
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));
	addr_size = sizeof (serverStorage);

	// create socket
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0){
		printf ("socket error\n");
		return 1;
	}

	// bind
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0){
		printf ("bind error\n");
		return 1;
	}

recv:recvfrom (sock, s_pak, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, &addr_size);
    pak_checksum = s_pak->header.checksum;
    s_pak->header.checksum = 0;
	local_checksum = calc_checksum(s_pak, sizeof(s_pak->header)+s_pak->header.length);

	if(local_checksum!=pak_checksum || state!=s_pak->header.seq_ack){
		printf("server inside if stmt\n");
		s_pak->header.seq_ack = (s_pak->header.seq_ack==0)? 1: 0; //flip ack to negative ack
		sendto (sock, s_pak, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);
		printf("server hit recv\n");
		goto recv;
	}

	/* send back positive ack */
	sendto (sock, s_pak, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);

	printf("%s\n",s_pak->data);
	outFile = fopen(s_pak->data, "w");
	printf("server reached while loop\n");
	while (1){
		state = (state==0) ? 1:0; //flips state
		// receive  datagrams
		//printf("server inside while loop\n");
		memset(s_pak->data, '\0',sizeof(s_pak->data));
		nBytes = recvfrom (sock, s_pak, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, &addr_size);
		if(s_pak->header.length == 0){
			fclose(outFile);
			goto EndStmt;
		}
		printf ("recv: %s with seq num: %d and state: %d\n", s_pak->data, s_pak->header.seq_ack, state);
		//printf("server recieved datagram\n");
		pak_checksum = s_pak->header.checksum;
    	s_pak->header.checksum = 0;
		local_checksum = calc_checksum(s_pak, sizeof(s_pak->header)+s_pak->header.length);


		if(local_checksum != pak_checksum || (state!=s_pak->header.seq_ack) ){
			printf("checksum failed\n");
			s_pak->header.checksum = 0;
			printf("%d vs %d\n",pak_checksum, local_checksum);
			printf("%s\n",s_pak->data);
			s_pak->header.seq_ack = (s_pak->header.seq_ack==0? 1: 0); //flips ack to negative ack
			sendto (sock, s_pak, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, addr_size); //send neg ack
		}
		else{
			printf("write %s\n", s_pak->data);
			fwrite(s_pak->data, 1, s_pak->header.length, outFile);
			printf("written %s\n", s_pak->data);
		}
		//printf("server reached sendto\n");
		sendto (sock, s_pak, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);
		//printf("server passed sendto\n");


		/*
		// convert message
		for (i = 0; i < nBytes - 1; i++)
			buffer[i] = toupper (buffer[i]);

		// send message back
		sendto (sock, buffer, nBytes, 0, (struct sockaddr *)&serverStorage, addr_size);
		*/
	}

EndStmt:	return 0;
}

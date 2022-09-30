#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

/* 
    The Tram data server (server.py) publishes messages over a custom protocol. 
    
    These messages are either:

    1. Tram Passenger Count updates (MSGTYPE=PASSENGER_COUNT)
    2. Tram Location updates (MSGTYPE=LOCATION)

    It publishes these messages over a continuous byte stream, over TCP.

    Each message begins with a 'MSGTYPE' content, and all messages are made up in the format of [CONTENT_LENGTH][CONTENT]:

    For example, a raw Location update message looks like this:

        7MSGTYPE8LOCATION7TRAM_ID7TRAMABC5VALUE4CITY

        The first byte, '7', is the length of the content 'MSGTYPE'. 
        After the last byte of 'MSGTYPE', you will find another byte, '8'.
        '8' is the length of the next content, 'LOCATION'. 
        After the last byte of 'LOCATION', you will find another byte, '7', the length of the next content 'TRAM_ID', and so on.

        Parsing the stream in this way will yield a message of:

        MSGTYPE => LOCATION
        TRAM_ID => TRAMABC
        VALUE => CITY

        Meaning, this is a location message that tells us TRAMABC is in the CITY.

        Once you encounter a content of 'MSGTYPE' again, this means we are in a new message, and finished parsing the current message

    The task is to read from the TCP socket, and display a realtime updating dashboard all trams (as you will get messages for multiple trams, indicated by TRAM_ID), their current location and passenger count.

    E.g:

        Tram 1:
            Location: Williams Street
            Passenger Count: 50

        Tram 2:
            Location: Flinders Street
            Passenger Count: 22

    To start the server to consume from, please install python, and run python3 server.py 8081

    Feel free to modify the code below, which already implements a TCP socket consumer and dumps the content to a string & byte array
*/

char tram1_loc[100];
char tram1_passengers[10];

char tram2_loc[100];
char tram2_passengers[10];

char tram3_loc[100];
char tram3_passengers[10];

char tram4_loc[100];
char tram4_passengers[10];

void error(char* msg) {
    perror(msg);
    exit(1);
}

void dump_buffer(char* name) {
    int e;
    size_t len = strlen(name);
    for (size_t i = 0; i < len; i++) {
        e = name[i];
        printf("%-5d", e);
    }
    printf("\n\n");
    for (size_t i = 0; i < len; i++) {
        char c = name[i];
        // if (!isalpha(name[i]) && !isdigit(name[i]) && (c != '_') && (c != ' '))
        //     c = '*';
        printf("%-5c", c);
    }
    printf("\n\n");
}

void extract_data_from_buffer(char* name) {
	int e;
	char tram_id[10] = "";
	int tram_id_value = 0;
	char passengers[10] = "";
	char tram_loc[100] = "";
	char msg_type[20] = "";
	size_t len = strlen(name);
	int next_seg_len = 0;

	//First Value in the buffer should be an int
	next_seg_len = name[0];

	name = name + next_seg_len+1;

	//Next value is message type, lets skip the MSG_TYPE identifier

	next_seg_len = name[0];

	name = name + 1;

	for (size_t i = 0; i < next_seg_len; i++) {
		msg_type[i] = name[i];
    }

    name = name+next_seg_len;

    //Next value is Tram Id. First, lets skip TRAM_ID identifier.

    next_seg_len = name[0];

    name = name + next_seg_len+1;

    next_seg_len = name[0];

	name = name + 1;

	for (size_t i = 0; i < next_seg_len; i++) {
		tram_id[i] = name[i];
    }

    tram_id_value = name[5]-'0';

    name = name+next_seg_len;

    //Next segment is either passenger count or location. Lets skip the identifier first.

    next_seg_len = name[0];

	name = name + next_seg_len+1;

	//Now lets remember our message type and proceed accordingly


    if(strcmp(msg_type,"PASSENGER_COUNT") == 0) {
    	next_seg_len = name[0];

    	name = name + 1;

    	for (size_t i = 0; i < next_seg_len; i++) {
			passengers[i] = name[i];
    	}	

    	name = name + next_seg_len;

    	switch(tram_id_value)
    	{
    		case 1:
    			strcpy(tram1_passengers, passengers); 
    			break;

    		case 2:
    			strcpy(tram2_passengers, passengers);
    			break;

    		case 3:
    			strcpy(tram3_passengers, passengers);
    			break;

    		case 4:
    			strcpy(tram4_passengers, passengers);
    			break;

    		default:
    			printf("Unknown Tram ID");
    	}

    } else {
    	//Is Location
    	next_seg_len = name[0];

    	name = name + 1;

    	for (size_t i = 0; i < next_seg_len; i++) {
			tram_loc[i] = name[i];
    	}	

    	name = name + next_seg_len;

    	switch(tram_id_value)
    	{
    		case 1:
    			strcpy(tram1_loc, tram_loc);
    			break;

    		case 2:
    			strcpy(tram2_loc, tram_loc);
    			break;

    		case 3:
    			strcpy(tram3_loc, tram_loc);
    			break;

    		case 4:
    			strcpy(tram4_loc, tram_loc);
    			break;

    		default:
    			printf("Unknown Tram ID");
    	}
    }
}

void display_tram_info() {
	printf("\n========================================================\n");

	printf("Tram 1:\n");
	printf("\tLocation:%s\n", tram1_loc);
	printf("\tPassenger Count:%s\n", tram1_passengers);
	printf("\n\n");

	printf("Tram 2:\n");
	printf("\tLocation:%s\n", tram2_loc);
	printf("\tPassenger Count:%s\n", tram2_passengers);
	printf("\n\n");

	printf("Tram 3:\n");
	printf("\tLocation:%s\n", tram3_loc);
	printf("\tPassenger Count:%s\n", tram3_passengers);
	printf("\n\n");

	printf("Tram 4:\n");
	printf("\tLocation:%s\n", tram4_loc);
	printf("\tPassenger Count:%s\n", tram4_passengers);
	printf("\n\n");

	printf("\n========================================================\n");


}


int main(int argc, char *argv[]){
	if(argc < 2){
        fprintf(stderr,"No port provided\n");
        exit(1);
	}	
	int sockfd, portno, n;
	char buffer[255];

	char tram1_loc[100] = "";
	char tram1_passengers[10] = "";

	char tram2_loc[100] = "";
	char tram2_passengers[10] = "";

	char tram3_loc[100] = "";
	char tram3_passengers[10] = "";

	char tram4_loc[100] = "";
	char tram4_passengers[10] = "";
	
	struct sockaddr_in serv_addr;
	struct hostent* server;
	
	portno = atoi(argv[1]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0){
		error("Socket failed \n");
	}
	
	server = gethostbyname("127.0.0.1");
	if(server == NULL){
		error("No such host\n");
	}
	
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	
	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0)
		error("Connection failed\n");
	
	while(1){
		bzero(buffer, 256);
		n = read(sockfd, buffer, 255);
		if(n<0)
			error("Error reading from Server");
		//dump_buffer(buffer);
		extract_data_from_buffer(buffer);
		display_tram_info();
	}
	
	return 0;
}
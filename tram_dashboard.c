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

struct tramInfo {
	char tramId[20];
	char location[100];
	char passengers[10];
};

struct tramDb {
	struct tramInfo trams[1000];
	int numTrams;
};

struct tramMsg {
	char tramId[20];
	char location[100];
	char passengers[10];
	char msgType[20];
};


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

struct tramMsg *extract_data_from_buffer(char* name, struct tramMsg *msg) {
	// int e;
	// char tram_id[10] = "";
	// int tram_id_value = 0;
	// char passengers[10] = "";
	// char tram_loc[100] = "";
	// char msg_type[20] = "";
	// size_t len = strlen(name);
	int next_seg_len = 0;

	//First Value in the buffer should be an int
	next_seg_len = name[0];

	name = name + next_seg_len+1;

	//Next value is message type, lets skip the MSG_TYPE identifier

	next_seg_len = name[0];

	name = name + 1;

	for (size_t i = 0; i < next_seg_len; i++) {
		msg->msgType[i] = name[i];
    }

    name = name+next_seg_len;

    //Next value is Tram Id. First, lets skip TRAM_ID identifier.

    next_seg_len = name[0];

    name = name + next_seg_len+1;

    next_seg_len = name[0];

	name = name + 1;

	for (size_t i = 0; i < next_seg_len; i++) {
		msg->tramId[i] = name[i];
    }

    name = name+next_seg_len;

    //Next segment is either passenger count or location. Lets skip the identifier first.

    next_seg_len = name[0];

	name = name + next_seg_len+1;

	//Now lets remember our message type and proceed accordingly


    if(strcmp(msg->msgType,"PASSENGER_COUNT") == 0) {
    	next_seg_len = name[0];

    	name = name + 1;

    	for (size_t i = 0; i < next_seg_len; i++) {
			msg->passengers[i] = name[i];
    	}

    	name = name + next_seg_len;

    } else {
    	//Is Location
    	next_seg_len = name[0];

    	name = name + 1;

    	for (size_t i = 0; i < next_seg_len; i++) {
			msg->location[i] = name[i];
    	}	

    	name = name + next_seg_len;
    }
    return msg;
}

void update_tram_db(struct tramDb *db, struct tramMsg *msg){

	int id_exists = 0;
	int element_pos = -1;

	for(int i=0; i<db->numTrams;i++){
		if(strcmp(msg->tramId, db->trams[i].tramId) == 0){
			id_exists = 1;
			element_pos = i;
		}
	}

	if(id_exists){
		if(strcmp(msg->msgType, "LOCATION") == 0){
			strcpy(db->trams[element_pos].location, msg->location); 
		} else if(strcmp(msg->msgType, "PASSENGER_COUNT") == 0) {
			strcpy(db->trams[element_pos].passengers, msg->passengers);
		} else {
			printf("UNKNOWN MSG TYPE\n");
		}
	} else {
		strcpy(db->trams[db->numTrams].tramId, msg->tramId);
		if(strcmp(msg->msgType, "LOCATION") == 0){
			strcpy(db->trams[db->numTrams].location, msg->location); 
			strcpy(db->trams[db->numTrams].passengers, "");
		} else if(strcmp(msg->msgType, "PASSENGER_COUNT") == 0) {
			strcpy(db->trams[db->numTrams].passengers, msg->passengers);
			strcpy(db->trams[db->numTrams].location, "");
		} else {
			printf("UNKNOWN MSG TYPE\n");
			strcpy(db->trams[db->numTrams].passengers, "");
			strcpy(db->trams[db->numTrams].location, "");
		}
		db->numTrams++;
	}
}

void display_tram_info(struct tramDb *db) {
	printf("\n========================================================\n");


	for(int i = 0; i < db->numTrams; i++){
		printf("%s:\n", db->trams[i].tramId);
		printf("\tLocation:%s\n", db->trams[i].location);
		printf("\tPassenger Count:%s\n", db->trams[i].passengers);
		printf("\n\n");
	}
	
	printf("\n========================================================\n");
}

struct tramMsg allocTramMsg(struct tramMsg *msg){
	strcpy(msg->tramId, "");
	strcpy(msg->location, "");
	strcpy(msg->passengers, "");
	strcpy(msg->msgType, "");
}

int main(int argc, char *argv[]){
	if(argc < 2){
        fprintf(stderr,"No port provided\n");
        exit(1);
	}	
	int sockfd, portno, n;
	char buffer[255];
	struct tramMsg msg;
	struct tramDb db;
	db.numTrams = 0;


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
		msg = allocTramMsg(&msg);
		extract_data_from_buffer(buffer, &msg);
		update_tram_db(&db, &msg);
		display_tram_info(&db);
	}
	
	return 0;
}
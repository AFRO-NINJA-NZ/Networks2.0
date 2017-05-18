//159.334 - Networks
// CLIENT: prototype for assignment 2.
//Note that this progam is not yet cross-platform-capable
// This code is different than the one used in previous semesters...
//************************************************************************/
//RUN WITH: Rclient_UDP 127.0.0.1 1235 0 0
//RUN WITH: Rclient_UDP 127.0.0.1 1235 0 1
//RUN WITH: Rclient_UDP 127.0.0.1 1235 1 0
//RUN WITH: Rclient_UDP 127.0.0.1 1235 1 1
//************************************************************************/

//---
#if defined __unix__ || defined __APPLE__
	#include <unistd.h>
	#include <errno.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <netdb.h>
    #include <iostream>
#elif defined _WIN32


	//Ws2_32.lib
	#define _WIN32_WINNT 0x501  //to recognise getaddrinfo()

	//"For historical reasons, the Windows.h header defaults to including the Winsock.h header file for Windows Sockets 1.1. The declarations in the Winsock.h header file will conflict with the declarations in the Winsock2.h header file required by Windows Sockets 2.0. The WIN32_LEAN_AND_MEAN macro prevents the Winsock.h from being included by the Windows.h header"
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	#include <iostream>
#endif

#include "myrandomizer.h"
#define GENERATOR 0x8005 //0x8005, generator for polynomial division

using namespace std;

#define WSVERS MAKEWORD(2,0)
#define BUFFER_SIZE 80  //used by receive_buffer and send_buffer
                        //the BUFFER_SIZE has to be at least big enough to receive the packet
#define SEGMENT_SIZE 78
//segment size, i.e., if fgets gets more than this number of bytes it segments the message into smaller parts.

WSADATA wsadata;
const int ARG_COUNT=5;
//---
int numOfPacketsDamaged=0;
int numOfPacketsLost=0;
int numOfPacketsUncorrupted=0;

int packets_damagedbit=0;
int packets_lostbit=0;
unsigned int send_CRC;
unsigned int recv_CRC;

unsigned int CRCpolynomial(char *buffer);
void extractTokens(char *str, unsigned int &CRC, char *command, int &packetNumber, char *data);


// Storing data in a custom vector

struct Data {
	string data;
	bool acked;
	clock_t timer;
};

class Client_vector {
private:
	Data *allData[200000];
	int count;
public:
	Client_vector() {
		count = 0;
		for (int i = 0; i<200000; ++i) {
			allData[i] = NULL;
		}
	};
	~Client_vector() {};
	void InsertLine(string data, int position);
	bool AckedStatus(int position) {
		//if (position < count) {
			return allData[position]->acked;
		//}
	}
	clock_t TimerValue (int position) {
		//if (position < count) {
			return allData[position]->timer;
		// }
	};
	void Print() {
		for (int i = 0; i<count; ++i) {
			 if (allData[i] == NULL) {
				 cout<<"NONE"<<endl;
			 }else {
				 cout<<i<<") "<<allData[i]->data<<endl;
			 }
		}
	}
	string GetData(int position) {
		return allData[position]->data;
	}


	void ResetTimer (int position) {
		allData[position]->timer = clock();
	}
	void UpdateACK (int position) {
		allData[position]->acked = true;
	}

	int GetCount() {return count;}

};

void Client_vector::InsertLine(string data, int position) {
	allData[position] = new Data();
	allData[position]->data = data;
	allData[position]->acked = false;
	allData[position]->timer = clock();
	count++;
}

/////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
//*******************************************************************
// Initialization
//*******************************************************************
   struct sockaddr_storage localaddr, remoteaddr;
   char portNum[NI_MAXSERV];
   struct addrinfo *result = NULL;
   struct addrinfo hints;

   memset(&localaddr, 0, sizeof(localaddr));  //clean up
   memset(&remoteaddr, 0, sizeof(remoteaddr));//clean up
   randominit();
   SOCKET s;
   char send_buffer[BUFFER_SIZE],receive_buffer[BUFFER_SIZE];
   int n,bytes,addrlen;

	 Client_vector *data_vector = new Client_vector();

	addrlen=sizeof(struct sockaddr);


	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

//********************************************************************
// WSSTARTUP
//********************************************************************
   if (WSAStartup(WSVERS, &wsadata) != 0) {
      WSACleanup();
      printf("WSAStartup failed\n");
   }
//*******************************************************************
//	Dealing with user's arguments
//*******************************************************************
   if (argc != ARG_COUNT) {
	   printf("USAGE: Rclient_UDP remote_IP-address remoteport allow_corrupted_bits(0 or 1) allow_packet_loss(0 or 1)\n");
	   exit(1);
   }

	int iResult=0;

	sprintf(portNum,"%s", argv[2]);
	iResult = getaddrinfo(argv[1], portNum, &hints, &result);

   packets_damagedbit=atoi(argv[3]);
   packets_lostbit=atoi(argv[4]);
   if (packets_damagedbit < 0 || packets_damagedbit > 1 || packets_lostbit< 0 || packets_lostbit>1){
	   printf("USAGE: Rclient_UDP remote_IP-address remoteport allow_corrupted_bits(0 or 1) allow_packet_loss(0 or 1)\n");
	   exit(0);
   }

//*******************************************************************
//CREATE CLIENT'S SOCKET
//*******************************************************************
   s = INVALID_SOCKET;
   s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

   if (s == INVALID_SOCKET) {
      printf("socket failed\n");
   	exit(1);
   }
    //nonblocking option
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the
	// socket based on the numerical value of iMode.
	// If iMode = 0, blocking is enabled;
	// If iMode != 0, non-blocking mode is enabled.
   u_long iMode=1;

   iResult=ioctlsocket(s,FIONBIO,&iMode);
   if (iResult != NO_ERROR){
         printf("ioctlsocket failed with error: %d\n", iResult);
		 closesocket(s);
		 WSACleanup();
		 exit(0);
   }

   cout << "==============<< UDP CLIENT >>=============" << endl;
   cout << "channel can damage packets = " << packets_damagedbit << endl;
   cout << "channel can lose packets = " << packets_lostbit << endl;

//*******************************************************************
//SEND A TEXT FILE
//*******************************************************************
   int counter=0;
   char temp_buffer[BUFFER_SIZE];
	 char SCRC[sizeof(unsigned int)];
   FILE *fin=fopen("data_for_transmission.txt","rb"); //original

//In text mode, carriage return�linefeed combinations
//are translated into single linefeeds on input, and
//linefeed characters are translated to carriage return�linefeed combinations on output.

   if(fin==NULL){
	   printf("cannot open data_for_transmission.txt\n");
	   closesocket(s);
   	   WSACleanup();
	   exit(0);
   } else {
	   printf("data_for_transmission.txt is now open for sending\n");
	}
   while (1){
		memset(send_buffer, 0, sizeof(send_buffer));//clean up the send_buffer before reading the next line
		if (!feof(fin)) {
			fgets(send_buffer,SEGMENT_SIZE,fin); //get one line of data from the file

			data_vector->InsertLine(send_buffer, counter);		//Adding data to vector

			sprintf(temp_buffer,"PACKET %d ",counter);  //create packet header with Sequence number
			send_buffer[strlen(send_buffer) - 2] = '\0';
			send_CRC = CRCpolynomial(send_buffer);   // Making CRC
			cout << "CRC is performed on \"" << send_buffer << "\"" << endl;
			counter++;
			sprintf(SCRC, "%d ", send_CRC);   // adding CRC
			strcat(temp_buffer, send_buffer);   //append data to packet header
			strcat(SCRC, temp_buffer);
			strcpy(send_buffer, SCRC);   //the complete packet
			printf("\n======================================================\n");
			cout << "calling send_unreliably, to deliver data of size " << strlen(send_buffer) << " where info is " << send_buffer << endl;
			send_unreliably(s,send_buffer,(result->ai_addr)); //send the packet to the unreliable data channel
			Sleep(1);  //sleep for 1 millisecond

			//CRC Packet %d
//********************************************************************
//RECEIVE
//********************************************************************
			unsigned int CRC = 0;   // starts as false
			char command[256];
			char data[256];
			int packetNumber = -1;
			unsigned int calculated_CRC = 0;
			addrlen = sizeof(remoteaddr); //IPv4 & IPv6-compliant
			memset(receive_buffer,0,sizeof(receive_buffer));
			bytes = recvfrom(s, receive_buffer, 78, 0,(struct sockaddr*)&remoteaddr,&addrlen);
			//cout << "Received YAYAYA" << endl;
			extractTokens(receive_buffer, CRC, command, packetNumber, data);
			calculated_CRC = CRCpolynomial(data);
			if (strncmp(receive_buffer,"ACK",3)==0) {
				if (calculated_CRC == CRC) {
					// Vector at position packetNumber changed to ACK
					data_vector->UpdateACK(packetNumber);
				}
				// If CRC doesnt match then timer takes care of corrupted ACK
			} else if (strncmp(receive_buffer,"NACK",4)==0) {
				memset(send_buffer,0,sizeof(send_buffer));
				memset(temp_buffer,0,sizeof(temp_buffer));
				memset(SCRC,0,sizeof(SCRC));
				// Resend data at vector position packetNumber
				string temp = data_vector->GetData(packetNumber);
				memcpy(send_buffer,temp.c_str(),temp.length());
				//sprintf(send_buffer, "%s ", data_vector->GetData(packetNumber));
				sprintf(temp_buffer,"PACKET %d ",packetNumber);  //create packet header with Sequence number
				send_CRC = CRCpolynomial(send_buffer);   // Making CRC
				sprintf(SCRC, "%d ", send_CRC);   // adding CRC
				strcat(temp_buffer, send_buffer);   //append data to packet header
				strcat(SCRC, temp_buffer);   // append packet|data to CRC
				strcpy(send_buffer, SCRC);   //the complete packet
				send_unreliably(s,send_buffer,(result->ai_addr));

				data_vector->ResetTimer(packetNumber);
			}

			int count = data_vector->GetCount();
			cout<<"\n\nCHECKING EXPIRED TIMER\n\n"<<endl;
			for (int i = 0; i<count; ++i) {
				if (!data_vector->AckedStatus(i)) {
					if ((clock() - data_vector->TimerValue(i)) > 10) {

						memset(send_buffer,0,sizeof(send_buffer));
						memset(temp_buffer,0,sizeof(temp_buffer));
						memset(SCRC,0,sizeof(SCRC));
						// Resend data at vector position packetNumber
						string temp = data_vector->GetData(i);
						memcpy(send_buffer,temp.c_str(),temp.length());
						//sprintf(send_buffer, "%s ", data_vector->GetData(packetNumber));
						sprintf(temp_buffer,"PACKET %d ",packetNumber);  //create packet header with Sequence number
						send_CRC = CRCpolynomial(send_buffer);   // Making CRC
						sprintf(SCRC, "%d ", send_CRC);   // adding CRC
						strcat(temp_buffer, send_buffer);   //append data to packet header
						strcat(SCRC, temp_buffer);   // append packet|data to CRC
						strcpy(send_buffer, SCRC);   //the complete packet
						send_unreliably(s,send_buffer,(result->ai_addr));

						data_vector->ResetTimer(i);
					}
				}
			}

//********************************************************************
//IDENTIFY server's IP address and port number.
//********************************************************************
	char serverHost[NI_MAXHOST];
    char serverService[NI_MAXSERV];
    memset(serverHost, 0, sizeof(serverHost));
    memset(serverService, 0, sizeof(serverService));


    getnameinfo((struct sockaddr *)&remoteaddr, addrlen,
                  serverHost, sizeof(serverHost),
                  serverService, sizeof(serverService),
                  NI_NUMERICHOST);



    printf("\nReceived a packet of size %d bytes from <<<UDP Server>>> with IP address: %s, at Port: %s\n", bytes, serverHost, serverService);

//********************************************************************
//PROCESS REQUEST
//********************************************************************
			//Remove trailing CR and LN
			if( bytes != SOCKET_ERROR ){
				n=0;
				while (n<bytes){
					n++;
					if ((bytes <= 0)) break;
					if (receive_buffer[n] == '\n') { /*end on a LF*/
						receive_buffer[n] = '\0';
						break;
					}
					if (receive_buffer[n] == '\r') /*ignore CRs*/
					receive_buffer[n] = '\0';
				}
				printf("RECEIVED --> %s, %d elements\n",receive_buffer, int(strlen(receive_buffer)));
			}

		} else {
			fclose(fin);
			printf("End-of-File reached. \n");
			memset(send_buffer, 0, sizeof(send_buffer));
			sprintf(send_buffer,"CLOSE \r\n"); //send a CLOSE command to the RECEIVER (Server)
			printf("\n======================================================\n");

			send_unreliably(s,send_buffer,(result->ai_addr));
			break;
		}

   } //while loop
	 cout<<"\nooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo\n"<<endl;
	 data_vector->Print();
	 cout<<"\nooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo\n"<<endl;
//*******************************************************************
//CLOSESOCKET
//*******************************************************************
   closesocket(s);
   printf("Closing the socket connection and Exiting...\n");
   cout << "==============<< STATISTICS >>=============" << endl;
   cout << "numOfPacketsDamaged=" << numOfPacketsDamaged << endl;
   cout << "numOfPacketsLost=" << numOfPacketsLost << endl;
   cout << "numOfPacketsUncorrupted=" << numOfPacketsUncorrupted << endl;
   cout << "===========================================" << endl;

   exit(0);
}

unsigned int CRCpolynomial(char *buffer){
	unsigned char i;
	unsigned int rem=0x0000;
    unsigned int bufsize=strlen(buffer);

	while(bufsize--!=0){
		for(i=0x80;i!=0;i/=2){
			if((rem&0x8000)!=0){
				rem=rem<<1;
				rem^=GENERATOR;
			} else{
	   	       rem=rem<<1;
		    }
	  		if((*buffer&i)!=0){
			   rem^=GENERATOR;
			}
		}
		buffer++;
	}
	rem=rem&0xffff;
	return rem;
}

void extractTokens(char *str, unsigned int &CRC, char *command, int &packetNumber, char *data){
	char * pch;
  int tokenCounter=0;
  printf ("Splitting string \"%s\" into tokens:\n\n",str);
  while (1)
  {
	 if(tokenCounter ==0){
       pch = strtok (str, " ,.-'\r\n'");
    } else {
		 	 pch = strtok (NULL, " ,.-'\r\n'");
	 }
	 if(pch == NULL) break;
	 printf ("Token[%d], with %d characters = %s\n",tokenCounter,int(strlen(pch)),pch);

	if (tokenCounter > 3) {
		strcat(data, " ");
		strcat(data, pch);
	}
    switch(tokenCounter){
      case 0: CRC = atoi(pch);
			     break;
      case 1: //command = new char[strlen(pch)];
			     strcpy(command, pch);

		        printf("command = %s, %d characters\n", command, int(strlen(command)));
              break;
		  case 2: packetNumber = atoi(pch);
		        break;
		  case 3: //data = new char[strlen(pch)];
			     strcpy(data, pch);

		        printf("data = %s, %d characters\n", data, int(strlen(data)));
              break;
    }

	 tokenCounter++;
  }
}

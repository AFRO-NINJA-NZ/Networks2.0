// TOKENIZER EXAMPLE
//
//
//NOTES
//
//strtok() doesn't create a new string and return it; 
//it returns a pointer to the token within the string you pass as argument to strtok(). 
//Therefore the original string gets affected.
//strtok() breaks the string means it replaces the delimiter character with NULL and 
//returns a pointer to the beginning of that token. 
//Therefore after you run strtok() the delim characters will be replaced by NULL characters. 

#include <stdio.h>
#include <string.h>
#include <cstdlib> //used by atoi()
#include <iostream> 
#include <vector> 

using namespace std;

void removeCrLn(char *send_buffer){
	unsigned int n=0;
	while (n < strlen(send_buffer)){
		
		
		if (send_buffer[n] == '\n') { /*end on a LF*/
			send_buffer[n] = '\0';
			break;
		}
		if (send_buffer[n] == '\r') /*ignore CRs*/
		send_buffer[n] = '\0';
		n++;
	}
}

void extractTokens(char *str, int &CRC, char *&command, int &packetNumber, char *&data){
	char * pch;
    
  //~ int CRC;  	
  //~ char *packetHeader;
  //~ int packetNumber;
  //~ char *data;	
  
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
	 printf ("Token[%d], with %d characters = %s\n",tokenCounter,strlen(pch),pch);
	 
    switch(tokenCounter){
      case 0: CRC = atoi(pch);
			     break;
      case 1: command = new char[strlen(pch)];
			     strcpy(command, pch);
		
		        printf("command = %s, %d characters\n", command, strlen(command));
              break;			
		case 2: packetNumber = atoi(pch);
		        break;
		case 3: data = new char[strlen(pch)];
			     strcpy(data, pch);
		
		        printf("data = %s, %d characters\n", data, strlen(data));
              break;			
    }	
	  
	 tokenCounter++;
  }
}

void test_extractTokens(){
  char str[256] ="12345 PACKET 9 data56789\r\n";

  int CRC;  	
  
  int packetNumber;
  char *data;
  char *command;
 
	
  extractTokens(str, CRC, command, packetNumber, data);
  
  printf("\n==== << Extracted Information >> =======\n");
  printf("CRC = %d\n", CRC);
  printf("command = %s, size = %d\n", command, strlen(command));
  printf("packetNumber = %d\n", packetNumber);
  printf("data = [%s], size = %d\n", data,strlen(data));
  printf("original_str = [%s], size = %d\n", str,strlen(str));
	
  delete command;
  delete data;
  printf("\n========================================\n");
  printf("(after deletions) original_str = [%s], size = %d\n", str,strlen(str));
  
}

void test_removeCrLn(){
  char str[256] ="12345 PACKET 9 data56789\r\n";


	
  printf("original_str = [%s], size = %d\n", str,strlen(str));
  removeCrLn(str);
  
  printf("\n========================================\n");
  printf("(after deletions) original_str = [%s], size = %d\n", str,strlen(str));
  
}



void extractMTokens(char *str, vector<int>& tokens){
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
	 printf ("Token[%d], with %d characters = %s\n",tokenCounter,strlen(pch),pch);
	 
	 tokens.push_back(atoi(pch));
	  
	 tokenCounter++;
  }
}


void test_extractMTokens(){
  char str[256] ="123 456 789\r\n";

  vector<int> data;
 
 
	
  extractMTokens(str, data);
  
  printf("\n==== << Extracted Information >> =======\n");
  
	for(unsigned int i=0; i < data.size(); i++){
		cout << "data[" << i << "]= " << data[i] << endl;
	}
	
  printf("original_str = [%s], size = %d\n", str,strlen(str));

  data.clear();
	
  printf("\n========================================\n");
  printf("(after deletions) original_str = [%s], size = %d\n", str,strlen(str));
  
}


int main ()
{
  //test_extractTokens();
	test_extractMTokens();
  
  //test_removeCrLn();
  return 0;
}

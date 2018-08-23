#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <netdb.h>

#define SERVERPORT 5581

void main(){


struct ifreq ifr;
struct hostent *server;
struct sockaddr_in servaddr, cliaddr;
int sd, rc;

server = gethostbyname("169.254.115.6");

/* Create the socket */
sd = socket(AF_INET, SOCK_DGRAM, 0);
if (sd < 0) 
{
    printf("Error in socket() creation\n");
}

/* Bind to vlan100 interface only - this is a private VLAN */

memset(&ifr, 0, sizeof(ifr));
snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "vlan100");
if ((rc = setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr))) < 0)
{
    perror("Server-setsockopt() error for SO_BINDTODEVICE");
    close(sd);
    exit(-1);
}

/* bind to an address */
memset(&servaddr, 0x00, sizeof(struct sockaddr_in));
servaddr.sin_family = AF_INET;
bcopy((char*)server->h_addr, (char*)&servaddr.sin_addr.s_addr, server->h_length);

servaddr.sin_port = htons(SERVERPORT);
//serveraddr.sin_addr.s_addr = INADDR_ANY;

#if 0
rc = bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr));

if(rc<0)
	printf("Failed to bind\n");
#endif

char buffer[1024] = "@DeathFire\0";
while(1){
	sendto(sd, buffer, strlen(buffer), 0, &servaddr, sizeof(servaddr));
sleep(2);
}
}

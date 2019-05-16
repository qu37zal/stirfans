/*
*  Copyright (C) 2009-2010, Marvell International Ltd.
*  All Rights Reserved.
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUF 4096
#define LOCATION_HDR "Location:"
#define SSDP_ADDR "239.255.255.250"
#define SSDP_PORT 1900

unsigned int quiet = 0;
char ip[16];
fpos_t pos;

void parse_options(int argc, char *argv[])
{
  int i = 1;
  memset(ip, 0, sizeof(ip));

  while (i < argc) {
    if (!strcmp(argv[i], "--quiet"))
    quiet = 1;
    else if (!strcmp(argv[i], "--ip")) {
      snprintf(ip, sizeof(ip), "%s", argv[i + 1]);
      i++;
    } else
    printf("Ignoring unknown option %s\n", argv[i]);

    i++;
  }
}

char *trim(char *str)
{
	char *end;
	while((unsigned char)*str==32) str++;
	if(*str == 0)
		return str;
	end = str + strlen(str) - 1;
	while(end > str && (unsigned char)*end==32) end--;
	end[1] = '\0';

	return str;
}

int main(int argc, char *argv[])
{
  int sock, ret, one = 1, len, ttl = 3;
  struct sockaddr_in cliaddr, destaddr;
  struct timeval tv;
  char buffer[MAX_BUF] = "TYPE: WM-DISCOVER\r\nVERSION: 1.0\r\n\r\nservices: com.marvell.wm.system*\r\n\r\n";
  char *token;
  int count = 0;
  struct ip_mreq mc_req;

  parse_options(argc, argv);

  /* Create socket */
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    printf("%s: Cannot open socket \n", argv[0]);
    exit(1);
  }

  /* Allow socket resue */
  ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
  sizeof(one));
  if (ret < 0) {
    printf("%s: Cannot prepare socket for reusing\n", argv[0]);
    exit(1);
  }

  /* Set receive timeout */
  tv.tv_sec = 3;  /* 3 second timeout */
  tv.tv_usec = 0;

  ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv,
  sizeof(tv));
  if (ret < 0) {
    printf("%s: Cannot set receive timeout to the socket (errno %d)\n", argv[0], ret);
    //exit(1);
  }

  ret = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl,
  sizeof(ttl));
  if (ret < 0) {
    printf("%s: Cannot set ttl to the socket\n", argv[0]);
    exit(1);
  }

  /* construct a socket bind address structure */
  cliaddr.sin_family = AF_INET;
  if (strlen(ip) > 0)
  cliaddr.sin_addr.s_addr = inet_addr(ip);
  else
  cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  cliaddr.sin_port = htons(0);

  ret = bind(sock, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
  if (ret < 0) {
    printf("%s: Cannot bind port\n", argv[0]);
    exit(1);
  }

  /* construct an IGMP join request structure */
  mc_req.imr_multiaddr.s_addr = inet_addr(SSDP_ADDR);
  mc_req.imr_interface.s_addr = htonl(INADDR_ANY);

  /* send an ADD MEMBERSHIP message via setsockopt */
  if ((setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
    (void*) &mc_req, sizeof(mc_req))) < 0) {
      perror("setsockopt() failed");
      exit(1);
    }

    /* Set destination for multicast address */
    destaddr.sin_family = AF_INET;
    destaddr.sin_addr.s_addr = inet_addr(SSDP_ADDR);
    destaddr.sin_port = htons(SSDP_PORT);

    /* Send the multicast packet */
    len = strlen(buffer);
    ret = sendto(sock, buffer, len, 0, (struct sockaddr *)&destaddr,
    sizeof(destaddr));
    if (ret < 0) {
      printf("%s: Cannot send data\n", argv[0]);
      exit(1);
    }

    /* quiet the noise */
    if (quiet == 0)
    printf
    ("Sent the SSDP multicast request and now waiting for a response..\n");

    if (quiet == 0)
    printf
    ("Preparing config file..\n");

    FILE *outFile = fopen("./config.json","wt");
    fprintf(outFile,"[ ");


    while (1) {
      /* Wait for response */
      len = sizeof(destaddr);
      ret =
      recvfrom(sock, buffer, MAX_BUF, 0,
        (struct sockaddr *)&destaddr, &len);

        if (ret == -1) /* time out */
        break;
        count++; /* Valid response */

        /* Parse the response */
        token = strtok(buffer, "\r\n");
        while (token != NULL) {
          if (!strncasecmp(token, LOCATION_HDR, strlen(LOCATION_HDR))) {
            printf("Found a wireless microcontroller, base URI: %s\n",
            token + strlen(LOCATION_HDR));
 	    fprintf(outFile,"\"%s\", ",trim(token+strlen(LOCATION_HDR)));
            break;
          }
          token = strtok(NULL, "\r\n");
        }
      }
    fseek(outFile,-2,SEEK_END);
    fprintf(outFile," ]\n");
    fclose(outFile);

      /* send a DROP MEMBERSHIP message via setsockopt */
      if ((setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
        (void*) &mc_req, sizeof(mc_req))) < 0) {
          perror("setsockopt() failed");
          exit(1);
        }
        if (quiet == 0)
        printf("Found %d device(s)\n", count);

        return 0;
      }

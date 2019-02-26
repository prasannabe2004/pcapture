/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>

#define DEST_MAC0	0x00
#define DEST_MAC1	0x00
#define DEST_MAC2	0x00
#define DEST_MAC3	0x00
#define DEST_MAC4	0x00
#define DEST_MAC5	0x00

#define ETHER_TYPE	0x0800

#define DEFAULT_IF	"enp0s3"
#define BUF_SIZ		1024
int print_ip(struct iphdr *iph)
{
    struct sockaddr_in source,dest;
	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = iph->saddr;
	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = iph->daddr;

    printf("IP Header:\n");
    printf("  Version=%d TOS=%d TTL=%d\n",iph->version,iph->tos,iph->ttl);
    printf("  Protocol=%d TotLen=%d\n",iph->protocol,iph->tot_len);
    printf("  Src=%s Dst=%s\n",inet_ntoa(source.sin_addr),inet_ntoa(dest.sin_addr));

    return iph->protocol;
}
void print_tcp(struct tcphdr *tcph)
{
    printf("TCP Header: \n");
    printf("  SrcPort=%d DstPort=%d\n", tcph->source,tcph->dest);
}
void print_udp(struct udphdr *udph)
{
    printf("UDP Header\n");
    printf("  SrcPort=%d DstPort=%d\n", udph->source,udph->dest);
}
void print_icmp(struct icmphdr *icmph)
{
    printf("ICMP Header\n");
    printf("  Code =%d Type=%d\n",icmph->code,icmph->type);
}
int main(int argc, char *argv[])
{
	char sender[INET6_ADDRSTRLEN];
	int sockfd, ret, i;
	int sockopt;
	ssize_t numbytes;
	struct ifreq ifopts;	/* set promiscuous mode */
	struct ifreq if_ip;	/* get ip addr */
	struct sockaddr_storage their_addr;
	uint8_t buf[BUF_SIZ];
	char ifName[IFNAMSIZ];
	int proto;

	/* Get interface name */
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);

	/* Header structures */
	struct ether_header *eh = (struct ether_header *) buf;
	struct iphdr *iph = (struct iphdr *) (buf + sizeof(struct ether_header));
	struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));
    struct tcphdr *tcph = (struct tcphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));
    struct icmphdr *icmph = (struct icmphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));

	memset(&if_ip, 0, sizeof(struct ifreq));

	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
		perror("listener: socket");	
		return -1;
	}

	/* Set interface to promiscuous mode - do we need to do this every time? */
	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	/* Bind to device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
		perror("SO_BINDTODEVICE");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

    while(1)
    {
        printf("listener: Waiting to recvfrom...\n");
	    numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
	    printf("listener: got packet %lu bytes\n", numbytes);
        
        // IP Header
        proto = print_ip(iph);
        if(proto == IPPROTO_TCP)
        {
            print_tcp(tcph);
        }
        else if(proto == IPPROTO_UDP)
        {
            print_udp(udph);
        }
        else if(proto == IPPROTO_ICMP)
        {
            print_icmp(icmph);
        }

	    /* Print packet */
    }
	close(sockfd);
	return 0;
}
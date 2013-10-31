#include <pcap.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netinet/if_ether.h>
#define ETHER_HDRLEN 14
struct my_ip 
{
	u_int8_t	ip_vhl;		/* header length, version */
#define IP_V(ip)	(((ip)->ip_vhl & 0xf0) >> 4)
#define IP_HL(ip)	((ip)->ip_vhl & 0x0f)
	u_int8_t	ip_tos;		/* type of service */
	u_int16_t	ip_len;		/* total length */
	u_int16_t	ip_id;		/* identification */
	u_int16_t	ip_off;		/* fragment offset field */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
	u_int8_t	ip_ttl;		/* time to live */
	u_int8_t	ip_p;		/* protocol */
	u_int16_t	ip_sum;		/* checksum */
	struct	in_addr ip_src,ip_dst;	/* source and dest address */
};

u_int16_t handle_ethernet(u_char *args,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
    u_int caplen = pkthdr->caplen;
    u_int length = pkthdr->len;
    struct ether_header *eptr;  /* net/ethernet.h */
    u_short ether_type;

    if (caplen < ETHER_HDRLEN)
    {
        fprintf(stdout,"Packet length less than ethernet header length\n");
        return -1;
    }

    /* lets start with the ether header... */
    eptr = (struct ether_header *) packet;
    ether_type = ntohs(eptr->ether_type);

    /* Lets print SOURCE DEST TYPE LENGTH */
    fprintf(stdout,"ETH: ");
    fprintf(stdout,"SRC: %s "
            ,ether_ntoa((struct ether_addr*)eptr->ether_shost));
    fprintf(stdout,"DST: %s "
            ,ether_ntoa((struct ether_addr*)eptr->ether_dhost));

    /* check to see if we have an ip packet */
    if (ether_type == ETHERTYPE_IP)
    {
        fprintf(stdout,"(IP)");
    }
    else  if (ether_type == ETHERTYPE_ARP)
    {
        fprintf(stdout,"(ARP)");
    }
    else  if (eptr->ether_type == ETHERTYPE_REVARP)
    {
        fprintf(stdout,"(RARP)");
    }
    else {
        fprintf(stdout,"(?)");
    }
    fprintf(stdout," %d\n",length);

    return ether_type;
}

u_char* handle_IP
        (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet)
{
    const struct my_ip* ip;
    u_int8_t length = pkthdr->len;
    //printf("Packet Header length=%d %d\n",length, sizeof(struct my_ip));
    u_int8_t hlen,off,version;
    u_int8_t tos = 0;
    int i;
    u_int8_t protocol = 0xFF, ttl = 0;
    int len,id,checksum;

    /* jump pass the ethernet header */
    ip = (struct my_ip*)(packet + sizeof(struct ether_header));
    length -= sizeof(struct ether_header); 

    /* check to see we have a packet of valid length */
    if (length < sizeof(struct my_ip))
    {
        printf("truncated ip %d",length);
        return NULL;
    }

    printf("recv len=%d\n",ip->ip_len);
    len     = ntohl(ip->ip_len);
    ttl     = ip->ip_ttl;
    id     = ntohs(ip->ip_id);
    hlen    = IP_HL(ip); /* header length */
    version = IP_V(ip);/* ip version */
    tos = ntohs(ip->ip_tos);
    protocol = ip->ip_p;
    checksum = ntohs(ip->ip_sum);

    /* check version */
    if(version != 4)
    {
      fprintf(stdout,"Unknown version %d\n",version);
      return NULL;
    }

    /* check header length */
    if(hlen < 5 )
    {
        fprintf(stdout,"bad-hlen %d \n",hlen);
    }

    /* see if we have as much packet as we should */
    if(length < len)
        printf("\ntruncated IP - %d(%d - %d) bytes missing\n",len - length,len,length);

    /* Check to see if we have the first fragment */
    off = ntohs(ip->ip_off);
    if((off & 0x1fff) == 0 )/* aka no 1's in first 13 bits */
    {/* print SOURCE DESTINATION hlen version len offset */
        fprintf(stdout,"IP: ");
        fprintf(stdout,"VER:%d ", version);
        fprintf(stdout,"HLEN(x8):%d ", hlen);
        fprintf(stdout,"TOS:%d ", tos);
        fprintf(stdout,"LEN:%d ", len);
        //fprintf(stdout,"ID:%d ", id);
        fprintf(stdout,"OFF:%d ", off);
        fprintf(stdout,"TTL:%d ", ttl);
        fprintf(stdout,"PROTO:%d ", protocol);
        fprintf(stdout,"CSUM:%d ", checksum);
        fprintf(stdout,"SRC:%s ", inet_ntoa(ip->ip_src));
        fprintf(stdout,"DST:%s \n", inet_ntoa(ip->ip_dst));
    }
    switch(protocol)
    {
        case IPPROTO_UDP:
          printf("UDP Packet received\n");
          break; 
        case IPPROTO_TCP:
          printf("TCP Packet received\n");
          break;
	default:
	  break; 
    }
    return NULL;
}


void my_callback(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char* packet) 
{ 
    u_int16_t type = handle_ethernet(args,pkthdr,packet);

    if(type == ETHERTYPE_IP)
    {/* handle IP packet */
        handle_IP(args,pkthdr,packet);
    }else if(type == ETHERTYPE_ARP)
    {/* handle arp packet */
    }
    else if(type == ETHERTYPE_REVARP)
    {/* handle reverse arp packet */
    }

}

int main(int argc,char **argv) 
{ 
    int i;
    char *dev; 
    char errbuf[PCAP_ERRBUF_SIZE]; 
    pcap_t* descr; 
    const u_char *packet; 
    struct pcap_pkthdr hdr;
    struct ether_header *eptr;    /* net/ethernet.h */
    struct bpf_program fp;        /* hold compiled program */
    bpf_u_int32 maskp;            /* subnet mask */
    bpf_u_int32 netp;             /* ip */
 
    if(argc != 2){
        fprintf(stdout, "Usage: %s \"expression\"\n"
            ,argv[0]);
        return 0;
    } 
 
    /* Now get a device */
    dev = pcap_lookupdev(errbuf); 
     
    if(dev == NULL) {
        fprintf(stderr, "%s\n", errbuf);
        exit(1);
    } 
        /* Get the network address and mask */
    pcap_lookupnet(dev, &netp, &maskp, errbuf); 
 
    /* open device for reading in promiscuous mode */
    descr = pcap_open_live(dev, BUFSIZ, 1,-1, errbuf); 
    if(descr == NULL) {
        printf("pcap_open_live(): %s\n", errbuf);
        exit(1);
    } 
    /* Now we'll compile the filter expression*/
    if(pcap_compile(descr, &fp, argv[1], 0, netp) == -1) {
        fprintf(stderr, "Error calling pcap_compile\n");
        exit(1);
    } 
 
    /* set the filter */
    if(pcap_setfilter(descr, &fp) == -1) {
        fprintf(stderr, "Error setting filter\n");
        exit(1);
    } 
 
    /* loop for callback function */
    pcap_loop(descr, -1, my_callback, NULL); 
    return 0; 
}


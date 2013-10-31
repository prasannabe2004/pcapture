#include <stdio.h>
#include <libipq.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <net/if.h>
#include <linux/netfilter.h>
int cb(struct nfq_q_handle *h, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
        printf("Packet received \n");
int id = ntohl(nfq_get_msg_packet_hdr(nfa)->packet_id);
	nfq_set_verdict(h, id, NF_QUEUE_NR(1), 0, NULL);

}

int main(int argc, char** argv) {
    int fd;
    ssize_t rv;
    char buf[4096];
    struct nfq_handle* h;
    struct nfq_q_handle* qh;
int i=0;
    h = nfq_open();
    if (!h) {
        fprintf(stderr, "error during nfq_open()\n");
        exit(1);
    }

    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_unbind_pf()\n");
        exit(1);
    }

    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_bind_pf()\n");
        exit(1);
    }

    printf("Binding to queue 0...\n");
    qh = nfq_create_queue(h, 0, &cb, NULL);
    if (!qh) {
        fprintf(stderr, "error during nfq_create_queue()\n");
        exit(1);
    }

    printf("Copying packets...\n");
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "error during nfq_set_mode()\n");
        exit(1);
    }

    fd = nfq_fd(h);

    memset(buf, 0, 4096);
    while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) 
    {
      for (i = 0; i < rv; i++) printf("%02x\n", *(buf+i));
      printf("\n\n");
      nfq_handle_packet(h, buf, rv);
    }

    nfq_destroy_queue(qh);
    nfq_close(h);
}


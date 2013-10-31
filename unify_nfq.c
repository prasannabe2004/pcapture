#include <stdio.h>
#include <libipq.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <net/if.h>
#include <linux/netfilter.h>
#define BUFSIZE 70000

int nfqCallback(struct nfq_q_handle *h, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) 
{
	printf("Packet received \n");

}

int main()
{
	struct nfq_handle *h;
	struct nfq_q_handle *qh;
	struct nfnl_handle *nh;
	struct timeval tv;
	int fd;
	int rv;
	char buf[BUFSIZE];

	printf("Netcard: Creating nfq_handle\n");
	h = nfq_open();
	if (!h) {

		printf("Netcard: Error..Exiting...1\n");
		exit(1);
	}

	if (nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}

	printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if (nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}

	printf("binding this socket to queue '1'\n");
	qh = nfq_create_queue(h, 1, &nfqCallback, NULL);
	if (!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}

	nh = nfq_nfnlh(h);
	fd = nfnl_fd(nh);

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) == -1) {
		printf("[%d] can't set socket timeout: %s\n", getpid(), strerror(errno));
		exit(1);
	}
	printf("PMP: [GW-NETCARD] Listening for packet at Netcard \n");
	while (1) {
		rv = recv(fd, buf, BUFSIZE, 0);

		if (rv < 0) {
		} else {
			nfq_handle_packet(h, buf, rv);
		}
	}

	printf("Netcard: unbinding from queue\n");
	printf("unbinding from queue 0\n");
	nfq_destroy_queue(qh);

	printf("closing library handle\n");
	nfq_close(h);

	return 0;
}


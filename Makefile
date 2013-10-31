#export LD_LIBRARY_PATH=/usr/local/lib
#iptables -A INPUT -i eth0 -p icmp -j QUEUE
#iptables -D INPUT -i eth0 -p icmp -j QUEUE

TARGET=my_tcpdump raw_icmp raw_ip 
all: $(TARGET)
default: $(TARGET)
internet_nfq1:
	gcc internet_nfq1.c /usr/local/lib/libnetfilter_queue.so.1.1.0 -o internet_nfq1
internet_nfq2:
	gcc internet_nfq2.c /usr/local/lib/libnetfilter_queue.so.1.1.0 -o internet_nfq2
my_tcpdump:
	gcc my_tcpdump.c -lpcap -o my_tcpdump
raw_icmp:
	gcc raw_icmp.c -o raw_icmp
raw_ip:
	gcc raw_ip.c -o raw_ip
clean:
	rm -f unify internet_nfq1 internet_nfq2 my_tcpdump raw_icmp raw_ip




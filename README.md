# CPE464 - Intro to Computer Networks

Labs: 
* Lab 1 - Intro to Networks and Wireshark
* Lab 2 - Working with Sockets

Projects: 
* Project 1 - Trace
* Project 2 - CCLient and Server

# Labs
1. Intro to Networks and Wireshark
* Intro to using Wireshark and its various features
* Analyzing PCAP packets and the various headers within packets
2. Working with Sockets
* Intro to Project 2
* Writing a simple client and server in C
* Implementing specialized `sendPDU()` and `recvPDU()` functions

# Projects

1. **Trace**
* A program that outputs protocol header information for a number of different types of headers
* Header types include: PCAP Packets, Ethernet Type II, ARP, IPv4, TCP, ICMP, and UDP

2. **C CLient and Server**
* A client and server written in C that comunicate using TCP and a custom packet implementation. 
* The server can accept connections from multiple clients, and route packets to and from clients appropriately. 
* The server keeps track of all connected clients using a dynamic hashing table combined with a liked list such that searching for user info is always O(1). 
* Clients use commands to send messages, multicast messages, and broadcasts, and the client may also request user info from the server. 

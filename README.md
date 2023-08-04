# CPE 464 - Intro to Computer Networks

## WARNING TO ALL STUDENTS: 
* **If you are currently enrolled in CSC/CPE 464, you probably shouldn't poke around in here.**
* **If you ever intend on enroling in CSC/CPE 464, you probably shouldn't poke around in here.**

Labs: 
* Lab 1 - Intro to Networks and Wireshark
* Lab 3 - Intro to CCLient and Server 
* Lab 7 - Intro to RCopy and Server 

Projects: 
* Project 1 - Trace
* Project 2 - CCLient and Server
* Project 3 - RCopy and Server

# Labs
1. Intro to Networks and Wireshark
* Intro to using Wireshark and its various features
* Analyzing PCAP packets and the various headers within packets
2. Intro to CClient and Server
* Intro to Project 2
* Implementing a basic TCP client and server with the goal of creating a chat network
3. Intro to RCopy and Server
* Intro to Project 3
* Implementing a basic UDP client and server with the goal of implementing a file transfer system

# Projects

1. **Trace**
* A program that outputs protocol header information for a number of different types of headers
* Header types include: PCAP Packets, Ethernet Type II, ARP, IPv4, TCP, ICMP, and UDP

2. **CCLient and Server**
* A client and server written in C that comunicate using TCP and a custom packet implementation. 
* The server can accept connections from multiple clients, and route packets to and from clients appropriately. 
* The server keeps track of all connected clients using a dynamic hashing table combined with a liked list such that searching for user info is always O(1). 
* Clients use commands to send messages, multicast messages, and broadcasts, and the client may also request user info from the server.

3. **RCopy and Server**
* A client and server written in C using UDP and a custom `send`/`recv` implementation.
* The server can accept connections from multiple rcopy clients using child sub-processies.
* The server recieves files and stores them locally, allowing for clients to request the files at a later date. 

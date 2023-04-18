
#ifndef PROJECT_2_USER_PACKET_H
#define PROJECT_2_USER_PACKET_H

#include <stdint.h>

/*
 * USER LEVEL PACKET FORMAT
 * All PDUs start with a 2-byte length field in network order and then a flag value as discussed below.
 * These three bytes are referred to as the chat header.
 */

/*
 * 2: Sent from server back to cclient
 *  - Confirming a good handle
 *  - This is a positive response to flag = 1
 *  - Format: 3-byte chat-header
 *
 * 3: Sent from server back to cclient
 *  - This is an error response to a flag = 1
 *  - Error on initial packet (handle already exists)
 *  - When the cclient receives this it prints out an error message and terminates
 *  - Format: 3-byte chat-header
 *
 * 8: Client to server when the client is exiting
 *  - Client program cannot terminate until it receives a flag = 9 from the server.
 *  - Client should not block while it is waiting for the Flag = 9 response packet.
 *  - Client needs to process any incoming messages until the Flag = 9 response arrives.
 *  - Format: chat-header
 *
 * 9: Server to client
 *  - ACK the clients exit (flag = 8) packet
 *  - After receiving this, the client can terminate
 *  - Format: chat-header
 *
 * 10: Client to server
 *  - Client requesting the list of handles. (So %L command)
 *  - After sending the Flag = 10, client must continue processing any other messages that come in until the Flag = 11 message arrives.
 *  - Format: chat-header
 *
 * 13: Server to client
 *  - This packet tells the client the %L is finished.
 *  - Immediately follows the last [FLAG 12] packet
 *  - Format: chat-header
 */

/* 3 bytes (12 bits) */
struct __attribute__((packed)) chat_header {    /* Offset */
    uint8_t len;                                /*      0 */
    uint8_t flag;                               /*      8 */
};

/*
 * 1: Sent from cclient to server
 *  - Client initial packet to the server
 *  - cclient blocks until it receives a packet with Flag = 2 or Flag = 3
 *  - Format: 3-byte chat-header, then 1 byte handle length then the handle (no nulls/padding)
 *
 * 7: From server to cclient
 *  - Error packet, if a destination handle in a message/multicast packet (flag = 5 or flag = 6) does not exist.
 *  - You should send one Flag=7 packet for each handle in error (so if a message contains 4 destination handles
 *    and 2 of them are invalid the server will send back two different Flag=7 packets).
 *  - Format: chat-header then 1 byte handle length then handle of the destination client (the one not found on the
 *    server) as specified in the flag = 5 or flag = 6 message.
 *
 * 12: Server to client
 *  - Sent immediately following the [FLAG 11] packet.
 *  - Each [FLAG 12] packet contain one handles currently registered at the server.
 *  - There will be one [FLAG 12] packet per handle.
 *  - The handles are sent one right after the other.
 *  - The server will not send any other packets until all the handles have been sent.
 *    eg: [FLAG 12] chat-header, 1 byte handle len, handle, chat-header...
 *  - After the [FLAG 12] message header arrives, you will only receive the handle data ([FLAG 12] packets)
 *    until all handles have been sent.
 *  - No other message should be intertwined with sending the handles from the server to the client.
 *  - You do not need to process any STDIN input while processing the Flag = 12 messages.
 *  - Format: chat-header, then one handle in the format of: (1 byte handle length, then the handle) (no null/padding).
 * */

/* 104 bytes (832 bits) */
struct __attribute__((packed)) handshake_pkt {  /* Offset */
    uint8_t len;                                /*      0 */
    uint8_t flag;                               /*      8 */
    uint8_t handle_len;                         /*     16 */
/*  char handle[MAX_HANDLE_LEN]; */             /*     24 */
};

/*
 * 4: Sent from cclient to all other clients via the server
 *  - Broadcast packet (%B)
 *  - Format:
 *    - Normal 3 byte chat-header (length, flag) (flag = 4)
 *    - 1 byte containing the length of the sending clients handle
 *    - Handle name of the sending client (no nulls or padding allowed)
 *    - Text message (a null terminated string)
 *    - This packet type does not have a destination handle since the packet should be forwarded to all other clients.
 */

/* 104 bytes (832 bits) */
struct __attribute__((packed)) broadcast_pkt {  /* Offset */
    uint8_t len;                                /*      0 */
    uint8_t flag;                               /*      8 */
    uint8_t handle_len;                         /*     16 */
/*  char handle[MAX_HANDLE_LEN]; */             /*     24 */
/*  char message[MAX_HANDLE_LEN]; */
};

/*
 * 5: Sent from cclient to another cclient via the server
 *  - Message packet (%M)
 *  - Format:
 *    - Normal 3 byte chat-header (message packet length (2 byte), flag (1 byte))
 *    - 1-byte containing the length of the sending clients handle
 *    - Handle name of the sending client (no nulls or padding allowed)
 *    - 1 byte with the value 1 (one) to specify that one destination handle is contained in this message. %M can only send to 1 other handle. This field with the value 1 (one) is to allow the %M and %C packets to have the same basic format and therefore simplify the processing of the %M and %C packets.
 *    - The destination handle in the format:
 *      - 1 byte containing the length of the handle of the destination client you want to talk with3
 *      - Handle name of the destination client (no nulls or padding allowed)
 *    - Text message (a null terminated string)
 *    - The sending cclient does not block waiting for a response from the server after sending the %M message
 */

/* 104 bytes (832 bits) */
struct __attribute__((packed)) message_pkt {
    uint8_t len;
    uint8_t flag;
    uint8_t src_handle_len;
/*  char src_handle[MAX_HANDLE_LEN]; */
    uint8_t num_dst;
    uint8_t dst_handle_len;
/*  char dst_handle[MAX_HANDLE_LEN]; */
};

/*
 * 6: Sent from the cclient to multiple other clients
 *  – Multicast packet, see format above under the %C (multicast) command.
 */

/*
 * 11: Server to client
 *  - Responding to flag = 10, giving the client the number of handles stored on the server.
 *  - Format: chat-header, 4 byte number (integer, in network order) stating how many handles are
 *    currently known by the server.
 */

/* 104 bytes (832 bits) */
struct __attribute__((packed)) num_handles_pkt {    /* Offset */
    uint8_t len;                                    /*      0 */
    uint8_t flag;                                   /*      8 */
    uint16_t num_handles;                           /*     16 */
};

#endif /* PROJECT_2_USER_PACKET_H */

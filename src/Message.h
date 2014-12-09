/*
 * Message.h
 *
 *  CS 6380, Distributed Computing
 *  Fall 2014
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 *
 *  Updated for Project #2:
 *  Added the following message types:
 *      MSG_ACK, MSG_INITIATE, MSG_REPORT, MSG_TEST, MSG_ACCEPT,
 *      MSG_REJECT, MSG_CHANGEROOT, MSG_CONNECT, MSG_CONNECT2ME
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <string>
#include "Edge.h"
#include "Neighbor.h"
/*
 * class Message:
 *     This creates an abstraction of a Message that can be sent and
 *     received between peers or between a child and the master.
 */
class Message {
    char buf[1024];
    int rcvd;
public:
    enum MsgType {
    	MSG_NULL=0,         // Sent in absence of a real message
		MSG_TICK,           // Sent from master->child
		MSG_ACK,            // Acknowledge the tick...
		MSG_DONE,           // Sent from child->master
		MSG_INITIATE,       // Sent from leader->leaf.  Start phase of AsyncGHS
		MSG_REPORT,         // Sent from leaf->leader.  Report my MWOE
		MSG_TEST,           // Send over MWOE
		MSG_ACCEPT,         // I'm in a different component
		MSG_REJECT,         // I'm in the same component
		MSG_CHANGEROOT,     // I'm your new leader now...
		MSG_CONNECT,        // You, leaf! Connect!!!
		MSG_CONNECT2ME      // Connect with me, please!
                 } msgType;

    int  round;
    int  level;
    Edge edge;

    Neighbor sentBy;

    Message() : msgType(MSG_NULL), round(0), level(0), sentBy{-1,-1,-1} {};
    Message( const Message& ) = default;
    Message( Message&& );

    // Read a message from a file descriptor
    Message( int fd );

    Message& operator=( const Message& m ) = default;

    // Create a message
    Message( enum MsgType mt, int _r=-1 ) :
    	msgType(mt), round(_r), level(0), edge(), sentBy{-1,-1,-1} {};
    Message( enum MsgType mt, int r, Edge e, int l=0 )
        : msgType(mt), round(r), level(l), edge(e), sentBy{-1,-1,-1} {};
    Message( enum MsgType mt, Edge e, int l=0 ) :
        msgType(mt), round(), level(l), edge(e), sentBy{-1,-1,-1} {};

    // Send this message to the named file descriptor
    int send( int fd );

    // Generate a string representation of this message
    std::string toString() const;

    std::string getBuf() const { return std::string{buf}; }
};


#endif /* MESSAGE_H_ */

/*
 * Message.h
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <string>

class Message {
public:
	enum MsgType { MSG_TICK, MSG_DONE, MSG_EXPLORE, MSG_REJECT, MSG_LEADER } msgType;
	int
	// Read a message from a file descriptor
	Message( int fd );

	// Create a message
	Message( enum MsgType );
	Message( enum MsgType, int _id );

	// Send this message to the named file descriptor
	int send( int fd );

	// Generate a string representation of this message
	std::string toString();
};


#endif /* MESSAGE_H_ */

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
#include <vector>

class Message {
	int rcvd;
	std::vector<int> children;
public:
	enum MsgType {  MSG_NULL=0, MSG_TICK, MSG_DONE, MSG_EXPLORE,
		           MSG_REJECT, MSG_LEADER, MSG_CHILDREN } msgType;

    void addChild( int c ) { children.push_back( c ); };
	int  id;
	// Read a message from a file descriptor
	Message() = delete;
	Message( const Message& ) = default;
	Message( Message&& );
	Message( int fd );
	Message( MsgType, const std::vector<int> );

	std::vector<int>& getChildren() { return children; }

	Message& operator=( const Message& m ) = default;

	// Create a message
	Message( enum MsgType mt, int _id=-1 ) : msgType(mt), id(_id) {};

	// Send this message to the named file descriptor
	int send( int fd );

	// Generate a string representation of this message
	std::string toString() const;
};


#endif /* MESSAGE_H_ */

/*
 * Message.cpp
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

#include "CS6380Project1.h"
#include "Message.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/sctp.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std;

// Move constructor

Message::Message( Message&& m ) : msgType( m.msgType ), id(m.id) {
	m.msgType = MSG_NULL;
	m.id = -1;
}


// Read a message from file descriptor <fd>
Message::Message( int fd ) {
    memset( buf, sizeof(buf), 0 );

    if ( (rcvd = recvfrom( fd, buf, 1023, 0, nullptr, nullptr )) < 0 ) {
	    throw runtime_error( string{"Unable to receive message: " } +
		      strerror( errno ) );
	}

	istringstream is(buf);
	string type;
	is >> type;

	is >> id;
	if ( is.eof() ) id = -1;

	if ( type == string{"TICK"}) msgType = MSG_TICK;
	else if ( type == string{"DONE"}) msgType = MSG_DONE;
	else if ( type == string{"EXPLORE"}) msgType = MSG_EXPLORE;
	else if ( type == string{"REJECT"}) msgType = MSG_REJECT;
	else if ( type == string{"LEADER"}) msgType = MSG_LEADER;
	else msgType = MSG_NULL;
}

// Send a message to a file descriptor
int Message::send(int fd) {
	string str = this->toString() + '\n';
	return ::send( fd, str.c_str(), str.length(), 0 );
}

// Utility function to convert a message into a human-readable string
string Message::toString() const {
	string str;

	switch( msgType ) {
	case MSG_TICK : str = "TICK"; break;
	case MSG_DONE : str = "DONE"; break;
	case MSG_EXPLORE : str = "EXPLORE"; break;
	case MSG_REJECT : str = "REJECT"; break;
	case MSG_LEADER : str = "LEADER"; break;
	case MSG_NULL : str = "(NULL)"; break;
	}

	str += string{ ' ' } + to_string( id );

	return str;
}



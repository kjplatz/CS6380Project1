/*
 * Message.cpp
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

#include "Message.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/sctp.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "CS6380Project2.h"

using namespace std;

// Move constructor

Message::Message( Message&& m ) :
	msgType( m.msgType ), round(m.round), level(m.level), edge(m.edge), sentBy(m.sentBy) {
    m.msgType = MSG_NULL;
    m.round = -1;
}


// Read a message from file descriptor <fd>
Message::Message( int fd ) :  sentBy{-1,-1,-1} {
    memset( buf, sizeof(buf), 0 );

    if ( (rcvd = recvfrom( fd, buf, 1023, 0, nullptr, nullptr )) < 0 ) {
        throw runtime_error( string {"Unable to receive message: " } +
                             strerror( errno ) );
    }

    istringstream is(buf);
    string type;
    is >> type;

    is >> round;
    if ( is.eof() ) round = -1;

    if ( type == string {"TICK"}) msgType = MSG_TICK;
    else if ( type == string {"ACK"}) msgType = MSG_ACK;
    else if ( type == string {"INITIATE"}) msgType = MSG_INITIATE;
    else if ( type == string {"REPORT"}) msgType = MSG_REPORT;
    else if ( type == string {"TEST"}) msgType = MSG_TEST;
    else if ( type == string {"ACCEPT"}) msgType = MSG_ACCEPT;
    else if ( type == string {"CONNECT"}) msgType = MSG_CONNECT;
    else if ( type == string {"CONNECT2ME" }) msgType=MSG_CONNECT2ME;
    else if ( type == string {"DONE"}) msgType = MSG_DONE;
    else if ( type == string {"REJECT"}) msgType = MSG_REJECT;
    else if ( type == string {"CHANGEROOT"}) msgType = MSG_CHANGEROOT;
    else msgType = MSG_NULL;

    if ( msgType == MSG_REPORT ||
    	 msgType == MSG_TEST   ||
		 msgType == MSG_ACCEPT ||
		 msgType == MSG_CONNECT||
		 msgType == MSG_CONNECT2ME ||
		 msgType == MSG_REJECT ||
		 msgType == MSG_CHANGEROOT ) {
    	int v1, v2;
    	float w;
    	char c;
        is >> c >> v1 >> v2 >> w;
        edge = Edge{v1,v2,w};

        if ( msgType == MSG_TEST ||
        	 msgType == MSG_ACCEPT ||
        	 msgType == MSG_CONNECT2ME ||
			 msgType == MSG_CONNECT ||
			 msgType == MSG_REJECT ||
			 msgType == MSG_REPORT ||
			 msgType == MSG_CHANGEROOT) {
        	is >> c >> level;
        }
    }
}

// Send a message to a file descriptor
// DEBUG DEBUG DEBUG
int Message::send(int fd) {
    string str = this->toString() + '\n';
    
    int out = ::send( fd, str.c_str(), str.length(), 0 );
//    ostringstream os;
//    os << "Message::send() sent " << out << " bytes to FD " << fd << ": " << str.c_str() << endl;
//    cerr << os.str();
    return out;
}

// Utility function to convert a message into a human-readable string
string Message::toString() const {
    string str;

    switch( msgType ) {
    case MSG_NULL :
        str = "(NULL)";
        break;
    case MSG_TICK :
        str = string{"TICK "} + to_string( round );
        break;
    case MSG_ACK :
    	str = string{"ACK "} + to_string( round );
    	break;
    case MSG_INITIATE:
    	str = string{"INITIATE "} + to_string( round );
    	break;
    case MSG_TEST:
    	str = string{"TEST "} + to_string( round )
		      + string{" "} + edge.to_string() + string{" "}
    	      + to_string(level);
    	return str;
    case MSG_ACCEPT:
    	str = string{"ACCEPT "} + to_string( round )
		      + string{" "} + edge.to_string() + string{" "}
		      + to_string(level);
    	return str;
    case MSG_CONNECT:
    	str = string{"CONNECT "} + to_string( round )
		      + string{" "} + edge.to_string() + string{" "} + to_string(level);
    	return str;
    case MSG_CONNECT2ME:
    	str = string{"CONNECT2ME "} + to_string( round )
		      + string{" "} + edge.to_string() + string{" "} + to_string(level);
    	return str;
    case MSG_REPORT:
    	str = string{"REPORT "} + to_string( round )
		      + string{" "} + edge.to_string() + string{" "} + to_string(level);
    	return str;
    case MSG_DONE :
        str = string{"DONE "} + to_string(round);
        break;
    case MSG_REJECT :
        str = string{"REJECT "} + to_string(round) + string{" "} + edge.to_string();
        break;
    case MSG_CHANGEROOT:
    	str = string{"CHANGEROOT "} + to_string( round )
		      + string{" "} + edge.to_string() + string{" "} + to_string(level);
    	return str;
    }

    return str;
}



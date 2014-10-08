/*
 * main.cpp
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/sctp.h>

#include "CS6380Project1.h"

using namespace std;

bool verbose=false;

fstream results;

int main( int argc, char** argv ) {
	int numNodes;
	vector<vector<bool>> neighbors;
	vector<int> nodeIds;

	if ( argc == 1 ) {
		cerr << "Error: usage: " << argv[0] << " <cfg_file>" << endl;
		return 0;
	}

	string cfg_file{ argv[1] };
	numNodes = parse_config( cfg_file, nodeIds, neighbors );

	if ( argc > 2 && string{argv[2]} == string{"verbose"} ) verbose = true;

        cout << "Processed " << numNodes << " nodes." << endl;
        for( int i=0; i<numNodes; i++ ) {
            cout << "Node " << i << " has ID: " << nodeIds[i] << endl;
            cout << "    -- Neighbors: ";
            for( unsigned j=0; j<neighbors[i].size(); j++ ) {
                if ( neighbors[i][j] ) cout << j << " ";
            }
            cout << endl;
        }

	// file descriptors for neighbors to talk
    vector<vector<int>> neighbor_fds(numNodes);
    // file descriptors for master to talk to threads
    vector<int> master_fds;
    vector<thread*> threads;

    // Set up all of the socket pairs between neighbors...

    int sockpair[2];

    for( int i=0; i<numNodes; i++ ) {
    	if ( socketpair( AF_LOCAL, SOCK_SEQPACKET, 0, sockpair ) < 0 ) {
    		ostringstream os( "Unable to create local socket pair: " );
        	os << strerror( errno );
        	throw runtime_error( os.str() );
        }
    	master_fds.push_back( sockpair[0] );

    	// File descriptor we'll talk to our master on...
    	int master_fd = sockpair[1];

     	verbose && cout << "Thread " << i << " has socketpair " << sockpair[0] << ", " << sockpair[1] << endl;

    	for( int j=0; j<numNodes; j++ ) {
    		if ( i == j ) continue;   // A node can't be its own neighbor...
    		if ( neighbors[i][j] ) {

    			// Note, we are using socketpair() here.  A (successful) call to
    			// socketpair() returns two local socket file descriptors at either
    			// endpoint of an established communications channel.  This allows
    			// us to effectively model a distributed system via message-passing.
    			// These operate much like other types of sockets, except that
    			// all communications happen within the kernel.
    			if ( socketpair( AF_LOCAL, SOCK_SEQPACKET, 0, sockpair ) < 0 ) {
    				ostringstream os( "Unable to create local socket pair: " );
    				os << strerror( errno );
    				throw runtime_error( os.str() );
    			}

    			// Add the appropriate file descriptors to process i's and j's neighbor
    			// lists.
    			neighbor_fds[i].push_back( sockpair[0] );
    			neighbor_fds[j].push_back( sockpair[1] );

    			verbose && cout << "Creating socketpair [" << sockpair[0] << ", " << sockpair[1] << "] for nodes "
    					        << i << " <-> " << j << endl;

    			// Set neighbors[j][i] to false so we don't create duplicate channels.
    			neighbors[j][i] = false;
    		}
    	}

    	//
    	// Spawn a new thread and push it back into the thread vector array...
    	threads.push_back( new thread( run_node, nodeIds[i], master_fd, neighbor_fds[i] ) );
    }

    int round=0;

    int leader = -1;
    while( leader < 0 ) {
    	round++;
    	sleep(1);

    	Message tick( Message::MSG_TICK, round );

    	// Send a tick to everyone
    	cout << "Sending to " << master_fds.size() <<
    			" threads: " << tick.toString() << endl;
    	for( auto fd : master_fds ) {
    		tick.send( fd );
    	}

    	sleep(1);

    	int doneCount=0;
    	// Read a message from each process file descriptor... this will either be a
    	// LEADER message or a DONE message.
    	for( int i=0; i<master_fds.size(); i++ ) {
    		int id;

    		// Get the message and parse it...
    		Message msg( master_fds[i] );

    	  	switch( msg.msgType ) {
    	 	case Message::MSG_LEADER:  // I know who the leader is.  Ignore me from here on out
    	 		cout << "Got message from fd " << master_fds[i] << ": " << msg.toString() << endl;
    	 		leader = msg.id;
    		    break;
    		// fall through
    		case Message::MSG_DONE:    // I don't know, but I'm done with this round.
    			doneCount++;
    		    break;

    		default:
    	        throw runtime_error(
    			    string{"Master thread received unexpected message: "} +
    		    msg.toString() );
    	    }
    	}
    	cout << "Received DONE from " << doneCount << " threads." << endl;
    }

    cout << "got LEADER(" << leader << ")" << endl;

    results.open( "results.log", fstream::in | fstream::out | fstream::trunc );

    queue<int> bfsQ;
    bfsQ.push( leader );
    Message leaderMsg{ Message::MSG_LEADER, leader };
    string ignore;
    while( !bfsQ.empty() ) {
    	int node = bfsQ.front();

    	verbose && cout << "Popped node " << node << " from queue..." << endl;
    	bfsQ.pop();
    	int i=0;
    	while( nodeIds[i] != node && i < nodeIds.size() ) i++;
    	verbose && cout << "Node " << node << " is number " << i << endl;

    	int fd = master_fds[i];
    	verbose && cout << "Sending " << leaderMsg.toString() << " to file descriptor " << fd << endl;
    	leaderMsg.send( fd );
    	Message done( fd );

    	results.seekg( -2, fstream::end ); // seek get to EOF.
    	results.seekp( 0, fstream::end ); // seek put to EOF.
    	results.clear();

    	streampos pos=results.tellg() - 2;
    	char c;
        for(int i = pos-2; i > 0; i-- )
        {
            results.seekg(i);
            c = results.get();
            if( c == '\r' || c == '\n' )//new line?
                 break;
        }

//    	cout << "Currently at position " << results.tellg() << endl;
    	results.clear();

    	while( results.peek() != '\n' ) results.seekg( -1, fstream::cur );
//    	cout << "Currently at position " << results.tellg() << endl;
        results.seekg( 1, fstream::cur );
        results >> ignore;
//        cout << "Ignoring: " << ignore;

        while( !results.eof() ) {
        	results >> node;
        	if ( !results.eof() ) {
//        		cout << "Got node " << node << endl;
        		bfsQ.push( node );
        	}
        }
        results.clear();
    }

    for( auto thread : threads ) {
    	thread->detach();
    }
    
    cout << "All threads terminated... exiting" << endl;

}




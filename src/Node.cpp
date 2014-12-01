/*
 * Node.cpp
 *
 *  Created on: Nov 30, 2014
 *      Author: ken
 */

#include <chrono>
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <thread>
#include <vector>

#include "Node.h"

using namespace std;
const Message nullReport{ Message::MSG_REPORT, 0, Edge{ 0, 0, numeric_limits<float>::max() }, 0 };

Node::Node( int _myId, int mfd, vector<Neighbor> nbrs ) :
    myParent(0,0,0), myId(_myId), myComponent( Edge{myId, 0, 0} ),
	masterFd(mfd), neighbors( std::move( nbrs ) ),
	fout(string {"node"} + to_string(myId) + string {".log"}),
	distribution(1, 20) {
    // Create a seed for the PRNG...
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    seed += myId;

    generator.seed(seed);
}

void Node::run() {
	fout << "Starting node " << myId << endl;
	bufferedReport = nullReport;

    for( unsigned i = 0; i < neighbors.size(); i++ ) {
    	neighbors[i].setIdx(i);
    	candidates.push_back( neighbors[i] );
    	msgQs.push_back( queue<Message>{} );
    }

    // Get all our candidates sorted (by edge weight).
    std::sort( candidates.begin(), candidates.end() );

    auto random_period = std::bind( distribution, generator );

    // Send out the first "test" message...
    Neighbor candidate = candidates.front();
    Message test{ Message::MSG_TEST,
    	          random_period(),    // Deliver at a random time between 0 and 20...
				  myComponent,        // My current component ID
				  myLevel };          // My current level
    fout << "Sending to neighbor " << candidate.getId() << "(" << candidate.getFd() << "): "
    	 << test.toString() << endl;
    test.send( candidate.getFd() );

    while( true ) {
    	Message begin( masterFd );
    	fout << "Starting round " << begin.round << endl;
    	round = begin.round;

    	queueIncoming();

    	/*
    	 * Process the queued messages
    	 */
    	for( unsigned i=0; i<msgQs.size(); i++ ) {
    		if ( msgQs[i].empty() ) continue;        // Queue is empty, skip
    		auto msg = msgQs[i].front();             // msg is the Message at queue head
    		if ( msg.round > begin.round ) continue; // If we're not ready to deliver, skip
    		msgQs[i].pop();                          // Okay, we can deliver.  Pop from the head of the queue

    		switch( msg.msgType ) {
    		case Message::MSG_TEST:       processTest(neighbors[i], msg); break;
    		case Message::MSG_INITIATE:   processInitiate(neighbors[i], msg ); break;
    		default: abort();
    		}
    	}

    	fout << "End of round " << round << endl;
    	Message done{ Message::MSG_ACK, round };
    	done.send( masterFd );
    }
}

/*
 * Node::queueIncoming()
 *     Perform a select() against each incoming file descriptor.
 *     Each incoming message is placed in the appropriate queue for
 *     that neighbor.  These messages aren't actually processed until
 *     the round indicated (as the 2nd field on each message)
 */
void Node::queueIncoming() {
	fd_set fdSet;
	FD_ZERO( &fdSet );
	int maxFd = 0;
	int nodesReady;

	// Read the pending messages from our neighbors...
	do {
	    for( auto nbr = neighbors.begin(); nbr != neighbors.end(); nbr++ ) {
		    FD_SET( nbr->getFd(), &fdSet );
		    maxFd = max( maxFd, nbr->getFd() );
	    }
	    struct timeval timeout = { 0, 0 };

	    // Figure out which neighbor file descriptors have messages queued...
	    nodesReady = select( maxFd, &fdSet, nullptr, nullptr, &timeout );
	    for( unsigned i=0; i<neighbors.size(); i++ ) {
	    	if ( FD_ISSET( neighbors[i].getFd(), &fdSet )) {
	    		// Read incoming message
	    		Message msg( neighbors[i].getFd() );
	    		fout << "Got from " << neighbors[i].getId() << ": "
	    		     << msg.toString() << endl;

	    		// Push to appropriate queue
	    		msgQs[i].push( msg );
	    	}
	    }
	} while ( nodesReady > 0 );
}

/*
 * Node::processTest( Message& msg )
 * Handle a "test" message
 */
void Node::processTest( const Neighbor& nbr, const Message& msg ) {
    verbose && fout << " received TEST from " << msg.edge.to_string();

    /*
     * Case 1: myLevel < msgLevel -- buffer the message
     */
    if ( myLevel < msg.level ) {
	    verbose && fout << "   (buffering...)" << endl;
    	testQ.push_back( msg );
    /*
     * Case 2: Component is different than mine -- send "Accept"
     */
	} else if ( myComponent != msg.edge ) {
		verbose && fout << " (accepting...)" << endl;
		Message accept( Message::MSG_ACCEPT,
				        round + distribution(generator),
				        myComponent,
			            myLevel );
		accept.send( nbr.getFd() );
		verbose && fout << "Sending to node " << nbr.getId() <<": "<<
				           accept.toString() << endl;
	/*
	 * Case 3: Component is the same as mine -- send "Reject"
	 */
	} else {
		verbose && fout << " (rejecting...)" << endl;

		Message reject{ Message::MSG_REJECT,
			            round + distribution(generator),
						msg.edge,
						msg.level };
		reject.send( nbr.getFd() );
		verbose && fout << "Sending to node " << nbr.getId() <<": "<<
		    	        reject.toString() << endl;
	}
}

/*
 * Initiate message:
 *     We have two things we must do here...
 *     First, we must propagate this message down our tree
 *     Second, we must start sending out TEST messages...
 */
void Node::processInitiate( const Neighbor& nbr, const Message& msg ) {
	verbose && fout << "Let's get this round started, baby!" << endl;
    // Forward the message to all tree nodes
    // And clear the "responded" flag...
    for( auto nbr : trees ) {
     	verbose && fout << "   Node " << nbr.getId()
       			        << ": Dude!! Wake up and begin!!" << endl;
     	Message fwd{ msg };
        fwd.round = round + distribution(generator);
        fwd.send( nbr.getFd() );
        nbr.clearResponded();
    }

    bufferedReport = nullReport;

    // If we have at least one candidate edge, send out a TEST message
    if ( !candidates.empty() ) {
        // Send a TEST message to the best candidate...
    	std::sort( candidates.begin(), candidates.end() );
    	Message test{ Message::MSG_TEST,
    		          round + distribution(generator),
					  myComponent,
    			      myLevel };
        test.send( candidates.front().getFd() );
    	hasSentTest = true;
    	verbose && fout << "    Node " << candidates.front().getId()
    	   		        << ": " << test.toString() << endl;
    } else if ( trees.empty() ) {
    	bufferedReport.round = round + distribution(generator);
    }
    // TODO -- If we have no candidates and no tree edges, send the message back up the tree...
}

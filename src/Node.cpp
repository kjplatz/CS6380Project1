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
const Message nullReport{ Message::MSG_REPORT,
	                      0,
	                      Edge{ 0, 0, numeric_limits<float>::max() },
						  -1 };

Node::Node( int _myId, int mfd, vector<Neighbor> nbrs ) :
    myParent(-1,-1,-1), myId(_myId), myComponent( Edge{myId, 0, 0} ),
	masterFd(mfd), neighbors( std::move( nbrs ) ),
	fout(string {"node"} + to_string(myId) + string {".log"}),
	distribution(1,1) { //20) { //TODO: change this back to 20 from 1, 1 is for testing only
    // Create a seed for the PRNG...
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    seed += myId;

    sentConnect = Message{Message::MSG_NULL};

    generator.seed(seed);
    round = 0;
}

void Node::run() {
	fout << "Starting node " << myId << endl;
	bufferedReport = nullReport;

	fout << "I have the following neighbors:" << endl;
    for( unsigned i = 0; i < neighbors.size(); i++ ) {
    	fout << "    " << neighbors[i].getId() << " (weight "
    		 << neighbors[i].getWeight() << ")" << endl;
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

    	if (begin.msgType == Message::MSG_REPORT) {
    		doReport();
    	}

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

    		fout << "Processing message: " << msg.toString() << endl;
    		switch( msg.msgType ) {
    		case Message::MSG_ACCEPT:     processAccept(neighbors[i], msg ); break;
    		case Message::MSG_CHANGEROOT: processChangeRoot( neighbors[i], msg ); break;
    		case Message::MSG_CONNECT:    processConnect( neighbors[i], msg ); break;
    		case Message::MSG_CONNECT2ME: processConnect2Me( neighbors[i], msg ); break;
    		case Message::MSG_REJECT:     processReject( neighbors[i], msg ); break;
    		case Message::MSG_REPORT:     processReport( neighbors[i], msg ); break;
    		case Message::MSG_INITIATE:   processInitiate(neighbors[i], msg ); break;
    		case Message::MSG_TEST:       processTest(neighbors[i], msg); break;
    		default:
    			cerr << "Unknown message: " << msg.toString() << endl;
    			abort();
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
	    for( auto nbr : neighbors ) {
		    FD_SET( nbr.getFd(), &fdSet );
		    fout << "DEBUG: Enabling select for " << nbr.getId() << "'s FD " << nbr.getFd() << endl;
		    maxFd = max( maxFd, nbr.getFd() );
	    }
	    struct timeval timeout = { 0, 0 };

	    // Figure out which neighbor file descriptors have messages queued...
	    nodesReady = select( maxFd, &fdSet, nullptr, nullptr, &timeout );
	    for( unsigned i=0; i<neighbors.size(); i++ ) {
	    	if ( FD_ISSET( neighbors[i].getFd(), &fdSet )) {
	                fout << "DEBUG: Found message on " << neighbors[i].getId() << "'s FD " << neighbors[i].getFd() << endl;

	    		// Read incoming message
	    		Message msg( neighbors[i].getFd() );
	    		fout << "Got from " << neighbors[i].getId() << ": "
	    		     << msg.toString() << endl;

	    		msg.sentBy = neighbors[i];
	    		// Push to appropriate queue
	    		msgQs[i].push( msg );
	    	}
	    }
	} while ( nodesReady > 0 );
}

/*
 * Got an accept message.  This can start a whole bunch of things in motion...
 */
void Node::processAccept( const Neighbor& nbr, const Message& msg ) {
	// Is this edge the best we've seen so far?
	if ( bufferedReport.edge > msg.edge ) {
		bufferedReport = msg;
		bufferedReport.level = myId;
	}
	hasSentTest = false;

	// Check to see if we're ready to convergecast this back up the tree.
    doConvergecast();
}

/*
 * We've been chrooted!!! Aaaaah!!!
 */
void Node::processChangeRoot( const Neighbor& nbr, const Message& msg ) {
    bool isTree = false;
    for( auto t : trees ) {
    	if ( nbr == t ) isTree = true;
    }
    if ( !isTree ) trees.push_back( nbr );
    myParent = nbr;
    myComponent = msg.edge;
    myLevel = msg.level;
    fout << "I got CHROOTED to level " << myLevel << endl;
    Message chroot{ msg };
    for( auto t : trees ) {
    	if ( t == myParent ) continue;
    	chroot.round = round + distribution(generator);
    	chroot.send( t.getFd() );
    	fout << "Passing CHROOT on to " << t.getId() << ": "
    		 << chroot.toString() << endl;
    }

    bufferedReport = nullReport;
    hasSentTest = false;
    sentConnect = Message{Message::MSG_NULL};

    // Handle any outstanding TEST or CONNECT requests that we may be able to deal with now.
    processTestQ();
    processConnectQ();
}
/*
 * Our fearless leader says "thou shalt connect!"
 */
void Node::processConnect( const Neighbor& nbr, const Message& msg ) {
	/*
	 * Note:  In a "CONNECT" message, the "level" field indicates our ID
	 */
	if ( msg.level == myId ) {
		sendConnect2Me();
	} else {
		Message connect = msg;
		for( auto tn : trees ) {
			if ( tn == myParent ) continue;
			connect.round = round + distribution(generator);
			connect.send( tn.getFd() );
			fout << "Sending CONNECT to " << tn.getId() << ": "
				 << connect.toString();
		}
	}
}
/*
 * We got rejected!  How sad...
 */
void Node::processReject( const Neighbor& nbr, const Message& msg ) {
	Neighbor youreject = candidates.front();   // YOU GOT REJECTED!
	candidates.erase( candidates.begin() );    // YOU'RE NO CANDIDATE!
	rejects.push_back( youreject );            // INTO THE REJECT PILE YOU GO!
	if ( candidates.empty() ) {
		hasSentTest = false;
		doConvergecast();
	} else {
		Neighbor cnd = candidates.front();
		Message test{ Message::MSG_TEST,
			          round + distribution(generator),
					  myComponent,
					  myLevel };
		test.send( cnd.getFd() );
		fout << "Sending TEST to " << cnd.getId() << ": " << test.toString() << endl;
	}
}

/*
 * One of our children is reporting its MWOE back up the tree...
 */
void Node::processReport( const Neighbor& nbr, const Message& msg ) {
    if ( bufferedReport.edge > msg.edge ) {
    	bufferedReport = msg;
    	bufferedReport.level = myId;
    }
    for( auto n : trees ) {
    	if ( n == nbr ) n.setResponded();
    }
    doConvergecast();
}

/*
 * Node::processTest( Message& msg )
 * Handle a "test" message
 */
void Node::processTest( const Neighbor& nbr, const Message& msg ) {
    /*
     * Case 1: myLevel < msgLevel -- buffer the message
     */
    if ( myLevel < msg.level ) {
	    fout << "   (buffering...)" << endl;
    	testQ.push_back( msg );
    /*
     * Case 2: Component is different than mine -- send "Accept"
     */
	} else if ( myComponent != msg.edge ) {
		fout << " (accepting...)" << endl;
		Message accept{ Message::MSG_ACCEPT,
				        round + distribution(generator),
				        myComponent,
			            myLevel };
		accept.send( nbr.getFd() );
		fout << "Sending to node " << nbr.getId() <<": "<<
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
		fout << "Sending to node " << nbr.getId() <<": "<<
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
    } else if ( trees.empty() ||
    		    (trees.size() == 1 &&
				 trees.at(0) == myParent ) ) {
    	// If our only "Tree" node is our parent, go ahead and send the
    	// nullReport back up the tree.
    	bufferedReport.round = round + distribution(generator);
    	bufferedReport.send( myParent.getFd() );
    	fout << "Sending REPORT to " << myParent.getId() << ": "
    		 << bufferedReport.toString() << endl;
    }
}

/*
 * Perform the convergecast.  This can be called upon receiving either
 * an ACCEPT, REJECT, or REPORT message.
 */
void Node::doConvergecast() {
    if ( hasSentTest ) return;

    bool phaseDone = true;
    for( auto nbr : trees ) {
    	if ( nbr == myParent ) continue;
    	phaseDone = phaseDone && nbr.hasResponded();
    }

    /*
     * Okay, we've received responses from everyone who we are waiting on...
     */
    if ( phaseDone ) {
    	if ( isLeader ) {
    		// If I'm the leader, and we haven't found a new outgoing edge
    		// we're done.  Yay!!!
    	    if ( bufferedReport.edge.getWeight() >=
    	         (float)numeric_limits<int>::max() ) {
    	    	Message done{ Message::MSG_DONE, round };
    	    	done.send( masterFd );
    	    } else if ( bufferedReport.level == myId ) {
    	    	// Send a CONNECT2ME message
    	    	// This is handled elsewhere, since it may trigger multiple things
                sendConnect2Me();
    	    } else {
    	    	Message connect{ Message::MSG_CONNECT,
    	    		             round,
								 bufferedReport.edge,
								 bufferedReport.level };
    	    	for( auto nbr : trees ) {
    	    		connect.round = round + distribution(generator);
    	    		connect.send( nbr.getFd() );
    	    	}
    	    }
    	} else {
    		// We're not the leader.
    		// Just propagate the report up the tree.
    		bufferedReport.round = round + distribution(generator);
    		bufferedReport.send( myParent.getFd() );
            fout << "Sending to node " << myParent.getId() << ": "
            	 << bufferedReport.toString() << endl;
    	}
    }
}

// Report our subtree back to the main thread...
void Node::doReport() {
	return;
}

// Start a new phase of the protocol
void Node::initiatePhase() {
	Message msg{ Message::MSG_INITIATE,
		         round };
    for( auto nbr : trees ) {
    	msg.round = round + distribution(generator);
    	msg.send( nbr.getFd() );
    	fout << "Sending to node " << nbr.getId() << ": "
    		 << msg.toString() << endl;
    	nbr.clearResponded();
    }

    if ( candidates.empty() ) return;

    Neighbor cnbr = candidates.front();
    hasSentTest = true;
    Message test{ Message::MSG_TEST,
    	          round + distribution(generator),
				  myComponent,
				  myLevel };
    test.send( cnbr.getFd() );
    fout << "Sending to node " << cnbr.getId() << ": "
         << test.toString() << endl;
}

// Send a connect2me message:
void Node::sendConnect2Me() {
	auto nbr = candidates.front();

	// Create the message and send it.
	sentConnect = Message { Message::MSG_CONNECT2ME,
		          round + distribution(generator),
				  bufferedReport.edge,
				  myLevel };
	sentConnect.sentBy = nbr; //WAS MISSING: sentBy used to store sentTo in this case to be used when determining if MERGE or ABSORB operation

	sentConnect.send( nbr.getFd() );
	fout << "Sending to node " << nbr.getId() << ": "
		 << sentConnect.toString() << endl;

	// Check to see if we've received a C2Me message from the other
	// guy.
	int foundCmsg = -1;
	for( unsigned i=0; i<connectQ.size(); i++ ) {
		if ( connectQ[i].sentBy == nbr ) foundCmsg = i;
	}

	if ( foundCmsg < 0 ) return;
	auto cmsg = connectQ[foundCmsg];

	fout << "Already received a Connect2Me message from this guy..." << endl;
	if ( cmsg.level == myLevel && myId < nbr.getId() ) {
		connectQ.erase( connectQ.begin() + foundCmsg );
		createNewComponent( nbr );
	}
}

// We just went up in level... can we do anything with these test messages?
void Node::processTestQ() {
	std::sort( testQ.begin(), testQ.end(),
			[]( const Message& a, const Message& b ) { return a.level < b.level; }
			);
    while( !testQ.empty() && testQ.front().level <= myLevel ) {
    	auto msg = testQ.front();
    	testQ.erase( testQ.begin() );

    	// Re-process.  Our level is sufficiently high that we can do something
    	// other than buffer.
    	processTest( msg.sentBy, msg );
    }
}

// We just went up in level.  Can we do anything with these connect messages?
void Node::processConnectQ() {
    // Sort the queued connect messages by level (ascending)
	std::sort( connectQ.begin(), connectQ.end(),
			[]( const Message& a, const Message& b ) { return a.level < b.level; }
			);

	// Look at the first one in the queue...
	while( !connectQ.empty() && connectQ.front().level < myLevel ) {
	    auto msg = connectQ.front();
	    connectQ.erase( connectQ.begin() );

	    processConnect2Me( msg.sentBy, msg );
	}
}

void Node::processConnect2Me( const Neighbor& nbr, const Message& msg ) {
	//DEBUG //fout << "\tHERE!!!! ProcessConnect2Me\n\t\tnbrId: " << nbr.getId() << "\n\t\tsentBy: " << sentConnect.sentBy.getId() << endl;
	if ( msg.level == myLevel ) {
		if ( sentConnect.sentBy.getId() == nbr.getId() && myId < nbr.getId() ) {
			createNewComponent( nbr );
		} else {
		    connectQ.push_back( msg );
		}
		return;
	}

    for( unsigned i=0; i<candidates.size(); i++ ) {
    	if ( candidates[i] == nbr ) {
    		candidates.erase( candidates.begin() + i );
    		trees.push_back( nbr );
    		trees.back().clearResponded();
    		Message chr{ Message::MSG_CHANGEROOT,
    			         round + distribution(generator),
						 myComponent,
						 myLevel };
    		chr.send( nbr.getFd() );
    		fout << "Sending CHANGEROOT to " << nbr.getId()
    			 << ": " << chr.toString() << endl;

    		if ( hasSentTest ) {
    			Message init{ Message::MSG_INITIATE,
    				          round + distribution(generator) };
    			init.send( nbr.getFd() );
    			fout << "Sending INITIATE to " << nbr.getId()
    				 << ": " << init.toString() << endl;
    		}
    	}
    }
}

void Node::createNewComponent( const Neighbor& nbr ) {
	myLevel++;
	fout << "==== GOING UP TO LEVEL " << myLevel << " ====" << endl;
	fout << "==== I HAVE THE POWER (I'm the new leader) ====" << endl;
	isLeader = true;
	myComponent = nbr.getEdge(myId);

	candidates.erase( candidates.begin() );
	trees.push_back( nbr );
	myParent = Neighbor{ -1, -1, -1 };

	Message chr{ Message::MSG_CHANGEROOT,
				 round,
				 myComponent,
				 myLevel };

	for( auto child : trees ) {
		chr.round = round + distribution(generator);
		chr.send( child.getFd() );
		fout << "Sending to child " << child.getId() << ": "
			 << chr.toString() << endl;
	}

	sentConnect = Message{Message::MSG_NULL};

	// Handle any outstanding test messages...
	processTestQ();

	// Handle any outstanding connection requests...
	processConnectQ();

	sentConnect = Message{Message::MSG_NULL};
	initiatePhase();
}

/*
 * run_thread.cpp
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <set>
#include <vector>

#include <sys/select.h>

#include "CS6380Project2.h"
#include "Message.h"
#include "Neighbor.h"

using namespace std;

//
// Function to run a node
// Parameters:
//    node_id     : The node ID to represent
//    master_fd   : Socket file descriptor to talk to/from master thread
//    neighbors   : Neighbor struct information about our neighbors

// Summary of execution:

void run_node(int node_id, int master_fd, vector<Neighbor> neighbors ) {
	bool isLeader = true;                     // Am I the leader of my component?  (Initially yes)
	Edge myComponent = Edge{ node_id, 0, 0 }; // What is the ID of my component? (Initially me)
	int  myLevel = 0;                         // What level is my component? (Initially 0)
	Neighbor myParent = Neighbor{ 0, 0, 0 };  // Who is my parent in the MST?

	// "Report" message we're waiting to send...
	Message bufferedReport{ Message::MSG_REPORT,
		                    0,
							Edge{ node_id, 0, std::numeric_limits<int>::max() },
							0 };

	// Log all my stuff...
    ofstream fout(string {"node"} + to_string(node_id) + string {".log"});
    fout << "Starting node " << node_id << endl;

    vector<Neighbor>   candidates,  // Candidate edges
	                   trees,       // Tree edges
					   rejects;     // Rejected edges

    vector<queue<Message>> msgQs;   // Queued messages from each neighbor
    std::vector<Message> testQ;     // Buffered test messages
    for( unsigned i = 0; i < neighbors.size(); i++ ) {
    	neighbors[i].setIdx(i);
    	candidates.push_back( neighbors[i] );
    	msgQs.push_back( queue<Message>{} );
    }

    std::sort( candidates.begin(), candidates.end() );

    fout << "My component ID is " << myComponent.to_string() << endl;

    fout << "Neighbor list..." << endl;
    for( auto nbr : candidates ) {
    	fout << "    " << nbr.to_string() << endl;
    }

    enum NodeState {
    	STATE_WAITING,      // Waiting for a message from leader
		STATE_INITIATED,    // Received 'initiate' from leader
		STATE_SENT_TEST,    // Sent test message, awaiting response
		STATE_SENT_MWOE,    // Sent MWOE to leader, awaiting response
		STATE_SENT_CONNECT, // Sent connect request to other node.
    } myState = STATE_SENT_TEST;

    // Create a seed for the PRNG...
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    seed += node_id;

    mt19937 generator(seed);
	uniform_int_distribution<int> distribution( 1, 20 );
    auto random_period = std::bind(distribution, generator);

    // At this point, calling random_period() will generate a random number
    // between 1 and 20, inclusive...

    // Send out the first "test" message...
    Neighbor candidate = candidates.front();
    Message test{ Message::MSG_TEST,
    	          random_period(),    // Deliver at a random time between 0 and 20...
				  myComponent,        // My current component ID
				  myLevel };          // My current level
    fout << "Sending to neighbor " << candidate.getId() << "(" << candidate.getFd() << "): "
    	 << test.toString() << endl;
    test.send( candidate.getFd() );

    bool hasSentTest = true;          // Have I sent a test message yet?  Yes i have!

    // We're as ready as we're ever going to be...

    while( true ) {
    	Message begin( master_fd );
    	fout << "Starting round " << begin.round << endl;

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
    	    nodesReady = select( maxFd, &fdSet, nullptr, nullptr, &timeout );
    	    for( unsigned i=0; i<neighbors.size(); i++ ) {
    	    	if ( FD_ISSET( neighbors[i].getFd(), &fdSet )) {
    	    		Message msg( neighbors[i].getFd() );
    	    		fout << "Got from " << neighbors[i].getId() << ": "
    	    		     << msg.toString() << endl;

    	    		msgQs[i].push( msg );
    	    	}
    	    }
    	} while ( nodesReady > 0 );

    	// Okay, now all of the messages are in their appropriate "pending" queues...
    	for( unsigned i=0; i<msgQs.size(); i++ ) {
    		if ( msgQs[i].empty() ) continue;
    		Message msg = msgQs[i].front();

    		// If we haven't hit the round we're supposed to deliver this message yet,
    		// then do nothing.
    		if ( msg.round > begin.round ) continue;

    		// Okay, we can actually deliver this message now.
    		msgQs[i].pop();                // Go ahead and pop it off the head of the queue
    		switch( msg.msgType ) {
    		/*
    		 * Test message.  We have 3 possible ways to handle this:
    		 *     If our level is less than that of the sender, buffer the message
    		 *     If the message is from a different component, send an ACCEPT
    		 *     If the message is from our same component, send a REJECT
    		 */
    		case Message::MSG_TEST: {
		    	    verbose && fout << " received TEST from " << msg.edge.to_string();

    			    if ( myLevel > msg.level ) {
    		    	    verbose && fout << "   (buffering...)" << endl;
                    	testQ.push_back( msg );
    		    	} else if ( myComponent != msg.edge ) {
    		    		verbose && fout << " (accepting...)" << endl;
    		    		Message accept( Message::MSG_ACCEPT,
    		    				        begin.round + random_period(),
    		    				        myComponent,
										myLevel );
    		    		accept.send( neighbors[i].getFd() );
    		    		verbose && fout << "Sending to node " << neighbors[i].getId() <<": "<<
    		    				           accept.toString() << endl;
    		    	} else {
    		    		verbose && fout << " (rejecting...)" << endl;

    		    		msg.msgType = Message::MSG_REJECT;
    		    		msg.round   = begin.round + random_period();
    		    		msg.send( neighbors[i].getFd() );
    		    		verbose && fout << "Sending to node " << neighbors[i].getId() <<": "<<
    		    		    	        msg.toString() << endl;
    		    	}
    			} break;
    		/*
    		 * Initiate message:
    		 *     We have two things we must do here...
    		 *     First, we must propagate this message down our tree
    		 *     Second, we must start sending out TEST messages...
    		 */
    		case Message::MSG_INITIATE: {
    			    verbose && fout << "Let's get this round started, baby!" << endl;
    			    // Forward the message to all tree nodes
    			    // And clear the "responded" flag...
    			    for( auto nbr : trees ) {
    			    	verbose && fout << "   Node " << nbr.getId()
    			    			        << ": Dude!! Wake up and begin!!" << endl;
    				    msg.round = begin.round + random_period();
    				    msg.send( nbr.getFd() );
    				    nbr.clearResponded();
    			    }

    			    // If we don't have any candidates, send a message back up the tree
    			    if ( !candidates.empty() ) {
    			    	// Send a TEST message to the best candidate...
    			    	std::sort( candidates.begin(), candidates.end() );
    			        Message test{ Message::MSG_TEST,
    			        	          begin.round + random_period(),
									  myComponent,
    			    				  myLevel };
    			        test.send( candidates.front().getFd() );
    			        hasSentTest = true;
    			        verbose && fout << "    Node " << candidates.front().getId()
    			        		        << ": " << test.toString() << endl;
    			    }
    		    } break;
    		case Message::MSG_REPORT: {
    			    if ( bufferedReport.edge < msg.edge ) {
    			    	bufferedReport = msg;
    			    }
    			    for( auto nbr : trees ) {
    			    	if ( nbr.getIdx == i ) nbr.hasResponded = true;
    			    }


    			    bool phaseDone = !hasSentTest;
    			    for( auto nbr : trees ) {
    			    	phaseDone = phaseDone && nbr.hasResponded;
    			    }
    			    if ( phaseDone ) {
    			    	bufferedReport.round = begin.round + random_period();
    			    	bufferedReport.edge  = Edge{ 0, 0, numeric_limits<int>::max() };
    			    	bufferedReport.send( myParent.getFd() );
    			    }

    		    } break;
    		case Message::MSG_ACCEPT: {

    		    } break;
    		case Message::MSG_CONNECT: {

    		    } break;
    		/*
    		 * We got rejected (D'oh!)
    		 * If we've done this correctly, this will only come from our first candidate
    		 * edge.  Move him to a rejected edge, and go ahead and send out another test
    		 * message if we've still got a candidate
    		 */
    		case Message::MSG_REJECT: {
    			    Neighbor reject = candidates.front();    // YOU REJECT YOU!
    			    rejects.push_back( reject );             // INTO THE REJECT PILE YOU GO!
    			    candidates.erase( candidates.begin() );

    			    // Try the next candidate...
                    if ( !candidates.empty() ) {
    			        Message test{ Message::MSG_TEST,
    			        	          begin.round + random_period(),
									  myComponent,
    			    				  myLevel };
    			        test.send( candidates.front().getFd() );
                    } else {
                    	hasSentTest = false;
                    	bool phaseDone = true;
                    	for( auto nbr : trees ) {
                    		phaseDone = phaseDone & nbr.hasResponded();
                    	}
                    	if (phaseDone) {
                    		if ( isLeader ) {
                                // If I'm the leader, figure out what to do here...
                    		} else {
                    			// If I'm not the leader, pass this up the chain...
                                bufferedReport.level = begin.level + random_period();
                                bufferedReport.send( myParent.getFd() );
                    		}
                    	}
                    }
    		    } break;
    		case Message::MSG_CHANGEROOT: {

    		    } break;
    		default:
    		    break;
    		}
    	}

    	fout << "End of round " << begin.round << endl;
    	Message ack{ Message::MSG_ACK, begin.round };
    	ack.send( master_fd );
    }
#if 0
	int maxId = node_id;
    ofstream fout(string {"node"} + to_string(node_id) + string {".log"});
    fout << "Starting node " << node_id << endl;

    int round = 0;

    // How many neighbors do I have?
    int numNbrs = neighbor_fds.size();

    // Who are my neighbors?
    vector<int> neighborIds( numNbrs, -1 );

    // Is my neighbor done?
    vector<bool> isDone( numNbrs, false );

    // Is my neighbor a child?
    vector<bool> isChild( numNbrs, false );

    // Am I done?
    bool done = false;

    // Which of my neighbors is my parent?
    int parent = -1;

    // Create initial explore message
    Message explore( Message::MSG_EXPLORE, node_id );

    // Vector of messages to send in next round.
    // Initially we send (EXPLORE, node_id) to everyone
    vector<Message> toSend( numNbrs, Message { Message::MSG_EXPLORE, node_id});
    while (true) {
        round++;
        // Wait for the next "TICK"
        Message msg(master_fd);

        // If the master thread gives us a "LEADER" message instead of a "TICK",
        // output our local snapshot of the tree:
        if ( msg.msgType == Message::MSG_LEADER ) {
            // If we get here, we're done.  Wootwoot!
            ostringstream os;

            os << "Node " << node_id << " terminating..." << endl;
            if ( parent != -1 ) os << "    parent: " << neighborIds[parent] << endl;
            else                os << "    Woohoo!! I'm the leader!" << endl;
            os << "    children: ";

            ofstream children("children.txt");

            for( int i=0; i<numNbrs; i++ ) {
                if ( isChild[i] ) {
                    os << neighborIds[i] << " ";
                    children << neighborIds[i] << " ";
                }
            }
            children << 0 << endl << flush;
            children.close();

            os << endl;
            cout << os.str();
            fout << os.str();

            Message done {Message::MSG_DONE};
            done.send( master_fd );

            return;
        }

        fout << "Begin of round " << msg.id << endl << flush;

        // Send messages to all my neighbors
        for( unsigned i=0; i<neighbor_fds.size(); i++ ) {
            fout << "--  Sending to node " << neighborIds[i] <<
                 "[fd " << neighbor_fds[i] << "]: " <<
                 toSend[i].toString() << endl;
            toSend[i].send( neighbor_fds[i] );
        }

        // By default everyone gets a NULL message
        std::fill( toSend.begin(), toSend.end(), Message {Message::MSG_NULL} );

        // Receive messages and process...
        for( unsigned i=0; i<neighbor_fds.size(); i++ ) {
            Message msg( neighbor_fds[i] );
            int receivedId;

            string pcd;
            if ( parent == (int)i ) pcd = "(parent)";
            else if ( isDone[i] ) {
                if ( isChild[i] ) pcd = "(child/done)";
                else pcd = "(rejected/done)";
            } else pcd = "pending";

            fout << "--  Node " << node_id <<
                 " received message from node " << neighborIds[i] <<
                 " [" << neighbor_fds[i] << "] " << pcd << ": " << msg.toString() << endl << flush;
            bool allDone = false;

            switch (msg.msgType) {
            case Message::MSG_EXPLORE:
                receivedId = msg.id;
                if ( neighborIds[i] < 0 ) {
                    neighborIds[i] = receivedId;
                }

                // Got a higher ID
                if (receivedId > maxId ) {
                    parent = i;
                    maxId = receivedId;
                    // New parent -- any DONE or REJECT messages
                    // we've received are no longer any good.
                    std::fill( isDone.begin(), isDone.end(), false );
                    std::fill( isChild.begin(), isChild.end(), false );
                    // Forward this ID on to all neighbors (but the parent)
                    std::fill( toSend.begin(), toSend.end(), msg );
                    toSend[parent] = Message::MSG_NULL;

                    // Is everyone done now?
                    allDone = true;
                    for( unsigned j=0; j<isDone.size(); j++ ) {
                        if ( (int)j == parent ) continue;
                        allDone = allDone && isDone[j];
                    }
                } else if ( receivedId == maxId ) {
                    // If we get maxId from another source, let him know
                    // he's not our parent...
                    toSend[i] = Message {Message::MSG_REJECT};
                    isDone[i] = true;

                    // Is everyone done now?
                    allDone = true;
                    for( unsigned j=0; j<isDone.size(); j++ ) {
                        if ( (int)j == parent ) continue;
                        allDone = allDone && isDone[j];
                    }
                } // else -- we don't need to worry about if we got
                if ( allDone ) {
                    toSend[parent] = Message {Message::MSG_DONE};
                }
                // an EXPLORE smaller than our maxId.
                // since we're going to send a new EXPLORE out within
                // the next round.
                break;

            case Message::MSG_REJECT:
                isDone[i] = true;

                // is everyone done now?
                allDone = true;
                for( unsigned j=0; j<isDone.size(); j++ ) {
                    if ( (int) j == parent ) continue;
                    allDone = allDone && isDone[j];
                }
                if ( allDone ) {
                    toSend[parent] = Message {Message::MSG_DONE};
                }
                break;

            case Message::MSG_DONE:
                isDone[i] = true;
                isChild[i] = true;
                allDone = true;
                for( unsigned j=0; j<isDone.size(); j++ ) {
                    if ( (int) j == parent ) continue;
                    allDone = allDone && isDone[j];
                }
                if ( allDone ) {
                    if ( parent == -1 ) {
                        done = true;
                    } else {
                        toSend[parent] = Message {Message::MSG_DONE};
                    }
                }
                break;

            // Nothing to see here.  Really!
            case Message::MSG_NULL:
                break;
            default:
                throw runtime_error(
                    string {"Process thread received unexpected message: "} + msg.toString());
            }
        }

        if ( !done ) {
            Message doneMsg(Message::MSG_DONE, round);
            fout << "End of round " << round << endl << flush;
            doneMsg.send(master_fd);
        } else {
            Message leaderMsg( Message::MSG_LEADER, node_id );
            fout << "I'm the leader!  Woooot!" << endl << flush;
            leaderMsg.send( master_fd );
        }
    }
#endif
}

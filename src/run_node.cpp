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
	Neighbor parent = Neighbor{ 0, 0, 0 };    // Who is my parent in the MST?

	// Log all my stuff...
    ofstream fout(string {"node"} + to_string(node_id) + string {".log"});
    fout << "Starting node " << node_id << endl;

    vector<Neighbor>   candidates,  // Candidate edges
	                   trees,       // Tree edges
					   rejects;     // Rejected edges

    vector<queue<Message>> msgQs;   // Queued messages from each neighbor
    std::vector<Message> testQ;     // Buffered test messages
    for( auto nbr = neighbors.begin(); nbr != neighbors.end(); nbr++ ) {
    	candidates.push_back( *nbr );
    	msgQs.push_back( queue<Message>{} );
    }

    std::sort( candidates.begin(), candidates.end() );

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
    test.send( candidate.getFd() );

    fout << "Sending to neighbor " << candidate.getId() << "(" << candidate.getFd() << ")"
    	 << test.toString() << endl;

    // We're as ready as we're ever going to be...

    while( true ) {
    	Message begin( master_fd );
    	fout << "Starting round " << begin.round << endl;

    	fd_set fdSet;
    	int maxFd = 0;
    	for( auto nbr = neighbors.begin(); nbr != neighbors.end(); nbr++ ) {
    		FD_SET( nbr->getFd(), &fdSet );
    		maxFd = max( maxFd, nbr->getFd() );
    	}


    	fout << "End of round " << begin.round << endl;
    	Message ack( Message::MSG_ACK, begin.round );
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

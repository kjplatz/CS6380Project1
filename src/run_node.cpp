/*
 * run_thread.cpp
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>

#include <sys/select.h>

#include "CS6380Project1.h"

using namespace std;

//
// Function to run a node
// Parameters:
//    node_id     : The node ID to represent
//    master_fd   : Socket file descriptor to talk to/from master thread
//    neighbor_fds: The socket file descriptors of my neighbors
void run_node(int node_id, int master_fd, vector<int> neighbor_fds) {
    int maxId = node_id;
    ofstream fout(string{"node"} + to_string(node_id) + string{".log"});
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
    vector<Message> toSend( numNbrs, Message{ Message::MSG_EXPLORE, node_id});
    while (true) {
        round++;
        
        fd_set neighborSet;
        int maxFd=0;

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
            for( int i=0; i<numNbrs; i++ ) {
                if ( isChild[i] ) os << neighborIds[i] << " ";
            }
            os << endl;
            cout << os.str();
            fout << os.str();
            results.clear();
            results.seekp( 0, fstream::end );
            results << os.str() << flush;

            Message done{Message::MSG_DONE};
            done.send( master_fd );

            return;
        }

        fout << "Begin of round " << msg.id << endl << flush;

        // Send messages to all my neighbors
        for( unsigned i=0; i<neighbor_fds.size(); i++ ) {
        	fout << "--  Sending to fd " << neighbor_fds[i] << ": " <<
        		 toSend[i].toString() << endl;
        	toSend[i].send( neighbor_fds[i] );

        	FD_SET( neighbor_fds[i], &neighborSet );
        	if ( maxFd < neighbor_fds[i] ) maxFd = neighbor_fds[i];
        }

        // By default everyone gets a NULL message
        std::fill( toSend.begin(), toSend.end(), Message{Message::MSG_NULL} );

        struct timeval timeout = { 0, 0 };
        // Use select() to figure out who has actually sent us a message...
        select( maxFd+1, &neighborSet, nullptr, nullptr, &timeout );

        // Receive messages and process...
        for( unsigned i=0; i<neighbor_fds.size(); i++ ) {
        	Message msg( neighbor_fds[i] );
        	int receivedId;

            string pcd;
            if ( parent == i ) pcd = "(parent)";
            else if ( isDone[i] ) {
                if ( isChild[i] ) pcd = "(child/done)";
                else pcd = "(rejected/done)";
            } else pcd = "pending";

            fout << "--  Node " << node_id <<
            	    " received message from fd " << neighbor_fds[i] << 
                    " [" << neighborIds[i] << "] " << pcd << ": " << msg.toString() << endl << flush;
            bool allDone = true;

            switch (msg.msgType) {
                case Message::MSG_EXPLORE:
                    receivedId = msg.id;
                    if ( neighborIds[i] < 0 ) {
                    	neighborIds[i] = receivedId;
                    }

                    // Got a higher ID
                    if (receivedId > maxId )
                    {
                    	parent = i;
                        maxId = receivedId;
                        std::fill( isDone.begin(), isDone.end(), false );
                        std::fill( isChild.begin(), isChild.end(), false );
                        // Forward this ID on to all neighbors (but the parent)
                        std::fill( toSend.begin(), toSend.end(), msg );
                        toSend[parent] = Message::MSG_NULL;
                    } else if ( receivedId == maxId ) {
                    	// If we get maxId from another source, let him know
                    	// he's not our parent...
                    	toSend[i] = Message{Message::MSG_REJECT};
                        isDone[i] = true;
                        allDone = true;
                        for( unsigned j=0; j<isDone.size(); j++ ) {
                        	if ( j == parent ) continue;
                        	verbose && fout << "Rejecting... node " << j << " is " <<
                        			(!isDone[j] ? "not " : "") << "done." << endl;
                        	allDone = allDone && isDone[j];
                        }
                        if ( allDone ) {
                        	toSend[parent] = Message{Message::MSG_DONE};
                        }
                    } // else -- we don't need to worry about if we got
                      // an EXPLORE smaller than our maxId.
                      // since we're going to send a new EXPLORE out within
                      // the next round.
                    break;
                case Message::MSG_REJECT:
                    isDone[i] = true;
                    allDone = true;
                    for( unsigned j=0; j<isDone.size(); j++ ) {
                    	if ( j == parent ) continue;
                    	allDone = allDone && isDone[j];
                    }
                    if ( allDone ) {
                    	toSend[parent] = Message{Message::MSG_DONE};
                    }
                    break;
                case Message::MSG_DONE:
                	isDone[i] = true;
                	isChild[i] = true;
                	allDone = true;
                    for( unsigned j=0; j<isDone.size(); j++ ) {
                    	if ( j == parent ) continue;
                    	allDone = allDone && isDone[j];
                    }
                    if ( allDone ) {
                        if ( parent == -1 ) {
                            done = true;
                        } else {
                    	    toSend[parent] = Message{Message::MSG_DONE};
                        }
                    }
                    break;

                // Someone knows who the leader is.
                case Message::MSG_LEADER:
                    done = true;
                    break;

                // Nothing to see here.  Really!
                case Message::MSG_NULL:
                    break;
                default:
                    throw runtime_error(
                            string{"Process thread received unexpected message: "} + msg.toString());
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
}

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

    // Am I still the leader?
    bool isLeader = true;

    // Am I done?
    bool done = false;

    // Which of my neighbors is my parent?
    int parent = -1;

    // Create initial explore message
    Message explore( Message::MSG_EXPLORE, node_id );

    // Vector of messages to send in next round.
    // Initially we send (EXPLORE, node_id) to everyone
    vector<Message> toSend( numNbrs, Message{ Message::MSG_EXPLORE, node_id});
    while (!done) {
        round++;
        
        // Wait for the next "TICK"
        Message msg(master_fd);

        fout << "Node " << node_id << " received message from fd " << master_fd <<
                ": " << msg.toString() << endl;

        // Send messages to all my neighbors
        for( int i=0; i<neighbor_fds.size(); i++ ) {
        	fout << "Sending to fd " << neighbor_fds[i] << ": " <<
        		 toSend[i].toString() << endl;
        	toSend[i].send( neighbor_fds[i] );
        }

        // By default everyone gets a NULL message
        std::fill( toSend.begin(), toSend.end(), Message{Message::MSG_NULL} );

        // Receive messages and process...
        for( int i=0; i<neighbor_fds.size(); i++ ) {
        	Message msg( neighbor_fds[i] );
        	int receivedId;

            fout << "Node " << node_id <<
            		" received message from fd " << neighbor_fds[i] <<
                    ": " << msg.toString() << endl << flush;
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
                    	isParent = false;
                        maxId = receivedId;
                        std::fill( isDone.begin(), isDone.end(), false );
                        std::fill( isChild.begin(), isChild.end(), false );
                        // Forward this ID on to all neighbors (but the parent)
                        for( int j=0; j<numNbrs; j++ ) {
                        	if ( j == parent ) continue;

                        	toSend[j] = msg;
                        }
                    } else if ( receivedId == maxId ) {
                    	// If we get maxId from another source, let him know
                    	// he's not our parent...
                    	toSend[i] = Message{Message::MSG_REJECT};
                    } // else -- we don't need to worry about if we got
                      // an EXPLORE smaller than our maxId.
                      // since we're going to send a new EXPLORE out within
                      // the next round.
                    break;
                case Message::MSG_REJECT:
                    isDone[i] = true;
                    allDone = true;
                    for( int j=0; j<isDone.size(); j++ ) {
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
                    for( int j=0; j<isDone.size(); j++ ) {
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
                case Message::MSG_LEADER:
                    done = true;
                    break;
                default:
                    throw runtime_error(
                            string{"Process thread received unexpected message: "} + msg.toString());
            }
        }
        
        Message done(Message::MSG_DONE, round);
        fout << "Node " << node_id << " sending message " << done.toString() << " to master thread." endl;
        done.send(master_fd);
    }

    // If we get here, we're done.  Wootwoot!
    ostringstream os;
    
    os << "Node " << node_id << " terminating..." << endl;
    os << "    Leader elected = " << maxID << endl;
    if ( parent != -1 ) os << "    My parent = " << neighbor_ids[parent] << endl;
    else                os << "    Woohoo!! I'm the leader!" << endl;
    os << "    My children = ";
    for( int i=0; i<Nbrs; i++ ) {
        if ( isChild[i] ) os << neighbor_ids[i] << " ";
    }
    os << endl;
    cout << os.str();

    // Propagate the LEADER message down the tree...
    Message msg{ Message::MSG_LEADER, maxID );
    for( int j=0; j<isDone.size(); j++ ) {
        if ( isChild[j] ) msg.send( nbrs[i] );
    }
    
    // Let the Master thread know we're done and to stop listening to us.
    msg.send( master_fd );
}

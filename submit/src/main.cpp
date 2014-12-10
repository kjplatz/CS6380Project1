/*
 * main.cpp
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

/*
 * CS 6380 - Distributed Computing
 * Fall 2014
 *
 * Programming Assignment #2
 * This project implements the AsyncGHS Minimum Spanning Tree protocol
 * in a general network.
 *
 * The program consists of three main portions:
 *     The parse_config() function reads a configuration file
 *     The main() function performs the following task:
 *         Invokes parse_config() to determine the system configuration
 *         Creates a set of socketpair() connections to communicate between
 *             the master and child threads
 *         Creates a set of socketpair() connections to simulate communications
 *             channels between peer threads
 *         Creates a vector of threads to simulate the processes
 *         Runs the main control loop:
 *             - Sends a TICK to all processes
 *             - Waits for a ACK from all processes
 *             - If any process sends a DONE message, it then performs
 *               a BFS search of the MST formed by the algorithm
 *
 * We have tested this program on configurations of up to 32 nodes.
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

#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/sctp.h>

#include "CS6380Project2.h"
#include "Neighbor.h"
#include "Node.h"

using namespace std;

bool verbose=false;

int main( int argc, char** argv ) {
    int numNodes;
    vector<vector<float>> neighborWts;
    vector<int> nodeIds;
    vector<Node*> nodes;

    if ( argc == 1 ) {
        cerr << "Error: usage: " << argv[0] << " <cfg_file>" << endl;
        return 0;
    }

    string cfg_file { argv[1] };
    numNodes = parse_config( cfg_file, nodeIds, neighborWts );

    if ( argc > 2 && string {argv[2]} == string {"verbose"} ) verbose = true;

    cout << "Processed " << numNodes << " nodes." << endl;
    for( int i=0; i<numNodes; i++ ) {
        cout << "Node " << i << " has ID: " << nodeIds[i] << endl;
        cout << "    -- Neighbors: ";
        for( unsigned j=0; j<neighborWts[i].size(); j++ ) {
            if ( neighborWts[i][j] ) cout << nodeIds[j] << " <" << neighborWts[i][j] << "> ";
        }
        cout << endl;
    }

    // file descriptors for neighbors to talk
    vector<vector<Neighbor>> neighbors(numNodes);
    // file descriptors for master to talk to threads
    vector<int> master_fds;
    vector<thread*> threads;

    // We discovered that with a sufficiently dense graph of ~80 nodes,
    // we would run out of file descriptors.
    //
    // As a result, we will just go ahead and attempt to bump up our
    // resources to the hard limit at the beginning of the computation.
    //
    cout << "Attempting to request additional resources..." << flush;
    struct rlimit lim;
    getrlimit( RLIMIT_NOFILE, &lim );
    lim.rlim_cur = lim.rlim_max;
    if ( setrlimit( RLIMIT_NOFILE, &lim ) != 0 ) {
        string err = string { "Unable to setrlimit: " };
        err += strerror( errno );
        throw runtime_error( err );
    }
    cerr << " (successful)" << endl;

    // Set up all of the socket pairs between neighbors...
    int sockpair[2];

    for( int i=0; i<numNodes; i++ ) {

        // Use local socket pairs for communications -- this simulates network communications without
        // having to do the listen/connect rigamarole.
        if ( socketpair( AF_LOCAL, SOCK_SEQPACKET, 0, sockpair ) < 0 ) {
            string err( "Unable to create local socket pair: " );
            err += strerror( errno );
            cerr << err << endl << flush;

            throw runtime_error( err );
        }

        master_fds.push_back( sockpair[0] );

        // File descriptor we'll talk to our master on...
        int master_fd = sockpair[1];

        verbose && cout << "Thread " << i << " has socketpair " << sockpair[0] << ", " << sockpair[1] << endl;

        for( int j=0; j<numNodes; j++ ) {
            if ( i == j ) continue;   // A node can't be its own neighbor...
            if ( neighborWts[i][j] > 0 ) {

                // Note, we are using socketpair() here.  A (successful) call to
                // socketpair() returns two local socket file descriptors at either
                // endpoint of an established communications channel.  This allows
                // us to effectively model a distributed system via message-passing.
                // These operate much like other types of sockets, except that
                // all communications happen within the kernel.

            	// We use SEQPACKET socket pairs, because they observe message
            	// boundaries.
                if ( socketpair( AF_LOCAL, SOCK_SEQPACKET, 0, sockpair ) < 0 ) {
                    string err( "Unable to create local socket pair: " );
                    err += strerror( errno );
                    throw runtime_error( err );
                }

                // Add the appropriate neighbor object to each node's neighbor list...
                neighbors[i].push_back( Neighbor{ nodeIds[j], sockpair[0], neighborWts[i][j]} );
                neighbors[j].push_back( Neighbor{ nodeIds[i], sockpair[1], neighborWts[i][j]} );
                verbose && cout << "Creating socketpair [" << sockpair[0] << ", " << sockpair[1] << "] for nodes "
                                << i << " <-> " << j << endl;

                // Set neighbors[j][i] to false so we don't create duplicate channels.
                neighborWts[j][i] = 0;
            }
        }

        // Create a new array of Node objects
        Node *n = new Node( nodeIds[i], master_fd, neighbors[i] );
        nodes.push_back( n );
        //
        // Spawn a new thread and push it back into the thread vector array...
        //
        //    Note, in C++, when creating a thread for a member method,
        //    you pass in the owner object as the first parameter of the
        //    thread constructor.
        threads.push_back( new thread( &Node::run, n ) );
        threads.back()->detach();
    }

    int round=0;
    sleep(1);

    int leader = -1;
    queue<int> bfsQ;
    while( leader < 0 ) {
        round++;
        usleep(DELAY);

        Message tick( Message::MSG_TICK, round );
        // Send a tick to everyone
        cout << "Sending to " << master_fds.size() <<
             " threads: " << tick.toString() << endl;
        for( auto fd : master_fds ) {
            tick.send( fd );
        }

        usleep(DELAY);
        int doneCount=0;
        // Read a message from each process file descriptor... this will either be an
        // ACK message or a DONE message.
        for( unsigned i=0; i<master_fds.size(); i++ ) {
            // Get the message and parse it...
            Message msg( master_fds[i] );

            switch( msg.msgType ) {
            /*
             * The protocol has finished correctly.  Go ahead and gather the BFS
             * information
             */
            case Message::MSG_DONE:
                cout << "Got message from fd " << master_fds[i] << ": " << msg.toString() << endl;
                leader = msg.round;
                bfsQ.push( leader );
                break;

            /*
             * Thread i is done with this tick.
             */
            case Message::MSG_ACK:    // I don't know, but I'm done with this round.
                doneCount++;
                break;

            /*
             * WTF?  How did we get here?
             */
            default:
                throw runtime_error(
                    string {"Master thread received unexpected message: "} +
                    msg.toString() );
            }
        }
        cout << "Received ACK from " << doneCount << " threads." << endl;
    }

    /*
     * Perform a BFS walk through the MST, and send a
     * REPORT to each node in turn.
     */
    Message done{ Message::MSG_REPORT };
    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
    cout << ">>> Node " << leader << " indicates that we're done! <<<" << endl;
    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
    while( !bfsQ.empty() ) {
        int next = bfsQ.front();
        bfsQ.pop();

        unsigned i;
        for( i=0; i<nodeIds.size(); i++ ) {
        	if ( nodeIds[i] == next ) break;
        }

        done.send( master_fds[i] );
        cout << "Node " << next << " has children:";
        bool gotDone = false;
        do {
            Message msg( master_fds[i] );
            /*
             * Message of type REPORT <ID> indicates that node <ID>
             * is one of my children
             */
            if ( msg.msgType == Message::MSG_REPORT ) {
            	bfsQ.push( msg.round );
            	cout << " " << msg.round;
            } else if ( msg.msgType == Message::MSG_DONE ) {
            	// Done message means I don't have any more children in the MST
            	gotDone = true;
            }
        } while ( !gotDone );
        cout << endl;
    }
}




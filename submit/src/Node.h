/*
 * Node.h
 * CS 6380, Distributed Computing
 * Fall 2014, Project #2
 *     Kenneth Platz
 *     Brian Snedic
 *     Josh Olson
 *
 * Header file to define the "Node" class
 */

#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#include <fstream>
#include <queue>
#include <random>
#include <thread>
#include <vector>

#include "CS6380Project2.h"
#include "Edge.h"
#include "Message.h"
#include "Neighbor.h"

class Node {
    Neighbor myParent;                 // My parent

    bool isLeader = true;
    bool hasSentTest = true;
    bool hasSentConnect = false;

    int myId;                          // My Node ID
    Edge myComponent;                  // My component ID
    int  myLevel = 0;                  // My level

    int round;                         // Current round.

    std::vector<Message> connectQ;     // Queued "Connect" messages
    std::vector<Message> testQ;        // Queued "Test" messages

    int masterFd;

    std::vector<Neighbor> neighbors;
    std::vector<Neighbor> candidates;  // Candidate edges
    std::vector<Neighbor> trees;       // Tree edges
    std::vector<Neighbor> rejects;     // Rejected edges

    std::vector<std::queue<Message>> msgQs;   // Queued messages from each neighbor

    std::ofstream fout;

    Message bufferedReport;            // Buffered report message
    Message sentConnect;               // If we've got an outstanding Connect2Me message

    std::mt19937 generator;
	std::uniform_int_distribution<int> distribution;

	void queueIncoming();              // Process incoming messages and place into queues

	// Convergecast messages to the root if necessary..
	void doConvergecast();

	// Send a connect2Me message, and do other stuff if necessary...
	void sendConnect2Me();

	// Report back to the main thread...
	void doReport();

	// Start a new phase
	void initiatePhase();

	// Handle any outstanding connection requests...
	void processConnectQ();

	// Handle any outstanding test requests...
	void processTestQ();

	// Create a new component (and go up in level)
	void createNewComponent( const Neighbor& nbr );

	// Handle each message type
	void processAccept( const Neighbor& nbr, const Message& msg );
	void processChangeRoot( const Neighbor& nbr, const Message& msg );
	void processConnect( const Neighbor& nbr, const Message& msg );
	void processConnect2Me( const Neighbor& nbr, const Message& msg );
	void processInitiate( const Neighbor& nbr, const Message& msg );
	void processReject( const Neighbor& nbr, const Message& msg );
	void processReport( const Neighbor& nbr, const Message& msg );
	void processTest(const Neighbor& nbr, const Message& msg);
public:
    Node() = delete;
    Node( Node&& ) = default;
    Node( const Node& ) = delete;
    Node( int myId, int mfd, std::vector<Neighbor> nbrs );

    void run();
};




#endif /* SRC_NODE_H_ */

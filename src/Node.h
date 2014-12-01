/*
 * Node.h
 *
 *  Created on: Nov 30, 2014
 *      Author: ken
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

    std::mt19937 generator;
	std::uniform_int_distribution<int> distribution;

	void queueIncoming();              // Process incoming messages and place into queues
	void processTest(const Neighbor& nbr, const Message& msg);        // Process test message...
	void processInitiate( const Neighbor& nbr, const Message& msg );  // Process initiate message
public:
    Node() = delete;
    Node( Node&& ) = default;
    Node( const Node& ) = delete;
    Node( int myId, int mfd, std::vector<Neighbor> nbrs );

    void run();
};




#endif /* SRC_NODE_H_ */

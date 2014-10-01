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

    while (round < 10) {
        round++;
        Message msg(master_fd);

        fout << "Node " << node_id << " received message from fd " << master_fd <<
                ": " << msg.toString() << endl;

        Message explr(Message::MSG_EXPLORE, maxId);

        //Send maxId to all neighbors
        for (auto fd : neighbor_fds) {
            explr.send(fd);
        }

        //Receive message from all neighbors
        for (auto fd : neighbor_fds) {
            int receivedId;

            Message msg(fd);

            fout << "Node " << node_id << " received message from fd " << fd <<
                    ": " << msg.toString() << endl;

            switch (msg.msgType) {
                case Message::MSG_EXPLORE:
                    receivedId = msg.id;
                    if (receivedId > maxId) {
                        maxId = receivedId;
                    }
                default:
                    throw runtime_error(
                            string{"Process thread received unexpected message: "} + msg.toString());
            }
        }
        
        if (round == 9) {
            Message done(Message::MSG_LEADER);
            fout << "Node " << node_id << " sending message " << done.toString() << endl;
            done.send(master_fd);
        } else {
            Message done(Message::MSG_DONE, round);
            fout << "Node " << node_id << " sending message " << done.toString() << endl;
            done.send(master_fd);
        }
    }
}
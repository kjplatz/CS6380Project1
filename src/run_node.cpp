/*
 * run_thread.cpp
 *
 *  Created on: Sep 25, 2014
 *      Author: ken
 */

#include <vector>

#include "CS6380Project1.h"

using namespace std;
//
// Function to run a node
// Parameters:
//    node_id     : The node ID to represent
//    master_fd   : Socket file descriptor to talk to/from master thread
//    neighbor_fds: The socket file descriptors of my neighbors
void run_node( int node_id, int master_fd,  vector<int> neighbor_fds ) {
	int maxId = node_id;
}



/*
 * CS6380Project1.h
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

#ifndef CS6380PROJECT1_H_
#define CS6380PROJECT1_H_

#include <iosfwd>
#include <string>
#include <vector>

#if __cplusplus <= 199711L
#    error  "This project requires a compiler that complies with the"\
            " ISO/IEC 14882:2011 standard (aka C++11)"
#endif

#define TESTING

// Parse the configuration file.
// Parameters:
//    cfg_file    : Name of the configuration file
//    return_vals : (Reference to) vector of ints to return the node ID's in
//    return_nbrs : (Reference to) vector of vector of ints to return the neighbor array
// Return value:
//    int         : Number of neighbors found

int parse_config( std::string cfg_file,
		          std::vector<int>& return_vals,
		          std::vector<std::vector<bool>>& return_nbrs );

//
// Function to run a node
// Parameters:
//    node_id     : The node ID to represent
//    master_fd   : Socket file descriptor to talk to/from master thread
//    neighbor_fds: The socket file descriptors of my neighbors
void run_node( int node_id, int master_fd,  std::vector<int> neighbor_fds );

extern bool verbose;

#include "Message.h"

extern std::fstream results;
#endif /* CS6380PROJECT1_H_ */

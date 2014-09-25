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
#include <vector>

#if __cplusplus <= 199711L
#    error  "This project requires a compiler that complies with the"\
            " ISO/IEC 14882:2011 standard (aka C++11)"
#endif

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
//    neighbor_fds: The socket file descriptors of my neighbors
int run_node( int node_id, std::vector<int> neighbor_fds );

#include "Message.h"

#endif /* CS6380PROJECT1_H_ */

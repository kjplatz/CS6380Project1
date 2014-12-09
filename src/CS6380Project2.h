/*
 * CS6380Project2.h
 *
 *  Created on: Sep 25, 2014
 *  Renamed to CS6380Project2.h on Nov 11, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

#ifndef CS6380PROJECT2_H_
#define CS6380PROJECT2_H_

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
                  std::vector<std::vector<float>>& return_nbrs );

//
// Function to run a node
// Parameters:
//    node_id     : The node ID to represent
//    master_fd   : Socket file descriptor to talk to/from master thread
//    neighbor_fds: The socket file descriptors of my neighbors
#include "Neighbor.h"
void run_node( int node_id, int master_fd,  std::vector<Neighbor> neighbor_fds );

extern bool verbose;

// Delay between ticks (in milliseconds)
const int DELAY=100000;

#include "Message.h"

#endif /* CS6380PROJECT1_H_ */

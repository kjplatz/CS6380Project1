/*
 * main.cpp
 *
 *  Created on: Sep 25, 2014
 *  Authors:
 *      Kenneth Platz
 *      Joshua Olson
 *      Brian Snedic
 */

#include <iostream>
#include <string>
#include <vector>

#define TESTING

#include <sys/socket.h>
#include <sys/types.h>

using namespace std;

int main( int argc, char** argv ) {
	int numNodes;
	vector<vector<bool>> neighbors;
	vector<int> nodeIds;

	if ( argc == 1 ) {
		cerr << "Error: usage: " << argv[0] << " <cfg_file>" << endl;
		return 0;
	}

	string cfg_file{ argv[1] };
	// numNodes = parse_config( cfg_file, nodeIds, neighbors );

#ifdef TESTING
	numNodes = 4;
	nodeIds = { 11, 22, 33, 44 };
	neighbors = {
			{ 0, 1, 1, 1 },
			{ 1, 0, 1, 1 },
			{ 1, 1, 0, 1 },
			{ 1, 1, 1, 0 }
	};
#endif TESTING

    vector<vector<int>> neighbor_fds;  // file descriptors for neighbors to talk
    vector<int> master_fds;            // file descriptors for master to talk to threads

    // Set up all of the socket pairs between neighbors...
    for( int i=0; i<numNodes; i++ ) {
    	for( int j=0; j<numNodes; j++ ) {
    		if ( i == j ) continue;
    		if ( neighbors[i][j] ) {

    		}
    	}
    }

}




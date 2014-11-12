/*
 * parse_config.cpp
 *     Authors:
 *     Brian Snedic
 *     Josh Olson
 *     Kenneth Platz
 *
 * Updated to handle floating-point edge weights.
 */
#include <iostream>
#include <fstream>
#include <vector>
#include "CS6380Project2.h"
using namespace std;

int parse_config( string cfg_file,
		          vector<int>& return_vals,
				  vector<vector<float>>& return_nbrs) {
    std::ifstream inf(cfg_file.c_str());
    int nds,x;
    float f;

    //get (n)number of nodes
    if(!inf.fail()) {
        inf >> nds;
    }

    //get (ids[n]) list of id values for the nodes
    for(int i=0; i<nds; i++) {
        inf >> x;
        return_vals.push_back(x);
    }

    //get (nbrs) neighbors
    for(int i=0; i<nds; i++) {
        std::vector<float> my_nbrs;
        for(int j=0; j<nds; j++) {
            inf >> f;
            my_nbrs.push_back(f);
        }
        return_nbrs.push_back(my_nbrs);
    }

    return nds;
}

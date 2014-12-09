/*
 * Edge.h
 *
 *  Created on: Nov 11, 2014
 *  CS6380 - Distributed Computing
 *  Fall 2014
 *  Programming Assignment #2
 *
 *  Definition of an Edge object.
 *     Most importantly, we define as an edge as a tuple <v1, v2, w>
 *     Where v1 and v2 are vertices, with v1 < v2.
 *     W is a floating-point weight.
 *
 *     We also define constructors, a to_string() method so we can output
 *     to a stream, and we define the important comparators, so we can
 *     intuitively compare edges against each other.
 */

#ifndef __EDGE_H_
#define __EDGE_H_

#include <algorithm>
#include <sstream>
#include <string>

class Edge {
    int v1, v2;
    float weight;
public:
    Edge() = default;
    Edge( int x, int y, float w ) : weight(w) {
    	v1 = std::min(x,y);
    	v2 = std::max(x,y);
    }
    Edge( const Edge& ) = default;
    Edge( Edge&& ) = default;
    Edge( const std::string& s ) {
    	std::istringstream is( s );
    	std::string reject;
    	is >> reject;  // Get first <
    	is >> v1;
    	is >> v2;
    	is >> weight;
    }

    Edge& operator=( const Edge& ) = default;

    bool operator<( const Edge& e2 ) const {
    	return ( weight == e2.weight ) ?
    			    (( v1 == e2.v1 ) ? ( v2 < e2.v2 ) : ( v1 < e2.v1 )) :
	    			( weight < e2.weight );
    }
    bool operator<=( const Edge& e2 ) const {
    	return ( weight == e2.weight ) ?
    			    (( v1 == e2.v1 ) ? ( v2 <= e2.v2 ) : ( v1 < e2.v1 )) :
	    			( weight < e2.weight );
    }
    bool operator>( const Edge& e2 ) const {
    	return ( weight == e2.weight ) ?
    			    (( v1 == e2.v1 ) ? ( v2 > e2.v2 ) : ( v1 > e2.v1 )) :
					( weight > e2.weight );
    }
    bool operator>=( const Edge& e2 ) const {
    	return ( weight == e2.weight ) ?
    			    (( v1 == e2.v1 ) ? ( v2 >= e2.v2 ) : ( v1 > e2.v1 )) :
					( weight > e2.weight );
    }
    bool operator!=( const Edge& e2 ) const {
        return ( weight != e2.weight ) ||
        	   ( v1 != e2.v1 ) ||
    		   ( v2 != e2.v2 );
    }
    bool operator==( const Edge& e2 ) const {
        return ( weight == e2.weight ) &&
        	   ( v1 == e2.v1 ) &&
    		   ( v2 == e2.v2 );
    }

    std::string to_string() const {
    	std::string str = std::string{"< "} + std::to_string(v1) + std::string{ " " } +
    		   std::to_string( v2 ) + std::string{ " " } + std::to_string( weight ) +
			   std::string{ " >" };
        return str;
    }

    float getWeight() const { return weight; }

    bool isMine( int node ) const {
    	return v1 == node || v2 == node;
    }

    bool getOther( int node ) const {
    	return (v2 == node) ? v1 : v2;
    }
};

#endif /* SRC_EDGE_H_ */

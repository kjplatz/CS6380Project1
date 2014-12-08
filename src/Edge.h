/*
 * Edge.h
 *
 *  Created on: Nov 11, 2014
 *      Author: ken
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

    bool operator<( const Edge& e2 ) {
    	return ( weight == e2.weight ) ?
    			    (( v1 == e2.v1 ) ? ( v2 < e2.v2 ) : ( v1 < e2.v1 )) :
	    			( weight < e2.weight );
    }
    bool operator<=( const Edge& e2 ) {
    	return ( weight == e2.weight ) ?
    			    (( v1 == e2.v1 ) ? ( v2 <= e2.v2 ) : ( v1 < e2.v1 )) :
	    			( weight < e2.weight );
    }
    bool operator>( const Edge& e2 ) {
    	return ( weight == e2.weight ) ?
    			    (( v1 == e2.v1 ) ? ( v2 > e2.v2 ) : ( v1 > e2.v1 )) :
					( weight > e2.weight );
    }
    bool operator>=( const Edge& e2 ) {
    	return ( weight == e2.weight ) ?
    			    (( v1 == e2.v1 ) ? ( v2 >= e2.v2 ) : ( v1 > e2.v1 )) :
					( weight > e2.weight );
    }
    bool operator!=( const Edge& e2 ) {
        return ( weight != e2.weight ) ||
        	   ( v1 != e2.v1 ) ||
    		   ( v2 != e2.v2 );
    }
    bool operator==( const Edge& e2 ) {
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

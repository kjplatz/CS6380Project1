/*
 * Neighbor.h
 *
 *  Created on: Nov 11, 2014
 *      Author: ken
 */

#ifndef __NEIGHBOR_H
#define __NEIGHBOR_H
#include <functional>
#include <string>
#include "Edge.h"
class Neighbor {
	int    id;     // Node identifier
	int    fd;     // File descriptor
	float  weight;
	int    idx;    // Index into node vector

    enum EdgeType {
	    EDGE_CANDIDATE = 0,
	    EDGE_REJECTED,
	    EDGE_TREE
    } edgeType;
    bool responded = false;
public:
	Neighbor( int i, int f, float w ) :
		id(i), fd(f), weight(w), edgeType(EDGE_CANDIDATE), responded(false) {};

	Neighbor() = delete;
	Neighbor( const Neighbor& ) = default;
	Neighbor( Neighbor&& ) = default;

	Neighbor& operator=( const Neighbor& ) = default;
	Neighbor& operator=( Neighbor&& ) = default;

	Edge getEdge( const int me ) const {
		return Edge{ me, id, weight };
	}

	float getWeight() const { return weight; }
	int getId() const { return id; }
	int getFd() const { return fd; }
	EdgeType getType() const { return edgeType; }
	void changeType( const EdgeType et ) { edgeType = et; }

	bool operator<( const Neighbor& n ) const {
		return (weight == n.weight) ? (id < n.id) : (weight < n.weight);
	}

	bool operator>( const Neighbor& n ) const {
		return (weight > n.weight) || (id > n.id);
	}

	bool operator==( const Neighbor& n ) const {
		return id == n.id;
	}

	bool operator!=( const Neighbor& n ) const {
		return (weight != n.weight) || (id != n.id) || (fd != n.fd);
	}

	std::string to_string() const {
		return std::to_string(id) + std::string{ " <" } +
			   std::to_string(weight) + std::string{ ">"};
	}

	void setIdx( int x ) { idx=x; }
	int getIdx() const { return idx; }
	bool hasResponded() const { return responded; }
	void clearResponded() { responded = 0; }
	void setResponded() { responded = 1; }
};
#endif




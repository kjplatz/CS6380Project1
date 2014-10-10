CS6380Project1
==============

CS 6380 Spring 2014 Project 1

Team Members:
1. Kenneth Platz
2. Brian Snedic
3. Joshua Olson

Instructions:
1. This should be run on CS1.utdallas.edu or CS2.utdallas.edu
2. Within the CS6380Project1 directory provided, run the "make" command.  This will create a "CS6380Project1" executable in the 
   CS6380Project1/bin directory.
3. Navigate to the CS6380Project1/run directory.
4. Run the CS6380Project1 executable with the first argument being the text file containing the graph configuration.
5. Output will display to console.  Additionally, each node will have a .log file in the /run directory describing its activity.

Example Usage:
{cslinux1:~} cd CS6380Project1
{cslinux1:~} make
{cslinux1:~} cd run
{cslinux1:~} ../bin/CS6380Project1 input6.txt
{cslinux1:~} less node3.log

Sample Test Case:
Input: CS6380Project1/run/input6.txt
Output: CS6380Project1/run/output6.txt
Detailed log files: CS6380Project1/run/logfiles.tar.gz

Visual of the Graph: https://drive.google.com/file/d/0B2fmtyogc8AINVpENXdBdTBPSnM/view?usp=sharing
Open with draw.io.

We have tested this on graphs up to 80 nodes successfully.  The project does have some limitations on the size of the system that it can 
simulate.  We use local UNIX domain sockets to simulate network connections between systems; as a result, each node requires two file 
descriptors (to communicate with the master) and each edge between two nodes requires 2 socket descriptors.  Likewise, each node creates its 
own local log file (which requires another file descriptor).  So the number of file descriptors consumed by a given graph is 3*|V| + 2*|E|.  We 
use a handful of other file descriptors for stdin, stdout, stderr, and a couple of utility files.  (6 total).  This sum, 3*|V| + 2*|E| + 5 o
must be less than the system's hard per-process file descriptor limit, which can be displayed via the command 'ulimit -nH'.  For most Linux
systems, this is 4,096.

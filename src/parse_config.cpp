/*
 * parse_config.cpp
 *     Authors:
 *     Brian Snedic
 *     Josh Olson
 *     Kenneth Platz
 */
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

//int parse_config(std::string, std::vector<int>&, std::vector< std::vector<bool> >&);
//
//int main(){
//
//   std::vector<int> ids;
//   std::vector< std::vector<bool> > nbrs;
//
//   int n = parse_config("input.txt", ids, nbrs);
//
//   cout << "\nn = "<< n << "\nids = \n";
//
//   for( std::vector<int>::iterator itr=ids.begin(); itr!=ids.end(); itr++){
//      cout << "\t" << *itr;
//   }
//
//   cout << "\nnbrs = \n";
//
//   for( std::vector< std::vector<bool> >::iterator itr=nbrs.begin(); itr!=nbrs.end(); itr++){
//      std::vector<bool> my_nbrs = (*itr);
//      for( std::vector<bool>::iterator itr2=my_nbrs.begin(); itr2!=my_nbrs.end(); itr2++){
//         cout << "\t" << (*itr2);
//      }
//      cout << endl;
//   }
//}

int parse_config( string cfg_file, vector<int>& return_vals, vector<vector<bool>>& return_nbrs){
   std::ifstream inf(cfg_file.c_str());
   int nds,x;

   //get (n)number of nodes
   if(!inf.fail()){ inf >> nds; }

   //get (ids[n]) list of id values for the nodes
   for(int i=0; i<nds; i++){
      inf >> x; 
      return_vals.push_back(x);
   }

   //get (nbrs) neighbors 
   for(int i=0; i<nds; i++){
      std::vector<bool> my_nbrs;
      for(int j=0; j<nds; j++){
         inf >> x;
         my_nbrs.push_back(x);
      }
      return_nbrs.push_back(my_nbrs);
   }
   
   return nds;
}

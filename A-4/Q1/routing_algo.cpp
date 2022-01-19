#include "node.h"
#include <iostream>
#define pb push_back
using namespace std;

int getCost(vector<RoutingEntry> routingEntries, string dstip)
{
  for(RoutingEntry e : routingEntries)
  {
    if(e.dstip==dstip)
      return e.cost;
  }
  return 1;
}

void printRT(vector<RoutingNode*> nd){
/*Print routing table entries*/
  for (int i = 0; i < nd.size(); i++) {
    nd[i]->printTable();
  }
}

void run(vector<RoutingNode*> nd)
{
  vector<struct routingtbl> routingTables; // store routing tables of all nodes
  bool hasConverged = true;
  int n = nd.size(), count=1;

  // while there is no convergence
  while(true)
  {
    // print routing table for each iteration
    cout<<endl<<"Iteration: "<<count<<endl;
    count++;
    printRT(nd);
    
    routingTables.clear();
    hasConverged=true;
        
    for(int i=0; i<n; i++)
    {
      nd[i]->sendMsg(); // send routing table to neighbours
      routingTables.pb(nd[i]->getTable()); // push routing table into duplicate vector
    }
    
    // check for convergence
    for(int i=0; i<n; i++)
    {

      // compare table sizes
      if(nd[i]->getTable().tbl.size() != routingTables[i].tbl.size())
      {
        hasConverged=false;
        break;
      }

      // compare previous and current tables
      for(int j=0; j<routingTables[i].tbl.size(); j++)
      {

        RoutingEntry prev = nd[i]->getTable().tbl[j];
        RoutingEntry curr = routingTables[i].tbl[j];

        // compare all values of all the tables
        bool b1 = prev.dstip == curr.dstip;
        bool b2 = prev.nexthop == curr.nexthop;
        bool b3 = prev.ip_interface == curr.ip_interface;
        bool b4 = prev.cost == curr.cost;
        
        // if no change for this node, move on to the next
        if(b1 && b2 && b3 && b4)
          continue;

        else
        {
          hasConverged=false;
          break;
        }
      }

    }

      if(hasConverged)
        break;
  }
}

void routingAlgo(vector<RoutingNode*> nd){
  //Your code here

  // run Routing Information Protocl (RIP) for initial input
	 run(nd);
   cout<<endl;
   printRT(nd);
}

// table updation goes here
// update self's table via msg's table
void RoutingNode::recvMsg(RouteMsg *msg) {

  vector<RoutingEntry> selfTable = mytbl.tbl;
  vector<RoutingEntry> recvTable = msg->mytbl->tbl;

  int n = selfTable.size();
  int m = recvTable.size();
  int i=0;

  // for every entry in the msg table
  while(i<m)
  {
    
    // push it in self table
    selfTable.pb(recvTable[i]);
    selfTable[selfTable.size()-1].cost++;
    selfTable[selfTable.size()-1].nexthop=msg->from;
    selfTable[selfTable.size()-1].ip_interface=msg->recvip; 

    bool alreadyPresent = false; // check if this entry is already present or not

    // traverse self's table to find things out
    for(int j=0; j<n; j++)
    {

      // if destination IPs are same, then the entry is already present
      if(selfTable[j].dstip == recvTable[i].dstip)
      {
        alreadyPresent = true; // this entry is already present
        int c = getCost(recvTable,selfTable[j].ip_interface); // get cost

        // check if a lower cost path is available or not
        if(selfTable[j].cost > recvTable[i].cost+c)
         {
            selfTable[j].nexthop = msg->from;
            selfTable[j].ip_interface = msg->recvip;
            selfTable[j].cost = min(16,recvTable[i].cost+c);
         }
      }
    }

    // if last node is already present, remove it
    if(alreadyPresent)
      selfTable.pop_back(); 
    i++;
  }
  mytbl.tbl = selfTable;
}

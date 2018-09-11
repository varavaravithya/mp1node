/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */

MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */

MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */

int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */

int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}


/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */


void MP1Node::nodeStart(char *servaddrstr, short servport) {
  
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
      
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
	
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */

int MP1Node::initThisNode(Address *joinaddr) {
  /*
   * This function is partially implemented and may require changes
   */
  int id = *(int*)(&memberNode->addr.addr);
  int port = *(short*)(&memberNode->addr.addr[4]);
  
  memberNode->bFailed = false;
  memberNode->inited = true;
  memberNode->inGroup = false;
  // node is up!
  memberNode->nnb = 0;
  memberNode->heartbeat = 0;
  memberNode->pingCounter = TFAIL;
  memberNode->timeOutCounter = -1;
  initMemberListTable(memberNode);
  
  return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
      
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
	cout << "Starting up Group\n";
#endif
        memberNode->inGroup = true;
    }
    else {
      
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

	//memberNode->heartbeat = 3;
	
        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
	
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */

int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */

void MP1Node::nodeLoop() {
  
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */

void MP1Node::checkMessages() {
    void *ptr;
    int size;
    MessageHdr *msg;
    
    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *) ptr, size);
    }
    
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */

bool MP1Node::recvCallBack(void *env, char *data, int size ) {
  /*
   * Your code goes here
   */
  
  MessageHdr *recvMsg;
  Address *recvAddr;

  int id = 0;
  short port = 0;
  long heartbeat = 0;
  long timestamp = 0;
  
#ifdef DEBUGLOG
  static char s[1024];
#endif

  recvMsg = (MessageHdr *) data; // Received message type.
  
  recvAddr = (Address *) ((MessageHdr *) data + 1); // Received message address.

#ifdef DEBUGLOG
  cout << "------------------------------------\n";
  cout << "recvCallBack() at time: " << par->getcurrtime() << " This node: ";
  cout << memberNode->addr.getAddress() << " From node: ";
  printAddress(recvAddr);
  printf("Received Message Type: %d\n", recvMsg->msgType);
  cout << "------------------------------------\n";
#endif
  
  id = recvAddr->addr[0];
  port =  recvAddr->addr[4];
  heartbeat = *((long *) ((char *)(recvMsg+1) + 1 + sizeof(memberNode->addr.addr)));
  timestamp =  par->getcurrtime();

  printf("Address: %d\t", id);
  printf("Port: %d\n",  port);
  printf("Heartbeat: %ld\t", heartbeat);
  printf("Timestamp: %ld\n",  timestamp);
  
  switch(recvMsg->msgType){
  case JOINREQ:
    cout << "Receive Join Request Message\n";
    // check existing memberList Table
    
    if(checkExistingMemberListTable(recvAddr)){
      cout << "Insert Member\n";
      printAddress(recvAddr);
      MemberListEntry* newEntry = new MemberListEntry(id, port, heartbeat, timestamp);
      memberNode->memberList.insert(memberNode->memberList.end(), *newEntry);
    }
    
    // send back join REPLY together with memberListTable

    log->logNodeAdd(&(memberNode->addr), recvAddr);
    sendJoinReplyMsg(recvAddr);
    
    break;
  case JOINREP:
    cout << "Receive Join Reply Message\n";
    
    memberNode->inGroup = true;
    
    log->logNodeAdd(&(memberNode->addr), &(memberNode->addr));
    
    if(checkExistingMemberListTable(recvAddr)){
      cout << "Insert Member\n";
      printAddress(recvAddr);
      MemberListEntry* newEntry = new MemberListEntry(id, port, heartbeat, timestamp);
      memberNode->memberList.insert(memberNode->memberList.end(), *newEntry);
    }
    
     break;
  case TLBUPDATE:
    break;
  case DUMMYLASTMSGTYPE:
    break;
  }
  return 1;
}

int MP1Node::sendJoinReplyMsg(Address *desAddr){

  // send back join REPLY together with memberListTable 
  MessageHdr *msg;
  
  size_t msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr.addr) + sizeof(long) + 1;

  msg = (MessageHdr *) malloc(msgsize * sizeof(char));
  msg->msgType = JOINREP;
  memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
  memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
  
  
  // send JOINREP message to introducer member
  
  emulNet->ENsend(&memberNode->addr, desAddr, (char *) msg, msgsize);
  
  free(msg);
  return 1;
}

int MP1Node::checkExistingMemberListTable(Address *chkAddr){
  int testAddr, testPort;
  
  cout << "Check Existing Member List Table\n";
  testAddr = chkAddr->addr[0];
  testPort =  chkAddr->addr[4];
  printf("Address: %d\t", testAddr);
  printf("Port: %d\n",  testPort);
  
  for(std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end(); ++it) {
    if((*(it)).getid() == testAddr &&  (*(it)).getport() == testPort){
      cout << "Found\n";
      return 0;
    }  
  }
  
  return 1; 
}




/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */

void MP1Node::nodeLoopOps() {
  
  /* Your code goes here
     print out MemberListEntry
     serialize the packet
     select member 
     send TLBUPDATE message */

  char *exTable;
  int i;
  MessageHdr *msg;
  size_t  msgsize;
  vector<MemberListEntry> unpackMemberList;
  MemberListEntry unpackMemberEntry;
  
  memberNode->heartbeat++;
  
  cout << "nodeLoopOps\t";
  printAddress(&memberNode->addr);

  cout << "MemberListEntry TABLE:\n";
  for(std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end(); ++it) {
    cout << "MemberListEntry:\t" << (*(it)).getid() << ":" << (*(it)).getport()  << ":" <<  (*(it)).getheartbeat()  << ":" <<
      (*(it)).gettimestamp() << "\n";
  }
  
  exTable = packUpTable();

  
  msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr.addr) + sizeof(long) + 1 +
    sizeof(MemberListEntry) * memberNode->memberList.size();
  
  msg = (MessageHdr *) malloc(msgsize * sizeof(char));
  msg->msgType = TLBUPDATE;
  memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
  memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
  memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr) + sizeof(memberNode->heartbeat), exTable, 
	 sizeof(MemberListEntry) * memberNode->memberList.size());
  
  // send TABUPDATE message to introducer member

  // pack destination address and send TABUPDATE to all the member.

  // emulNet->ENsend(&memberNode->addr, desAddr, (char *) msg, msgsize);
  
  free(msg);

  for(i = 0; i < memberNode->memberList.size(); i++){
    memcpy(&unpackMemberEntry, exTable + i*sizeof(MemberListEntry), sizeof(MemberListEntry));    
    unpackMemberList.insert(unpackMemberList.end(), unpackMemberEntry);
  }

  for(std::vector<MemberListEntry>::iterator it = unpackMemberList.begin(); it != unpackMemberList.end(); ++it) {
    cout << "Unpack MemberListEntry:\t" << (*(it)).getid() << ":" << (*(it)).getport()  << ":" <<  (*(it)).getheartbeat()  << ":" <<
      (*(it)).gettimestamp() << "\n";
  }

  free(exTable);
  
  return;
}

char * MP1Node::packUpTable(){
  size_t  msgsize;
  char *data;
  int i = 0;
  
  msgsize = sizeof(MemberListEntry) * memberNode->memberList.size();

  data = (char *) malloc(msgsize);

  for(i = 0; i < memberNode->memberList.size(); i++){
    memcpy(data + i*sizeof(MemberListEntry), &memberNode->memberList[i], sizeof(MemberListEntry));    
  }

  
  return data;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */

Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */

void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}

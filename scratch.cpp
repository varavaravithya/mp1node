
//for(std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end(); ++it) {

/*
     MemberListEntry* newEntry = new MemberListEntry(id, port, heartbeat, timestamp);
    
        memberNode->memberList.insert(memberNode->memberList.end(), *newEntry);
*/

	
#ifdef DEBUGLOG
	cout << "------------------------------------\n";
	cout << "checkMessage() This Node\t" <<  memberNode->addr.getAddress() << "\t";;
	cout << "Time:" << par->getcurrtime() << endl;  
	msg =  (MessageHdr *) ptr;
	cout << "MessageType\t" << msg->msgType << endl;
	cout << "Message Size\t" << size << endl;
	cout << "Source address\t";
	printAddress((Address *) (msg+1));
	cout << "\n------------------------------------\n";
#endif

#ifdef DEBUGLOG
  cout << "------------------------------------\n";
  cout << "recvCallBack() at time: " << par->getcurrtime() << " This node: ";
  cout << memberNode->addr.getAddress() << " From node: ";
  printAddress((Address *) desaddr);
  cout << "------------------------------------\n";
#endif
  

#ifdef DEBUGLOG
  cout << "Member Node Address (in msg) ";
  printAddress((Address *) (msg+1));
  cout << "Heartbeat (in msg)";
  printf("%d\n", *((long *) (msg+1+sizeof(memberNode->addr.addr))));
  sprintf(s, "Send JOINREP to %d.%d.%d.%d:%d", desaddr->addr[0], desaddr->addr[1], desaddr->addr[2], desaddr->addr[3], *(short*)&desaddr->addr[4]);   
  cout << "------------------------------------\n";
  cout << "Send Join Reply at Time " << par->getcurrtime() << " From This node: ";
  cout << memberNode->addr.getAddress() << " To node: ";
  printAddress((Address *) desaddr);
  cout << "Member Node Address ";
  printAddress(&memberNode->addr);
  cout << "Destination Address ";
  printAddress(desaddr);
#endif



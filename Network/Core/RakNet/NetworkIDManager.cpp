/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#include "NetworkIDManager.h"
#include "NetworkIDObject.h"
#include "RakAssert.h"
#include "GetTime.h"
#include "RakSleep.h"
#include "SuperFastHash.h"
#include "RakPeerInterface.h"

using namespace RakNet;

STATIC_FACTORY_DEFINITIONS(NetworkIDManager,NetworkIDManager)

NetworkIDManager::NetworkIDManager()
{
	startingOffset = RakPeerInterface::Get64BitUniqueRandomNumber();
	Clear();
}
NetworkIDManager::~NetworkIDManager(void)
{

}
void NetworkIDManager::Clear(void)
{
	memset(networkIdHash,0,sizeof(networkIdHash));
}
NetworkIDObject *NetworkIDManager::GET_BASE_OBJECT_FROM_ID(NetworkID x)
{
	unsigned int hashIndex=NetworkIDToHashIndex(x);
	NetworkIDObject *nio=networkIdHash[hashIndex];
	while (nio)
	{
		if (nio->GetNetworkID()==x)
			return nio;
		nio=nio->nextInstanceForNetworkIDManager;
	}
	return 0;
}
NetworkID NetworkIDManager::GetNewNetworkID(void)
{
    while (GET_BASE_OBJECT_FROM_ID(++startingOffset))
        ;
    return startingOffset;
}
unsigned int NetworkIDManager::NetworkIDToHashIndex(NetworkID networkId)
{
//	return SuperFastHash((const char*) &networkId.guid.g,sizeof(networkId.guid.g)) % NETWORK_ID_MANAGER_HASH_LENGTH;
	return (unsigned int) (networkId % NETWORK_ID_MANAGER_HASH_LENGTH);
}
void NetworkIDManager::TrackNetworkIDObject(NetworkIDObject *networkIdObject)
{
	// 6/27/13 Automatically set this. If forgotten, then the destructor of NetworkIDObject will not call StopTrackingNetworkIDObject()
	networkIdObject->SetNetworkIDManager(this);

	RakAssert(networkIdObject->GetNetworkID()!=UNASSIGNED_NETWORK_ID);
	unsigned int hashIndex=NetworkIDToHashIndex(networkIdObject->GetNetworkID());
//	printf("TrackNetworkIDObject hashIndex=%i guid=%s\n",hashIndex, networkIdObject->GetNetworkID().guid.ToString()); // removeme
	if (networkIdHash[hashIndex]==0)
	{
		networkIdHash[hashIndex]=networkIdObject;
		return;
	}
	NetworkIDObject *nio=networkIdHash[hashIndex];
	// Duplicate insertion?
	RakAssert(nio!=networkIdObject);
	// Random GUID conflict?
	RakAssert(nio->GetNetworkID()!=networkIdObject->GetNetworkID());

	while (nio->nextInstanceForNetworkIDManager!=0)
	{		
		nio=nio->nextInstanceForNetworkIDManager;

		// Duplicate insertion?
		RakAssert(nio!=networkIdObject);
		// Random GUID conflict?
		RakAssert(nio->GetNetworkID()!=networkIdObject->GetNetworkID());
	}

	networkIdObject->nextInstanceForNetworkIDManager=0;
	nio->nextInstanceForNetworkIDManager=networkIdObject;
}
void NetworkIDManager::StopTrackingNetworkIDObject(NetworkIDObject *networkIdObject)
{
	if (networkIdObject->networkID==UNASSIGNED_NETWORK_ID)
		return;

	// RakAssert(networkIdObject->GetNetworkID()!=UNASSIGNED_NETWORK_ID);
	unsigned int hashIndex=NetworkIDToHashIndex(networkIdObject->GetNetworkID());
//	printf("hashIndex=%i\n",hashIndex); // removeme
	NetworkIDObject *nio=networkIdHash[hashIndex];
	if (nio==0)
	{
		// RakAssert("NetworkIDManager::StopTrackingNetworkIDObject didn't find object" && 0);
		return;
	}
	if (nio==networkIdObject)
	{
		networkIdHash[hashIndex]=nio->nextInstanceForNetworkIDManager;
		return;
	}

	while (nio)
	{
		if (nio->nextInstanceForNetworkIDManager==networkIdObject)
		{
			nio->nextInstanceForNetworkIDManager=networkIdObject->nextInstanceForNetworkIDManager;
			return;
		}
		nio=nio->nextInstanceForNetworkIDManager;
	}

	//RakAssert("NetworkIDManager::StopTrackingNetworkIDObject didn't find object" && 0);
}

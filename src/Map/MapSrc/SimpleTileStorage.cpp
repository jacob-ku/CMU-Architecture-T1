//---------------------------------------------------------------------------


#pragma hdrstop

#include "SimpleTileStorage.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
SimpleTileStorage::SimpleTileStorage() {
     DWORD id;

	m_QueueMutex = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

	if (m_QueueMutex == NULL) {
		throw SysException("CreateMutex() failed", GetLastError());
	}

	 ThreadKillEvent=CreateEvent (NULL,  // No security attributes
                                  FALSE, // Manual-reset event
                                  FALSE, // Initial state is signaled
                                  NULL); // Object name

	if (ThreadKillEvent == NULL) {
		CloseHandle(m_QueueMutex);
		throw SysException("CreateEvent() failed", GetLastError());
	}
    
	SemQueueCount=CreateSemaphore(NULL, 0, LONG_MAX, NULL);

	if (SemQueueCount == NULL) {
		CloseHandle(m_QueueMutex);
		CloseHandle(ThreadKillEvent);
		throw SysException("CreateSemaphore() failed", GetLastError());
	}
 
     m_Thread= CreateThread(NULL, 0, ThreadEntryPoint, (LPVOID)this, 0, &id);
	if (m_Thread== NULL) {
		CloseHandle(SemQueueCount);
		CloseHandle(m_QueueMutex);
		CloseHandle(ThreadKillEvent);
		throw SysException("CreateThread() failed", GetLastError());
	}

	m_pNextLoadStorage = 0;
	m_pSaveStorage = 0;
}

SimpleTileStorage::~SimpleTileStorage() {

	/* wait for loader thread to finish */
	printf("SimpleTileStorage: signaling thread to stop\n");
	SetEvent(ThreadKillEvent);

	/* wait for thread to finish with timeout */
	printf("SimpleTileStorage: wait for thread to finish\n");
	DWORD waitResult = WaitForSingleObject(m_Thread, 5000); // 5 second timeout
	if (waitResult == WAIT_TIMEOUT) {
		printf("SimpleTileStorage: thread termination timeout, forcing termination\n");
		// Force terminate thread if it doesn't respond
		TerminateThread(m_Thread, 0);
	} else if (waitResult == WAIT_OBJECT_0) {
		printf("SimpleTileStorage: thread finished normally\n");
	} else {
		printf("SimpleTileStorage: thread wait failed with error %d\n", GetLastError());
	}

	/* cleanup synchronisation objects */
	if (SemQueueCount != NULL) {
		CloseHandle(SemQueueCount);
		SemQueueCount = NULL;
	}
	if (m_QueueMutex != NULL) {
		CloseHandle(m_QueueMutex);
		m_QueueMutex = NULL;
	}
	if (ThreadKillEvent != NULL) {
		CloseHandle(ThreadKillEvent);
		ThreadKillEvent = NULL;
	}
	if (m_Thread != NULL) {
		CloseHandle(m_Thread);
		m_Thread = NULL;
	}
	printf("SimpleTileStorage: destructor done\n");
}

DWORD WINAPI SimpleTileStorage::ThreadEntryPoint(LPVOID pthis) {
	SimpleTileStorage* ts = (SimpleTileStorage*)pthis;

	/* start loader */
	ts->ThreadRun();

	return (0);
}

void SimpleTileStorage::Enqueue(TilePtr tile) {
	/* insert requested item in queue */
    WaitForSingleObject(m_QueueMutex,INFINITE); 
	m_Queue.push(tile);
	ReleaseMutex(m_QueueMutex);
	ReleaseSemaphore(SemQueueCount,1,NULL);

}

void SimpleTileStorage::ThreadRun() {
	 HANDLE         Handles[2];
     DWORD          Result;
    Handles[0]=SemQueueCount;
    Handles[1]=ThreadKillEvent;

	/* spin in this loop forever */
	while(1) {
		Result=WaitForMultipleObjects(2,Handles,false,INFINITE);   
        if ((Result==WAIT_OBJECT_0+1) || (Result!=WAIT_OBJECT_0)) {
			printf("SimpleTileStorage: thread received termination signal\n");
			break;
		}

		WaitForSingleObject(m_QueueMutex,INFINITE); 
		TilePtr current = m_Queue.front();
		m_Queue.pop();
		ReleaseMutex(m_QueueMutex);

		if (!current->IsOld()) {
			try {
				try {
					/* do actual processing - defined in derived class */
					Process(current);
				} catch (std::exception &e) {
					/* storage error worth to be reported to user, like:
					 * - unable to download from web due to `cannot connect' or `authentication failed'
					 * - unable to save
					 */
					warning("SimpleTileStorage: error (%s) (%d %d %d)\n", e.what(), current->GetX(), current->GetY(), current->GetLevel());

					/* XXX: this may need to be added to avoid loops
					 * (but there will be no loops in basic storage layout with 1 local and 1 remote storages)
					if (current->IsSaveable)
						throw;
					*/
				}

				/* pass it down the chain */
				if (current->IsLoaded() && current->IsSaveable() && m_pSaveStorage)
					m_pSaveStorage->Enqueue(current);
				else if (!current->IsLoaded() && m_pNextLoadStorage)
					m_pNextLoadStorage->Enqueue(current);
			} catch (std::exception &e) {
				/* fatal error in enqueue?! */
				warning("SimpleTileStorage: fatal error (%s) (%d %d %d)\n", e.what(), current->GetX(), current->GetY(), current->GetLevel());
			}
		}
	}
	printf("SimpleTileStorage: thread exiting\n");
}

void SimpleTileStorage::SetNextLoadStorage(TileStorage *ts) {
	m_pNextLoadStorage = ts;
}

void SimpleTileStorage::SetSaveStorage(TileStorage *ts) {
	m_pSaveStorage = ts;
}

void SimpleTileStorage::Detach() {
	m_pNextLoadStorage = 0;
	m_pSaveStorage = 0;
}


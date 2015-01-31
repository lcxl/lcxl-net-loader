#ifndef _LCXL_QUEUE_H_
#define _LCXL_QUEUE_H_
//LCXL
// 队列相关结构
//
// Types and macros to manipulate packet queue
//
typedef struct _QUEUE_ENTRY
{
	struct _QUEUE_ENTRY * Next;
}QUEUE_ENTRY, *PQUEUE_ENTRY;

typedef struct _QUEUE_HEADER
{
	PQUEUE_ENTRY     Head;
	PQUEUE_ENTRY     Tail;
} QUEUE_HEADER, PQUEUE_HEADER;

#define InitializeQueueHeader(_QueueHeader)             \
{                                                       \
	(_QueueHeader)->Head = (_QueueHeader)->Tail = NULL; \
}

//
// Macros for queue operations
//
#define IsQueueEmpty(_QueueHeader)      ((_QueueHeader)->Head == NULL)

#define RemoveHeadQueue(_QueueHeader)                   \
	(_QueueHeader)->Head;                               \
{                                                   \
	PQUEUE_ENTRY pNext;                             \
	ASSERT((_QueueHeader)->Head);                   \
	pNext = (_QueueHeader)->Head->Next;             \
	(_QueueHeader)->Head = pNext;                   \
	if (pNext == NULL)                              \
	(_QueueHeader)->Tail = NULL;                \
}

#define InsertHeadQueue(_QueueHeader, _QueueEntry)                  \
{                                                               \
	((PQUEUE_ENTRY)(_QueueEntry))->Next = (_QueueHeader)->Head; \
	(_QueueHeader)->Head = (PQUEUE_ENTRY)(_QueueEntry);         \
	if ((_QueueHeader)->Tail == NULL)                           \
	(_QueueHeader)->Tail = (PQUEUE_ENTRY)(_QueueEntry);     \
}

#define InsertTailQueue(_QueueHeader, _QueueEntry)                      \
{                                                                   \
	((PQUEUE_ENTRY)(_QueueEntry))->Next = NULL;                     \
	if ((_QueueHeader)->Tail)                                       \
	(_QueueHeader)->Tail->Next = (PQUEUE_ENTRY)(_QueueEntry);   \
		else                                                            \
		(_QueueHeader)->Head = (PQUEUE_ENTRY)(_QueueEntry);         \
		(_QueueHeader)->Tail = (PQUEUE_ENTRY)(_QueueEntry);             \
}

#endif
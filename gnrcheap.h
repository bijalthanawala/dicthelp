
/********************************************************************
*  Project: hffcmprss
*  Description: Compression program that uses huffman coding
*  Licence: The MIT license
*  Author:  See accompanied AUTHORS.txt
*
*  File: gnrcheap.h
*  Description: Generic heap header file
*  
*
********************************************************************/

#ifndef GENERIC_HEAP
#define GENERIC_HEAP


/* Includes
**************/
#include "common/common_types.h"

/* Type definitions
*********************/
typedef int32_t HEAP_TYPE;
typedef uint32_t HEAP_OFFSET;
typedef int (*PFN_HEAPELEMENT_CMP)(PVOID,PVOID);
typedef void (*PFN_HEAPELEMENT_DEL)(PVOID);

/* Constants / Definitions
****************************/
#define INVALID_HEAP_OFFSET ((HEAP_OFFSET)-1)
#define HEAP_TYPE_MIN		((HEAP_TYPE)-1)
#define HEAP_TYPE_MAX		((HEAP_TYPE)-2)

/* Structs / Unions
*********************/

typedef struct gnrcheap { 
		HEAP_TYPE   type;       /* HEAP_TYPE_MIN || HEAP_TYPE_MAX */   
		PVOID 	   *heaparr;	/*Array of PVOID - the real heap*/
		uint32_t	capacity;	/* Total capacity */
		uint32_t	occupancy;  /* Current occupancy */ 
		PFN_HEAPELEMENT_CMP pfnheapelecmp;
} GNRCHEAP, * PGNRCHEAP;

/* Prototypes
***************/
PGNRCHEAP gnrcheap_create(HEAP_TYPE heaptype,
						  uint32_t capacity, 
					      PFN_HEAPELEMENT_CMP pfnheapelecmp); 
VOID gnrcheap_destroy(PGNRCHEAP pheap,
	                  PFN_HEAPELEMENT_DEL pfnheapeledel); 
VOID gnrcheap_bottomup(PGNRCHEAP pheap);
PVOID gnrcheap_getmax(PGNRCHEAP pheap);
PVOID gnrcheap_getmin(PGNRCHEAP pheap);
void gnrcheap_delmin(PGNRCHEAP pheap,PFN_HEAPELEMENT_DEL pfnheapeledel);
void gnrcheap_delmax(PGNRCHEAP pheap,PFN_HEAPELEMENT_DEL pfnheapeledel);
BOOL  gnrcheap_insert(PGNRCHEAP pheap,PVOID pnewele);

#endif

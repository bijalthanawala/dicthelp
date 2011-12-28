
/********************************************************************
*  Licence: The MIT license
*  Author:  See accompanied AUTHORS.txt
*
*  File: gnrcheap.c
*  Description: Generic heap implementation 
*  
*
********************************************************************/


/* Includes
***************/
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include "common/common_types.h"
#include "gnrcheap.h"

/* Prototypes
***************/
HEAP_OFFSET gnrcheap_minofthree(PGNRCHEAP pheap,
    	  					    HEAP_OFFSET parent);
HEAP_OFFSET gnrcheap_maxofthree(PGNRCHEAP pheap,
    	  					    HEAP_OFFSET parent);
HEAP_OFFSET gnrcheap_minofparentchild(PGNRCHEAP pheap,
									  HEAP_OFFSET child);
HEAP_OFFSET gnrcheap_maxofparentchild(PGNRCHEAP pheap,
									  HEAP_OFFSET child);
VOID gnrcheap_swap(PVOID *heaparr, HEAP_OFFSET offset_a, HEAP_OFFSET offset_b);
VOID gnrcheap_siftup(PGNRCHEAP pheap);
VOID gnrcheap_siftdown(PGNRCHEAP pheap,
					   HEAP_OFFSET downfrom);
PVOID gnrcheap_getroot(PGNRCHEAP pheap);
void gnrcheap_delroot(PGNRCHEAP pheap,PFN_HEAPELEMENT_DEL pfnheapeledel);
void gnrcheap_del(PGNRCHEAP pheap,
				   PFN_HEAPELEMENT_DEL pfnheapeledel,
 		           HEAP_OFFSET deloffset);
/* Routines
**************/

PGNRCHEAP gnrcheap_create(HEAP_TYPE heaptype,
						  uint32_t capacity, 
					      PFN_HEAPELEMENT_CMP pfnheapelecmp) 
{
	PGNRCHEAP pheap = NULL;

	//Validate requested heap type
	if(!(heaptype==HEAP_TYPE_MIN || heaptype==HEAP_TYPE_MAX)) {
		return NULL;
	}

	//Allocate for metadata
	pheap = malloc(sizeof(GNRCHEAP)); 
	if(!pheap) {
		return NULL;
	}
	memset(pheap,0,sizeof(*pheap));

	capacity++;	          //NOTE: Heap operates from offset 1, hence +1	
	pheap->heaparr = malloc(sizeof(PVOID) * capacity);
	if(!pheap->heaparr) {
		free(pheap);
		return NULL;
	}
	memset(pheap->heaparr,0,sizeof(PVOID) * capacity);

	pheap->type		     = heaptype;
	pheap->capacity      = capacity;
	pheap->occupancy     = 0;
	pheap->pfnheapelecmp = pfnheapelecmp;

	return pheap;
}

VOID gnrcheap_destroy(PGNRCHEAP pheap, 
					  PFN_HEAPELEMENT_DEL pfnheapeledel)
{
	int i=INVALID_HEAP_OFFSET;

	//NOTES: 
	// => Heap operates from offset 1
	// => 'deep delete' need be done only for 'occupied' elements 
	for(i=1;i<pheap->occupancy;i++)
    {
		(*pfnheapeledel)(pheap->heaparr[i]);
	}

	free(pheap->heaparr);
	free(pheap);
}

PVOID gnrcheap_getmin(PGNRCHEAP pheap)
{
	if(pheap->type == HEAP_TYPE_MIN) {
		return gnrcheap_getroot(pheap);
	}

	return NULL;
}


PVOID gnrcheap_getmax(PGNRCHEAP pheap)
{
	if(pheap->type == HEAP_TYPE_MAX) {
		return gnrcheap_getroot(pheap);
	}

	return NULL;
}

PVOID gnrcheap_getroot(PGNRCHEAP pheap)
{
	if(pheap->occupancy > 0) {
		return pheap->heaparr[1];
    }

	return NULL; 
}

void gnrcheap_delmin(PGNRCHEAP pheap,PFN_HEAPELEMENT_DEL pfnheapeledel)
{
	if(pheap->type == HEAP_TYPE_MIN) {
		gnrcheap_delroot(pheap,pfnheapeledel);
	}

}

void gnrcheap_delmax(PGNRCHEAP pheap,PFN_HEAPELEMENT_DEL pfnheapeledel)
{
	if(pheap->type == HEAP_TYPE_MAX) {
		gnrcheap_delroot(pheap,pfnheapeledel);
	}
}

void gnrcheap_delroot(PGNRCHEAP pheap,PFN_HEAPELEMENT_DEL pfnheapeledel)
{
	gnrcheap_del(pheap,pfnheapeledel,1);
}

void gnrcheap_del(PGNRCHEAP pheap,
				   PFN_HEAPELEMENT_DEL pfnheapeledel,
				   HEAP_OFFSET deloffset)
{
	PVOID *heaparr = pheap->heaparr;

	if(pheap->occupancy < deloffset) {
		return;
	}


	/* If caller has requested 'deep' delete, perform so...*/
	if(pfnheapeledel) {
		(*pfnheapeledel)(heaparr[deloffset]);
	}

	heaparr[deloffset] = heaparr[pheap->occupancy];
	pheap->occupancy--;
	gnrcheap_siftdown(pheap,deloffset);
}


VOID gnrcheap_bottomup(PGNRCHEAP pheap)
{
	HEAP_OFFSET parent = pheap->occupancy / 2;
	HEAP_OFFSET minmaxat = INVALID_HEAP_OFFSET;

	while(parent) 
	{
		if(pheap->type == HEAP_TYPE_MIN) {
			minmaxat = gnrcheap_minofthree(pheap,parent);
		} else {
			minmaxat = gnrcheap_maxofthree(pheap,parent);
		}

		if(minmaxat != parent) {
			 gnrcheap_swap(pheap->heaparr,parent,minmaxat);
			 gnrcheap_siftdown(pheap,minmaxat);
		}
		parent--;
	}
}


BOOL  gnrcheap_insert(PGNRCHEAP pheap,PVOID pnewele) {
	
	if((pheap->occupancy+1) > pheap->capacity) {
		return FALSE;
	}

	pheap->occupancy++;
	pheap->heaparr[pheap->occupancy] = pnewele;
	
	gnrcheap_siftup(pheap);

    return TRUE;	
}

HEAP_OFFSET gnrcheap_minofthree(PGNRCHEAP pheap,
    	  					    HEAP_OFFSET parent) 
{
	HEAP_OFFSET left = parent*2;
	HEAP_OFFSET right = (parent*2)+1;
	PVOID *heaparr = pheap->heaparr;

	/*note(s): 
			=> the way the logic is laid makes order of the 
			   conditions important 
			=> this routine is parent-centric,
			   in other words, if certain comparison(s) lead to equality,
			   parent's offset is return to avoid causing
			   unnecessary swap by the callers of this routine
	*/

    if(left > pheap->occupancy) {
	   /*If parent has no left node, in other words,
	     If parent has no child, in other words,
		 If *parent* is actually a leaf node, then
		 let be known that the parent is the MIN */

	   return (parent);	 
	}

	if(right > pheap->occupancy) {
		/* If we reach here means, parent has only one child node (left) */

		if((*pheap->pfnheapelecmp)(heaparr[parent],heaparr[left]) <= 0) {
		   /*Parent is smaller than (or equal to) the left child */
			return (parent); 
		}
		else {
			return (left);
		}
	}


    /* If we reach here means parent has two child nodes */

	/* So, do three-way comparison */

	if((*pheap->pfnheapelecmp)(heaparr[parent],heaparr[left]) <= 0) {
		/*Parent is smaller than (or equal to) the left child */
		if((*pheap->pfnheapelecmp)(heaparr[parent],heaparr[right]) <= 0) {
		  /*Parent is also smaller than (or equal to) the right child */
		  return (parent);
		} else {
			/* But right child is smaller than parent and the left child */
			return (right);
		}
	} else {

		/* Left child is smaller than the parent */
		if((*pheap->pfnheapelecmp)(heaparr[left],heaparr[right]) <= 0) {
			/* Left child is also smaller than (or eq to) the right child*/
			return (left);
		} else {
			/* But, right child is smaller than the left child */
			return (right);
		}
	}

	/* A good input should never make it here.
	   Something terribly wrong if we reach here. */
	printf("An unexplicable supernatural phenomenon just struck to function: %s\n",__FUNCTION__);
    assert(TRUE);
}

HEAP_OFFSET gnrcheap_maxofthree(PGNRCHEAP pheap,
    	  					    HEAP_OFFSET parent) 
{
	HEAP_OFFSET left = parent*2;
	HEAP_OFFSET right = (parent*2)+1;
	PVOID *heaparr = pheap->heaparr;


	/*NOTE(S): 
			=> The way the logic is laid makes order of the 
			   conditions important 
			=> This routine is parent-centric,
			   in other words, if certain comparison(s) lead to equality,
			   parent's offset is return to avoid causing
			   unnecessary swap by the callers of this routine
	*/


    if(left > pheap->occupancy) {
	   /*If parent has no left node, in other words,
	     If parent has no child, in other words,
		 If *parent* is actually a leaf node, then
		 let be known that the parent is the MAX */

	   return (parent);	 
	}

	if(right > pheap->occupancy) {
		/* If we reach here means, parent has only one child node (left) */

		if((*pheap->pfnheapelecmp)(heaparr[parent],heaparr[left]) >= 0) {
		   /*Parent is larger than (or equal to) the left child */
			return (parent); 
		}
		else {
			return (left);
		}
	}


    /* If we reach here means parent has two child nodes */

	/* So, do three-way comparison */

	if((*pheap->pfnheapelecmp)(heaparr[parent],heaparr[left]) >= 0) {
		/*Parent is larger than (or equal to) the left child */
		if((*pheap->pfnheapelecmp)(heaparr[parent],heaparr[right]) >= 0) {
			/*Parent is also larger than (or equal to) the right child */
			return (parent);
		} else {
			/* But right child is larger than parent and the left child */
			return (right);
		}
	} else {

		/* Left child is larger than the parent */
		if((*pheap->pfnheapelecmp)(heaparr[left],heaparr[right]) >= 0) {
			/* Left child is also larger than (or eq to) the right child*/
			return (left);
		} else {
			/* But, right child is larger than the left child */
			return (right);
		}
	}

	/* A good input should never make it here.
	   Something terribly wrong if we reach here. */
	printf("An unexplicable supernatural phenomenon just struck to function: %s\n",__FUNCTION__);
    assert(TRUE);
}


VOID gnrcheap_swap(PVOID *heaparr, HEAP_OFFSET offset_a, HEAP_OFFSET offset_b)
{
	/* A very light-weight routine.
	   Assumes all input's are good and hence performs no sanity checks on
	   parameters */

	PVOID tmp = heaparr[offset_a];

	heaparr[offset_a] = heaparr[offset_b];
	heaparr[offset_b] = tmp;

}


VOID gnrcheap_siftdown(PGNRCHEAP pheap,
		HEAP_OFFSET downfrom)
{
	HEAP_OFFSET minmaxat = INVALID_HEAP_OFFSET;

	if(pheap->type == HEAP_TYPE_MIN) {
		minmaxat = gnrcheap_minofthree(pheap,downfrom);
	} else {
		minmaxat = gnrcheap_maxofthree(pheap,downfrom);
	}

	while(minmaxat != downfrom) {

		gnrcheap_swap(pheap->heaparr,downfrom,minmaxat);
		downfrom = minmaxat;

		if(pheap->type == HEAP_TYPE_MIN) {
			minmaxat = gnrcheap_minofthree(pheap,downfrom);
		} else {
			minmaxat = gnrcheap_maxofthree(pheap,downfrom);
		}

	}
}

VOID gnrcheap_siftup(PGNRCHEAP pheap) 
{
	HEAP_OFFSET this_off = INVALID_HEAP_OFFSET; 		
	HEAP_OFFSET minmaxat = INVALID_HEAP_OFFSET;

	this_off = pheap->occupancy;


	/* NOTE(s):
				=> Yes, for MIN-HEAP we look for MAX element - and 
				   vice-versa.
				   This is not a typo. For this implementation of siftup 
				   operation this is the correct logic.  Think about it.
	*/
	if(pheap->type == HEAP_TYPE_MIN) { 
		minmaxat = gnrcheap_maxofparentchild(pheap,this_off);
	} else {
		minmaxat = gnrcheap_minofparentchild(pheap,this_off);
	}

	while(this_off != minmaxat) {
		gnrcheap_swap(pheap->heaparr,this_off,minmaxat);
		this_off = minmaxat;

		if(pheap->type == HEAP_TYPE_MIN) { 
			minmaxat = gnrcheap_maxofparentchild(pheap,this_off);
		} else {
			minmaxat = gnrcheap_minofparentchild(pheap,this_off);
		}
	}

}


HEAP_OFFSET gnrcheap_minofparentchild(PGNRCHEAP pheap,
									  HEAP_OFFSET child)
{
	HEAP_OFFSET parent = INVALID_HEAP_OFFSET;
	PVOID *heaparr = NULL;

	/*Note(s): 
			=> this routine is child-centric,
			   in other words, if certain comparison(s) lead to equality,
			   child's offset is return to avoid causing
			   unnecessary swap by the callers of this routine
	*/

	if(child == 1) { 
		return child; /* Root node has no parent to compare against */
	}

	parent = child/2;
	heaparr = pheap->heaparr;

    if(pheap->pfnheapelecmp(heaparr[parent],heaparr[child]) >= 0)
		return child;
		

	return parent;
}

HEAP_OFFSET gnrcheap_maxofparentchild(PGNRCHEAP pheap,
									  HEAP_OFFSET child)
{
	HEAP_OFFSET parent = INVALID_HEAP_OFFSET;
	PVOID *heaparr = NULL;
	
	/*Note(s): 
			=> this routine is child-centric,
			   in other words, if certain comparison(s) lead to equality,
			   child's offset is return to avoid causing
			   unnecessary swap by the callers of this routine
	*/

	if(child == 1) { 
		return child; /* Root node has no parent to compare against */
	}

	parent = child/2;
	heaparr = pheap->heaparr;

    if(pheap->pfnheapelecmp(heaparr[parent],heaparr[child]) <= 0)
		return child;
		

	return parent;
}


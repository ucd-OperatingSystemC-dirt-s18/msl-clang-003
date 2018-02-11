/*
 * Created by Ivo Georgiev on 2/9/16.
 *
 *
 * C Programming Assignment 3
 * Christina Tsui
 * Dalton Burke
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h> // for perror()

#include "mem_pool.h"

/*************/
/*           */
/* Constants */
/*           */
/*************/
static const float      MEM_FILL_FACTOR                 = 0.75;
static const unsigned   MEM_EXPAND_FACTOR               = 2;

static const unsigned   MEM_POOL_STORE_INIT_CAPACITY    = 20;
static const float      MEM_POOL_STORE_FILL_FACTOR      = 0.75;
static const unsigned   MEM_POOL_STORE_EXPAND_FACTOR    = 2;

static const unsigned   MEM_NODE_HEAP_INIT_CAPACITY     = 40;
static const float      MEM_NODE_HEAP_FILL_FACTOR       = 0.75;
static const unsigned   MEM_NODE_HEAP_EXPAND_FACTOR     = 2;

static const unsigned   MEM_GAP_IX_INIT_CAPACITY        = 40;
static const float      MEM_GAP_IX_FILL_FACTOR          = 0.75;
static const unsigned   MEM_GAP_IX_EXPAND_FACTOR        = 2;



/*********************/
/*                   */
/* Type declarations */
/*                   */
/*********************/
typedef struct _alloc {
    char *mem;
    size_t size;
} alloc_t, *alloc_pt;

typedef struct _node {
    alloc_t alloc_record;
    unsigned used;
    unsigned allocated;
    struct _node *next, *prev; // doubly-linked list for gap deletion
} node_t, *node_pt;

typedef struct _gap {
    size_t size;
    node_pt node;
} gap_t, *gap_pt;

typedef struct _pool_mgr {
    pool_t pool;
    node_pt node_heap;
    unsigned total_nodes;
    unsigned used_nodes;
    gap_pt gap_ix;
    unsigned gap_ix_capacity;
} pool_mgr_t, *pool_mgr_pt;



/***************************/
/*                         */
/* Static global variables */
/*                         */
/***************************/
static pool_mgr_pt *pool_store = NULL; // an array of pointers, only expand
static unsigned pool_store_size = 0;
static unsigned pool_store_capacity = 0;



/********************************************/
/*                                          */
/* Forward declarations of static functions */
/*                                          */
/********************************************/
static alloc_status _mem_resize_pool_store();
static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr);
static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr);
static alloc_status
_mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                   size_t size,
                   node_pt node);
static alloc_status
_mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                        size_t size,
                        node_pt node);
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr);
static alloc_status _mem_invalidate_gap_ix(pool_mgr_pt pool_mgr);



/****************************************/
/*                                      */
/* Definitions of user-facing functions */
/*                                      */
/****************************************/
alloc_status mem_init() {
    // ensure that it's called only once until mem_free
    // allocate the pool store with initial capacity
    // note: holds pointers only, other functions to allocate/deallocate
    //----------------------------------------------------------------------

    if(pool_store == NULL){//Call once - Allocate the pool store w/ initial cap

        pool_store = calloc(MEM_POOL_STORE_INIT_CAPACITY, sizeof(pool_mgr_pt));

        pool_store_size = 0;
        pool_store_capacity = MEM_POOL_STORE_INIT_CAPACITY;
        return ALLOC_OK;

    }
    else {//Ensured only called once

        return ALLOC_CALLED_AGAIN;

    }

    //return ALLOC_FAIL; // From before edits made

}//DONE

alloc_status mem_free() {
    // ensure that it's called only once for each mem_init
    // make sure all pool managers have been deallocated
    // can free the pool store array
    // update static variables
    //----------------------------------------------------------------------

    if(pool_store == NULL){//Ensure once per mem_init

        return ALLOC_CALLED_AGAIN;

    }
    else{

        for (int i = 0; i < pool_store_size; ++i) {

            if (pool_store[i] != NULL){//Make sure all pool managers have been deallocated

                return ALLOC_NOT_FREED;

            }
        }

        free(pool_store);//Free pool store array

        pool_store = NULL;//Update variables
        pool_store_size = 0;
        pool_store_capacity = 0;
        return ALLOC_OK;

    }

    //return ALLOC_FAIL; // From before edits made
}//DONE

pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    // make sure there the pool store is allocated
    // expand the pool store, if necessary
    // allocate a new mem pool mgr
    // check success, on error return null
    // allocate a new memory pool
    // check success, on error deallocate mgr and return null
    // allocate a new node heap
    // check success, on error deallocate mgr/pool and return null
    // allocate a new gap index
    // check success, on error deallocate mgr/pool/heap and return null
    // assign all the pointers and update meta data:
    //   initialize top node of node heap
    //   initialize top node of gap index
    //   initialize pool mgr
    //   link pool mgr to pool store
    // return the address of the mgr, cast to (pool_pt)
    //----------------------------------------------------------------------

    if(pool_store == NULL){// make sure there the pool store is deallocated

        return NULL;

    }

    pool_mgr_pt poolMgr = malloc(sizeof(pool_mgr_t));//New mem_pool_mgr
    if(poolMgr == NULL) {//Check success

        return NULL;

    }

    char* poolMem = malloc(size);//Allocate new mem_pool
    if (poolMem== NULL) {//Check success

        free(poolMgr);//Free if error and return Null
        return NULL;

    }

    node_pt nodeHeap = calloc(MEM_NODE_HEAP_INIT_CAPACITY, sizeof(node_t));//New node heap

    if (nodeHeap == NULL) {//Check success

        free(poolMem);//deallocate if error, return Null
        free(poolMgr);
        return NULL;

    }

    gap_pt gapIx = calloc(MEM_GAP_IX_INIT_CAPACITY, sizeof(gap_t));//New gap index

    if(gapIx == NULL) {//Check success

        free(nodeHeap);//deallocate if error, return Null
        free(poolMem);
        free(poolMgr);
        return NULL;

    }

    poolMgr->pool.mem = poolMem;// assign all the pointers and update meta data:
    poolMgr->pool.alloc_size = 0;
    poolMgr->pool.num_allocs = 0;
    poolMgr->pool.num_gaps = 0;
    poolMgr->pool.policy = policy;
    poolMgr->pool.total_size = size;

    poolMgr->node_heap = nodeHeap;
    poolMgr->total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;
    poolMgr->used_nodes = 1;

    poolMgr->gap_ix = gapIx;
    poolMgr->gap_ix_capacity = MEM_GAP_IX_INIT_CAPACITY;

    nodeHeap[0].used = 1;//initialize top node of node heap
    nodeHeap[0].allocated = 0;
    nodeHeap[0].alloc_record.mem = poolMem;
    nodeHeap[0].alloc_record.size = size;
    nodeHeap[0].next = NULL;
    nodeHeap[0].prev = NULL;

    _mem_add_to_gap_ix(poolMgr, size, &nodeHeap[0]);//initialize top node of gap index

    pool_store[pool_store_size] = poolMgr;//initialize pool mgr
    pool_store_size++;//link pool mgr to pool store


    return (pool_pt) poolMgr;// return the address of the mgr, cast to (pool_pt)

    //return ALLOC_FAIL; // From before edits made

}//DONE

alloc_status mem_pool_close(pool_pt pool) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    // check if this pool is allocated
    // check if pool has only one gap
    // check if it has zero allocations
    // free memory pool
    // free node heap
    // free gap index
    // find mgr in pool store and set to null
    // note: don't decrement pool_store_size, because it only grows
    // free mgr
    //----------------------------------------------------------------------

    pool_mgr_pt poolMgr = (pool_mgr_pt) pool;// get mgr from pool by casting the pointer to (pool_mgr_pt)


    if (poolMgr == NULL) {// check if this pool is allocated

        return ALLOC_NOT_FREED;

    }

    if (poolMgr->pool.num_gaps > 1) {// check if pool has only one gap

        return ALLOC_NOT_FREED;

    }

    if (poolMgr->pool.num_allocs != 0) {// check if it has zero allocations

        return ALLOC_NOT_FREED;

    }

    free(poolMgr->pool.mem);// free memory pool
    free(poolMgr->node_heap);// free node heap
    free(poolMgr->gap_ix);// free gap index

    for (int i = 0; i < pool_store_size; i++) {// find mgr in pool store and set to null

        if(pool_store[i] == poolMgr) {

            pool_store[i] = NULL;
            break;

        }

    }

    free(poolMgr);// free mgr

    return ALLOC_OK;
    //return ALLOC_FAIL; // From before edits made
}//Done

void * mem_new_alloc(pool_pt pool, size_t size) {
    /* get mgr from pool by casting the pointer to (pool_mgr_pt)
    // check if any gaps, return null if none
    // expand heap node, if necessary, quit on error //TODO
    // check used nodes fewer than total nodes, quit on error
    // get a node for allocation:
    // if FIRST_FIT, then find the first sufficient node in the node heap
    // if BEST_FIT, then find the first sufficient node in the gap index
    // check if node found   // update metadata (num_allocs, alloc_size)
    // calculate the size of the remaining gap, if any
    // remove node from gap index    // convert gap_node to an allocation node of given size
    // adjust node heap:
    //   if remaining gap, need a new node//   find an unused one in the node heap
    //   make sure one was found //   initialize it to a gap node
    //   update metadata (used_nodes)
    //   update linked list (new node right after the node for allocation)
    //   add to gap index//   check if successful
    // return allocation record by casting the node to (alloc_pt)*/
    //----------------------------------------------------------------------

    pool_mgr_pt poolMgr = (pool_mgr_pt) pool;// get mgr from pool by casting the pointer to (pool_mgr_pt)

    if (poolMgr->gap_ix_capacity == 0){// check if any gaps, return null if none

        return NULL;

    }

    if (poolMgr->total_nodes <= poolMgr->used_nodes){// check used nodes fewer than total nodes, quit on error

        return NULL;

    }

    node_pt nodeForAlloc = NULL;// get a node for allocation:


    if (poolMgr->pool.policy==FIRST_FIT){// if FIRST_FIT,

        nodeForAlloc = poolMgr->node_heap;

        while (nodeForAlloc->allocated == 1 || nodeForAlloc->alloc_record.size < size){

            if(nodeForAlloc->next == NULL){// then find the first sufficient node in the node heap

                return NULL;

            }

            nodeForAlloc = nodeForAlloc->next;
        }
    }

    else {// if BEST_FIT

        int check = 1;
        int nodeIndex = 0;

        while (check && nodeIndex < poolMgr->pool.num_gaps){

            if (poolMgr->gap_ix[nodeIndex].size >= size){//then find the first sufficient node in the gap index

                check = 0;
                nodeForAlloc = poolMgr->gap_ix[nodeIndex].node;

            }
            else{

                nodeIndex =  nodeIndex +1;
            }
        }

        if(nodeIndex == poolMgr->pool.num_gaps) {//check if node found

            return NULL;
        }
    }

    poolMgr->pool.num_allocs++;//Update metadata num_allocs
    poolMgr->pool.alloc_size = poolMgr->pool.alloc_size + size;//update alloc_size

    size_t remainingSize = nodeForAlloc->alloc_record.size - size;//calculate the size of the remaining gap, if any

    _mem_remove_from_gap_ix(poolMgr, nodeForAlloc->alloc_record.size, nodeForAlloc);//remove node from gap index

    nodeForAlloc->alloc_record.size = size;//convert gap_node to an allocation node of given size
    nodeForAlloc->allocated = 1;

    if (remainingSize > 0){// adjust node heap:

        node_pt gapNode = NULL;//   if remaining gap, need a new node

        for (int i = 0; i < poolMgr->total_nodes; ++i) {

            if (poolMgr->node_heap[i].used == 0){//   find an unused one in the node heap

                gapNode = &poolMgr->node_heap[i];
                break;
            }
        }

        gapNode->used = 1;//initialize it to a gap node//TODO may be null?
        gapNode->alloc_record.size = remainingSize;
        gapNode->alloc_record.mem = nodeForAlloc->alloc_record.mem + size;
        gapNode->allocated = 0;

        poolMgr->used_nodes++;//   update metadata (used_nodes)

        gapNode->next = nodeForAlloc->next;//update linked list (new node right after the node for allocation)
        gapNode->prev = nodeForAlloc;

        if (nodeForAlloc->next != NULL){

            nodeForAlloc->next->prev = gapNode;

        }

        nodeForAlloc->next = gapNode;

        if(_mem_add_to_gap_ix(poolMgr,gapNode->alloc_record.size,gapNode) != ALLOC_OK){//add to gap index

            return NULL;
        }
    }

    return (alloc_pt)nodeForAlloc;// return allocation record by casting the node to (alloc_pt)
}//99%Done

alloc_status mem_del_alloc(pool_pt pool, void * alloc) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    // get node from alloc by casting the pointer to (node_pt)
    // find the node in the node heap
    // this is node-to-delete
    // make sure it's found
    // convert to gap node
    // update metadata (num_allocs, alloc_size)
    // if the next node in the list is also a gap, merge into node-to-delete
    //   remove the next node from gap index
    //   check success
    //   add the size to the node-to-delete
    //   update node as unused
    //   update metadata (used nodes)
    //   update linked list:
    // this merged node-to-delete might need to be added to the gap index
    // but one more thing to check...
    // if the previous node in the list is also a gap, merge into previous!
    //   remove the previous node from gap index
    //   check success
    //   add the size of node-to-delete to the previous
    //   update node-to-delete as unused
    //   update metadata (used_nodes)
    //   update linked list
    //   change the node to add to the previous node!
    // add the resulting node to the gap index
    // check success
    //----------------------------------------------------------------------

    pool_mgr_pt poolMgr = (pool_mgr_pt) pool;// get mgr from pool by casting the pointer to (pool_mgr_pt)
    node_pt nodePt = (node_pt)alloc;// get node from alloc by casting the pointer to (node_pt)

    if(nodePt == NULL){//Make sure to find node in node heap

        return ALLOC_FAIL;
    }

    nodePt->allocated = 0;// convert to gap node
    poolMgr->pool.num_allocs--;// update metadata (num_allocs, alloc_size)
    poolMgr->pool.alloc_size = poolMgr->pool.alloc_size - nodePt->alloc_record.size;

    if (nodePt->next !=NULL && nodePt->next->allocated == 0 && nodePt->next->used){//If next on list==gap, merge

        node_pt next = nodePt->next;//remove the next node from gap index

        if(_mem_remove_from_gap_ix(poolMgr,next->alloc_record.size, next) != ALLOC_OK){//Check success

            return ALLOC_FAIL;
        }

        nodePt->alloc_record.size = nodePt->alloc_record.size + next->alloc_record.size;//add the size to the node-to-delete
        next->used = 0;//update node as unused
        poolMgr->used_nodes--;//update metadata (used nodes)


        if (next->next) {//update linked list with given code
            next->next->prev = nodePt;
            nodePt->next = next->next;
        } else {
            nodePt->next = NULL;
        }
        next->next = NULL;
        next->prev = NULL;

    }

    if(nodePt->prev != NULL && nodePt->prev->allocated == 0 && nodePt->prev->used){//this merged node-to-delete might need to be added to the gap index

        node_pt prev = nodePt->prev;//remove the previous node from gap index

        if(_mem_remove_from_gap_ix(poolMgr,prev->alloc_record.size, prev) != ALLOC_OK){//check success

            return ALLOC_FAIL;
        }

        prev->alloc_record.size = prev->alloc_record.size + nodePt->alloc_record.size;//add the size of node-to-delete to the previous
        nodePt->used = 0;//   update node-to-delete as unused
        poolMgr->used_nodes--;//   update metadata (used_nodes)

        if (nodePt->next) {//   update linked list with given code
            prev->next = nodePt->next;
            nodePt->next->prev = prev;
        } else {
            prev->next = NULL;
        }
        nodePt->next = NULL;
        nodePt->prev = NULL;

        nodePt = prev;// change the node to add to the previous node!
    }

    if(_mem_add_to_gap_ix(poolMgr, nodePt->alloc_record.size, nodePt) != ALLOC_OK){// add the resulting node to the gap index

        return ALLOC_FAIL;
    }

    return ALLOC_OK;
    //return ALLOC_FAIL; // From before edits made
}//Done

void mem_inspect_pool(pool_pt pool,
                      pool_segment_pt *segments,
                      unsigned *num_segments) {
    // get the mgr from the pool
    // allocate the segments array with size == used_nodes
    // check successful
    // loop through the node heap and the segments array
    //    for each node, write the size and allocated in the segment
    // "return" the values:
    //----------------------------------------------------------------------

    pool_mgr_pt poolMgr = (pool_mgr_pt) pool;// get the mgr from the pool

    pool_segment_pt segmentArray = calloc(poolMgr->used_nodes,
                                          sizeof(pool_segment_t));// allocate the segments array with size == used_nodes

    if (segmentArray == NULL){// check successful

        segments = NULL;//TODO never used?
        return;
    }

    node_pt currentNode = poolMgr->node_heap;//loop through the node heap and the segments array
    for (int i = 0; i < poolMgr->used_nodes; i++){//for each node, write the size and allocated in the segment

        segmentArray[i].size = currentNode->alloc_record.size;
        segmentArray[i].allocated = currentNode->allocated;
        currentNode = currentNode->next;

    }

    *segments = segmentArray;// "return" the values:
    *num_segments = poolMgr->used_nodes;
}//99%Done

/***********************************/
/*                                 */
/* Definitions of static functions */
/*                                 */
/***********************************/
static alloc_status _mem_resize_pool_store() {
    // check if necessary
    /*
                if (((float) pool_store_size / pool_store_capacity)
                    > MEM_POOL_STORE_FILL_FACTOR) {...}
     */
    // don't forget to update capacity variables

    return ALLOC_FAIL;
}//TODO

static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) {
    // see above

    return ALLOC_FAIL;
}//TODO

static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) {
    // see above

    return ALLOC_FAIL;
}//TODO

static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                                       size_t size,
                                       node_pt node) {
    // expand the gap index, if necessary (call the function)
    // add the entry at the end
    // update metadata (num_gaps)
    // sort the gap index (call the function)
    // check success
    //-------------------------------------------------------------------

    //expand the gap
    pool_mgr->gap_ix[pool_mgr->pool.num_gaps].size = size;
    pool_mgr->gap_ix[pool_mgr->pool.num_gaps].node = node;

    // add the entry at the end
    // check success//TODO

    pool_mgr->pool.num_gaps ++;// update metadata (num_gaps)

    if(_mem_sort_gap_ix(pool_mgr) != ALLOC_OK) {// sort the gap index (call the function)
        return ALLOC_FAIL;
    }

    return ALLOC_OK;//return result;
}//TODO

static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                                            size_t size,
                                            node_pt node) {
    // find the position of the node in the gap index
    // loop from there to the end of the array:
    //    pull the entries (i.e. copy over) one position up
    //    this effectively deletes the chosen node
    // update metadata (num_gaps)
    // zero out the element at position num_gaps!
    //--------------------------------------------------------------

    unsigned found = 0;
    int gapNodeIndex = 0;

    for (gapNodeIndex = 0; gapNodeIndex < pool_mgr->pool.num_gaps;
         ++gapNodeIndex) {// find the position of the node in the gap index

        if (pool_mgr->gap_ix[gapNodeIndex].node == node) {

            found = 1;
            break;
        }
    }

    if(!found) {

        return ALLOC_FAIL;
    }

    while(gapNodeIndex < pool_mgr->pool.num_gaps) {// loop from there to the end of the array:

        pool_mgr->gap_ix[gapNodeIndex].size = pool_mgr->gap_ix[gapNodeIndex+1].size;//pull the entries (i.e. copy over) one position up
        pool_mgr->gap_ix[gapNodeIndex].node = pool_mgr->gap_ix[gapNodeIndex+1].node;//this effectively deletes the chosen node
        gapNodeIndex++;

    }

    pool_mgr->pool.num_gaps--;// update metadata (num_gaps)
    pool_mgr->gap_ix[pool_mgr->pool.num_gaps].size = 0;// zero out the element at position num_gaps!
    pool_mgr->gap_ix[pool_mgr->pool.num_gaps].node = NULL;

    return ALLOC_OK;
}//TODO Size not used, not sure where to use.

// note: only called by _mem_add_to_gap_ix, which appends a single entry
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr) {
    // the new entry is at the end, so "bubble it up"
    // loop from num_gaps - 1 until but not including 0:
    //    if the size of the current entry is less than the previous (u - 1)
    //    or if the sizes are the same but the current entry points to a
    //    node with a lower address of pool allocation address (mem)
    //       swap them (by copying) (remember to use a temporary variable)
    //---------------------------------------------------------------------

    if(pool_mgr->pool.num_gaps == 0){//Check single entry call
        return ALLOC_FAIL;
    }

    for(int i = pool_mgr->pool.num_gaps - 1; i > 0; i--){// loop from num_gaps - 1 until but not including 0:

        gap_pt current = &pool_mgr->gap_ix[i];//    if the size of the current entry is less than the previous (u - 1)
        gap_pt previous = &pool_mgr->gap_ix[i-1];//    or if the sizes are the same but the current entry points to a
                                                 //    node with a lower address of pool allocation address (mem)

        if//Checking size and mem locations
                (current->size < previous->size ||
                 (current->size == previous->size &&
                  current->node->alloc_record.mem < previous->node->alloc_record.mem))
        {

            gap_t temp;//Temp variable

            temp.node = current->node;//   swap them (by copying) (remember to use a temporary variable)
            temp.size = current->size;

            current->node = previous->node;
            current->size = previous->size;
            previous->node = temp.node;
            previous->size = temp.size;

        }
    }

    return ALLOC_OK;
    //return ALLOC_FAIL; From initial function
}

static alloc_status _mem_invalidate_gap_ix(pool_mgr_pt pool_mgr) {
    return ALLOC_FAIL;
}//TODO

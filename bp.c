/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#define INSTRUCTION_SIZE  32

enum state {SNT, WNT, WT, ST};

struct branch_target_buffer{
    void* tag;
    void* target;
    void* history;
    void* tableState;
    void* globalTableState;

    unsigned int btbSize;       //num of lines in btb
    unsigned int historySize;   //num history bit
    unsigned int tagSize;       //num of tag bit
    enum state beginState;
    bool isGlobalHist;
    bool isGlobalTableState;
    bool usingLGShare;          //relevant only if Global Table State

    int Shared;
}btb;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){

    struct branch_target_buffer* btb = (struct branch_target_buffer*)malloc(sizeof(struct branch_target_buffer));
    if (btb == NULL) {
        return -1;
    }

    //creating tag column and zeroing
    btb->tag = malloc(tagSize * btbSize);
    if (btb->tag == NULL) {
        free(btb);
        return -1;
    }
    memset(btb->tag,0,sizeof (btb->tag));

    //creating target column and zeroing
    btb->target = malloc((INSTRUCTION_SIZE) * btbSize);
    if (btb->target == NULL) {
        free(btb->tag);
        free(btb);
        return -1;
    }
    memset(btb->target,0,sizeof (btb->target));


    //creating History and zeroing
    if(isGlobalHist) {
        btb->history = malloc(historySize);                        //if global history, size= historysize
        if (btb->history == NULL) {
            free(btb->target);
            free(btb->tag);
            free(btb);
            return -1;
        }
    }
    else{
        btb->history = malloc(historySize * btbSize);             //if local History, size = historysize*btbsize
        if (btb->history == NULL) {
            free(btb->target);
            free(btb->tag);
            free(btb);
            return -1;
        }
    }
    memset(btb->history,0,sizeof (btb->history));


    //creating state table
    if (isGlobalTable) {                                               //if stateTable is global, size = 2*2^historysize
        btb->tableState = malloc(sizeof(enum state) * pow(2,historySize));
        if (btb->tableState == NULL) {
            free(btb->history);
            free(btb->target);
            free(btb->tag);
            free(btb);
            return -1;
        }
    }
    else{                                                               //if stateTable is local, size =2*2^btbsize
        btb->tableState = malloc(sizeof(enum state) * btbSize);
        if (btb->tableState == NULL) {
            free(btb->history);
            free(btb->target);
            free(btb->tag);
            free(btb);
            return -1;
        }
    }

    //filling table state with begin state
    enum state *stateArray = (enum state *)btb->tableState;
    size_t stateArraySize = isGlobalTable ? pow(2, historySize) : btbSize;
    for (size_t i = 0; i < stateArraySize; ++i) {
        stateArray[i] = fsmState;
    }

    btb->btbSize = btbSize;
    btb->historySize = historySize;
    btb->tagSize = tagSize;
    btb->beginState = fsmState;
    btb->isGlobalHist = isGlobalHist;
    btb->isGlobalTableState = isGlobalTable;
    btb->Shared=Shared;

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
    int upperBit = (int)log2(btb.btbSize);
    int mask = ((1<<upperBit)-1)>>2;
    int index = pc & mask  >> 2; // We got the index of the instruction in the btb


}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
    return;
}


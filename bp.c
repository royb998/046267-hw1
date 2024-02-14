/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#define INSTRUCTION_SIZE  32

struct btb_entry{
    int valid;
    int tag;
    int target;
}btb_entry;

struct branch_target_buffer{
    struct btb_entry* btbTable;
    unsigned* history;
    unsigned** tableState;

    unsigned int btbSize;       //num of lines in btb table
    unsigned int historySize;   //num history bit
    unsigned int tagSize;       //num of tag bit
    unsigned beginState;

    bool isGlobalHist;
    bool isGlobalTableState;
//  bool usingLGShare;          //relevant only if Global Table State
    int Shared;
    SIM_stats stats;

};

struct branch_target_buffer *btb;

unsigned getTagFromPc(int btbSize,int tagSize, uint32_t pc){
    int lenIndex = (int)log2(btbSize);
    unsigned tag = pc >> (2 + lenIndex); //align address
    return tag % (int)(pow(2, tagSize));
}


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
    unsigned int size = 0;
//    struct branch_target_buffer* btb = (struct branch_target_buffer*)malloc(sizeof(struct branch_target_buffer));
//    if (btb == NULL) {
//        return -1;
//    }

    //////////////////////////////////////creating btbTable and zeroing////////////////////////////////////
    btb->btbTable = malloc(btbSize * ( 1 + tagSize + INSTRUCTION_SIZE ));
    if (btb->btbTable->tag == NULL) {
        free(btb);
        return -1;
    }
    memset(btb->btbTable,0,sizeof (*btb->btbTable));
    size += 1 + tagSize + INSTRUCTION_SIZE;


    //////////////////////////////////////creating History and zeroing////////////////////////////////////
    if(isGlobalHist) {
        btb->history = malloc(sizeof(unsigned));                        //if global history, size= 8
        if (btb->history == NULL) {
            free(btb->btbTable);
            free(btb);
            return -1;
        }
        size += historySize;
    }
    else{
        btb->history = malloc(sizeof(unsigned) * btbSize);             //if local History, size = 8*btbsize
        if (btb->history == NULL) {
            free(btb->btbTable);
            free(btb);
            return -1;
        }
        size += btbSize * historySize;
    }
    memset(btb->history,0,sizeof (*btb->history));



    ///////////////////////////////////creating state table/////////////////////////////////////////////////
    if (isGlobalTable) {                                               //if stateTable is global, size = 2*2^historysize
        btb->tableState = malloc(sizeof(unsigned) * pow(2,historySize));
        if (btb->tableState == NULL) {
            free(btb->history);
            free(btb->btbTable);
            free(btb);
            return -1;
        }
        size += pow(2, (historySize + 1));
    }
    else{                                                               //if stateTable is local, size =2*2^btbsize
        btb->tableState = malloc(sizeof(unsigned) * btbSize);
        if (btb->tableState == NULL) {
            free(btb->history);
            free(btb->btbTable);
            free(btb);
            return -1;
        }
        size += btbSize * (int)pow(2, (historySize + 1));
    }



    /////////////////////////////////////filling table state with begin state////////////////////////////////////
    if (isGlobalTable){
        for (int i = 0; i < pow(2, historySize); i++){
            btb->tableState[0][i] = fsmState;
        }
    }
    else{
        for (int j = 0; j < btbSize; j++){
            for (int i = 0; i < pow(2, historySize); i++)
                btb->tableState[j][i] = fsmState;
        }
    }


    ////////////////////////////////////initialising values////////////////////////////////////
    btb->btbSize = btbSize;
    btb->historySize = historySize;
    btb->tagSize = tagSize;
    btb->beginState = fsmState;
    btb->isGlobalHist = isGlobalHist;
    btb->isGlobalTableState = isGlobalTable;
    btb->Shared=Shared;

    //initializing stats
    btb->stats.br_num=0;
    btb->stats.flush_num=0;
    btb->stats.size = size;

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
    int upperBit = (int)log2(btb->btbSize);
    int mask = ((1<<upperBit)-1)>>2;
    int index = pc & mask  >> 2; // We got the index of the instruction in the btb


}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
    curStats->br_num = btb->stats.br_num;
    curStats->flush_num = btb->stats.flush_num;
    curStats->size = btb->stats.size;

    free(btb->btbTable);
    free(btb->history);
    free(btb->tableState);
    return;
}


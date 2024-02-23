/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#define INSTRUCTION_SIZE  32

/* ----- Structs ----- */

struct btb_entry
{
    int valid;
    int tag;
    int target;
} btb_entry;

struct branch_target_buffer
{
    struct btb_entry * btbTable;
    unsigned * history;
    unsigned ** tableState;

    unsigned int btbSize;       // num of lines in btb table
    unsigned int historySize;   // num history bit
    unsigned int tagSize;       // num of tag bit
    unsigned beginState;        // first state

    bool isGlobalHist;
    bool isGlobalTableState;
    int Shared;                 // 0, 1 or 2

    SIM_stats stats;

};

/* ----- Globals ----- */

struct branch_target_buffer * btb;   // global btb

/* ----- Internal methods ----- */

unsigned getTagFromPc(unsigned btbSize, unsigned tagSize, uint32_t pc)
{
    int lenIndex = (int)log2(btbSize);
    unsigned tag = pc >> (2 + lenIndex); // align address
    return tag % (int)(pow(2, tagSize));
}

unsigned getBtbIndex(uint32_t pc)
{
    unsigned index = pc >> 2;
    return index % (unsigned)(pow(2, log2(btb->btbSize)));
}

unsigned getStateIndex(uint32_t pc)
{
    unsigned shareValue;
    unsigned n;

    if (btb->Shared == 1)
    {
        shareValue = (pc >> 2) % (unsigned)(pow(2, btb->historySize));
    }
    else if (btb->Shared == 2)
    {
        shareValue = (pc >> 16) % (unsigned)(pow(2, btb->historySize));
    }
    else
    {
        shareValue = 0;
    }

    if (btb->isGlobalHist)
    {
        n = *btb->history;
    }
    else
    {
        unsigned btbIndex = getBtbIndex(pc);
        n = btb->history[btbIndex];
    }

    return n ^ shareValue;
}

/* ----- External methods ----- */

int BP_init(
    unsigned btbSize,
    unsigned historySize,
    unsigned tagSize,
    unsigned fsmState,
    bool isGlobalHist,
    bool isGlobalTable,
    int Shared)
{
    unsigned int size = 0;

    // creating btbTable and zeroing
    btb->btbTable = malloc(btbSize * (1 + tagSize + INSTRUCTION_SIZE));
    if (btb->btbTable->tag == NULL)
    {
        free(btb);
        return -1;
    }
    memset(btb->btbTable, 0, sizeof(*btb->btbTable));
    size += 1 + tagSize + INSTRUCTION_SIZE;

    // creating History and zeroing
    if (isGlobalHist)
    {
        // if global history, size = 8
        btb->history = malloc(sizeof(unsigned));

        if (btb->history == NULL)
        {
            free(btb->btbTable);
            free(btb);
            return -1;
        }
        size += historySize;
    }
    else
    {
        // if local History, size = 8 * btb_size
        btb->history = malloc(sizeof(unsigned) * btbSize);

        if (btb->history == NULL)
        {
            free(btb->btbTable);
            free(btb);
            return -1;
        }
        size += btbSize * historySize;
    }
    memset(btb->history, 0, sizeof(*btb->history));

    // creating state table
    if (isGlobalTable)
    {
        // if stateTable is global, size = 2^history_size * 2
        btb->tableState = malloc(sizeof(unsigned) * (size_t)pow(2, historySize));
        if (btb->tableState == NULL)
        {
            free(btb->history);
            free(btb->btbTable);
            free(btb);
            return -1;
        }
        size += (unsigned)pow(2, (historySize + 1));
    }
    else
    {
        // if stateTable is local, size = btb_size * 2^history_size * 2
        btb->tableState = malloc(sizeof(unsigned) * btbSize * (size_t)pow(2, historySize));
        if (btb->tableState == NULL)
        {
            free(btb->history);
            free(btb->btbTable);
            free(btb);
            return -1;
        }
        size += btbSize * (unsigned)pow(2, (historySize + 1));
    }

    // filling table state with begin state
    if (isGlobalTable)
    {
        for (int i = 0; i < pow(2, historySize); i++)
        {
            btb->tableState[0][i] = fsmState;
        }
    }
    else
    {
        for (int j = 0; j < btbSize; j++)
        {
            for (int i = 0; i < pow(2, historySize); i++)
            {
                btb->tableState[j][i] = fsmState;
            }
        }
    }

    // initialising values
    btb->btbSize = btbSize;
    btb->historySize = historySize;
    btb->tagSize = tagSize;
    btb->beginState = fsmState;
    btb->isGlobalHist = isGlobalHist;
    btb->isGlobalTableState = isGlobalTable;
    btb->Shared = Shared;

    // initializing stats
    btb->stats.br_num = 0;
    btb->stats.flush_num = 0;
    btb->stats.size = size;

    return 0;
}

bool BP_predict(uint32_t pc, uint32_t * dst)
{
    unsigned btbIndex = getBtbIndex(pc);
    unsigned tableStateIndex = getStateIndex(pc);
    unsigned currentState = 0;

    if (btb->btbTable[btbIndex].valid == 0 ||
        btb->btbTable[btbIndex].tag != getTagFromPc(btb->btbSize, btb->tagSize, pc))
    {
        *dst = pc + 4;
        return false;
    }

    if (btb->isGlobalTableState)
    {
        currentState = *(btb->tableState[tableStateIndex]);
    }
    else
    {
        currentState = *(btb->tableState[btbIndex * (unsigned)pow(2, btb->historySize) + tableStateIndex]);
    }

    if (currentState > 1)
    {
        *dst = btb->btbTable[btbIndex].target;
        return true;
    }
    else
    {
        *dst = pc + 4;
        return false;
    }
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
    return;
}

void BP_GetStats(SIM_stats * curStats)
{
    curStats->br_num = btb->stats.br_num;
    curStats->flush_num = btb->stats.flush_num;
    curStats->size = btb->stats.size;

    free(btb->btbTable);
    free(btb->history);
    free(btb->tableState);
}


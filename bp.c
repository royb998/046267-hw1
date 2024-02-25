/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

/* ----- Macros ----- */

#define FREE(_ptr)          \
do {                        \
    if ((_ptr) != NULL) {   \
        free((_ptr));       \
}} while(0)

#define INSTRUCTION_SIZE  (32)

#define FSM_MASK (0x0000000000000003)

/* ----- Structs ----- */

typedef struct btb_entry
{
    bool valid;
    size_t tag;
    size_t target;
} btb_entry_t;

struct branch_target_buffer
{
    struct btb_entry * btbTable;
    size_t * history;
    size_t * tableState;
//    unsigned ** tableState;

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

struct branch_target_buffer g_btb;

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
    return index % g_btb.btbSize;
}

unsigned getStateIndex(uint32_t pc)
{
    unsigned shareValue;
    unsigned n;

    if (g_btb.Shared == 1)
    {
        shareValue = (pc >> 2) % (unsigned)(pow(2, g_btb.historySize));
    }
    else if (g_btb.Shared == 2)
    {
        shareValue = (pc >> 16) % (unsigned)(pow(2, g_btb.historySize));
    }
    else
    {
        shareValue = 0;
    }

    if (g_btb.isGlobalHist)
    {
        n = *g_btb.history;
    }
    else
    {
        unsigned btbIndex = getBtbIndex(pc);
        n = g_btb.history[btbIndex];
    }

    // Ensure correct history register size.
    n %= (1 << g_btb.historySize);

    return n ^ shareValue;
}

size_t get_state_table_size(size_t history_size)
{
    return (1 << history_size);
}

void reset_branch_states(uint32_t pc)
{
    size_t table_index = g_btb.isGlobalTableState ? 0 : getBtbIndex(pc);
    size_t table_base = table_index * get_state_table_size(g_btb.historySize);

    for (int i = 0; i < (1 << g_btb.historySize); ++i)
    {
        g_btb.tableState[table_base + i] = g_btb.beginState;
    }
}

size_t get_branch_state(uint32_t pc)
{
    size_t table_index = g_btb.isGlobalTableState ? 0 : getBtbIndex(pc);
    size_t table_base = table_index * get_state_table_size(g_btb.historySize);
    size_t state_index = getStateIndex(pc);

    // Only return lowest 2 bytes.
    return g_btb.tableState[table_base + state_index] & FSM_MASK;
}

void update_branch_state(uint32_t pc, bool taken)
{
    size_t table_index = g_btb.isGlobalTableState ? 0 : getBtbIndex(pc);
    size_t table_base = table_index * get_state_table_size(g_btb.historySize);
    size_t state_index = getStateIndex(pc);

    size_t * state = &g_btb.tableState[table_base + state_index];
    switch (*state & FSM_MASK)
    {
        case 0b00:
            *state = taken ? 0b01 : 0b00;
            break;
        case 0b01:
            *state = taken ? 0b10 : 0b00;
            break;
        case 0b10:
            *state = taken ? 0b11 : 0b01;
            break;
        case 0b11:
            *state = taken ? 0b11 : 0b10;
            break;
    }
}

/**
 * @brief Initialize the history register(s) for the predictor.
 * @param btbSize (IN)      Number of BTB entries.
 * @param historySize (IN)  Size of each history register.
 * @param isGlobalHist (IN) True if global history register should be used
 *                      instead of individual ones; False otherwise.
 * @param total_size (INOUT) Total (theoretical) memory size used by predictor.
 * @return 0 for success, -1 for failed allocation.
 */
int
initialize_history(
    unsigned int btbSize,
    unsigned int historySize,
    bool isGlobalHist,
    size_t * total_size)
{
    // If global history, keep only 1 history register.
    size_t history_register_count = isGlobalHist ? 1 : btbSize;

    // Initialize with zero for clean history.
    g_btb.history = calloc(history_register_count, sizeof(size_t));

    if (g_btb.history == NULL)
    {
        return -1;
    }

    // Update total size.
    (*total_size) += history_register_count * historySize;

    return 0;
}

/**
 * @brief Allocate the prediction state machine(s) for the predictor.
 * @param btbSize (IN)      Number of BTB entries.
 * @param historySize (IN)  Size of each history register.
 * @param isGlobalTable (IN) True if global prediction state machine should be
 *                      used instead of individual ones; False otherwise.
 * @param total_size (INOUT) Total (theoretical) memory size used by predictor.
 * @return 0 for success, -1 for failed allocation.
 */
int
allocate_states(
    unsigned int btbSize,
    unsigned int historySize,
    bool isGlobalTable,
    size_t * total_size)
{
    size_t state_table_size = 1 << historySize;

    // Only one entry if global table.
    size_t entry_count = isGlobalTable ? 1 : btbSize;

    // Allocate the entries - for each entry, allocate table of state machines.
    g_btb.tableState = malloc(entry_count * state_table_size * sizeof(*g_btb.tableState));
    if (g_btb.tableState == NULL)
    {
        return -1;
    }

    // 2 bits for every SM,
    (*total_size) += entry_count * state_table_size * 2;

    return 0;
}

void
initialize_states(
    unsigned int btbSize,
    unsigned int historySize,
    unsigned int fsmState,
    bool isGlobalTable)
{
    size_t table_count = isGlobalTable ? 1 : btbSize;
    size_t state_count = 1 << historySize;

    for (int i = 0; i < table_count; ++i)
    {
        for (int j = 0; j < state_count; j++)
        {
            g_btb.tableState[i * state_count + j] = fsmState;
        }
    }
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
    int return_value;

    // Theoretical total_size for each BTB entry.
    // Instruction lowest 2 bits are always 0, so don't keep them.
    size_t entry_size = 1 + tagSize + (INSTRUCTION_SIZE - 2);

    // Total size that should be used by the predictor.
    size_t total_size = 0;

    // Allocate the entries.
    g_btb.btbTable = calloc(btbSize, sizeof(btb_entry_t));
    if (NULL == g_btb.btbTable)
    {
        return -1;
    }

    total_size += btbSize * entry_size;

    // Allocate history registers.
    return_value = initialize_history(btbSize,
                                      historySize,
                                      isGlobalHist,
                                      &total_size);
    if (0 != return_value)
    {
        FREE(g_btb.btbTable);
        return -1;
    }

    // Allocate & fill prediction state machines.
    return_value = allocate_states(btbSize,
                                   historySize,
                                   isGlobalTable,
                                   &total_size);
    if (0 != return_value)
    {
        FREE(g_btb.btbTable);
        FREE(g_btb.history);
        return -1;
    }

    // filling table state with begin state
    initialize_states(btbSize, historySize, fsmState, isGlobalTable);

    // initialising values
    g_btb.btbSize = btbSize;
    g_btb.historySize = historySize;
    g_btb.tagSize = tagSize;
    g_btb.beginState = fsmState;
    g_btb.isGlobalHist = isGlobalHist;
    g_btb.isGlobalTableState = isGlobalTable;
    g_btb.Shared = Shared;

    // initializing stats
    g_btb.stats.br_num = 0;
    g_btb.stats.flush_num = 0;
    g_btb.stats.size = total_size;

    return 0;
}

bool BP_predict(uint32_t pc, uint32_t * dst)
{
    unsigned btbIndex = getBtbIndex(pc);
    unsigned currentState;

    // If the BTB isn't valid, or if the tags don't match, there's no prediction
    // for the given pc, assume not taken.
    if (!g_btb.btbTable[btbIndex].valid ||
        g_btb.btbTable[btbIndex].tag != getTagFromPc(g_btb.btbSize,
                                                     g_btb.tagSize, pc))
    {
        *dst = pc + 4;
        return false;
    }

    currentState = get_branch_state(pc);

    if (currentState > 1)
    {
        *dst = g_btb.btbTable[btbIndex].target;
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
    // Update branch count.
    g_btb.stats.br_num++;

    // Update flush count for mispredicts.
    if ((taken && pred_dst != targetPc) || (!taken && pc + 4 != pred_dst))
    {
        g_btb.stats.flush_num++;
    }

    btb_entry_t * btb_entry = &g_btb.btbTable[getBtbIndex(pc)];
    size_t tag = getTagFromPc(g_btb.btbSize, g_btb.tagSize, pc);
    size_t history_register_index = g_btb.isGlobalHist ? 0 : getBtbIndex(pc);

    // Reset local history register for new entries.
    if (!g_btb.isGlobalHist && (btb_entry->tag != tag || !btb_entry->valid))
    {
        g_btb.history[history_register_index] = 0;
        // TODO: Not sure if should reset state machines.
        reset_branch_states(pc);
    }

    // Update entry.
    btb_entry->tag = tag;
    btb_entry->target = targetPc;
    btb_entry->valid = true;

    // Update FSM.
    update_branch_state(pc, taken);

    // Update history register.
    g_btb.history[history_register_index] <<= 1;
    g_btb.history[history_register_index] |= taken ? 1 : 0;
    g_btb.history[history_register_index] &= (1 << g_btb.historySize) - 1;
}

void BP_GetStats(SIM_stats * curStats)
{
    curStats->br_num = g_btb.stats.br_num;
    curStats->flush_num = g_btb.stats.flush_num;
    curStats->size = g_btb.stats.size;

    FREE(g_btb.btbTable);
    FREE(g_btb.history);
    FREE(g_btb.tableState);
}


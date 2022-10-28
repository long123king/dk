#pragma once

#include "CmdInterface.h"

#include <map>

DECLARE_CMD(free_pool)
DECLARE_CMD(bigpool);
DECLARE_CMD(poolrange);
DECLARE_CMD(pooltrack);
DECLARE_CMD(poolmetrics);
DECLARE_CMD(tpool);
DECLARE_CMD(poolhdr);

class CPoolHeader
{
public:
	CPoolHeader(size_t data)
	{
		memcpy(this, &data, 8);
	}

	uint32_t prev_size : 8;
	uint32_t pool_index : 8;
	uint32_t block_size : 8;
	uint32_t pool_type : 8;
	char tag[4];
};

//kd> dt _POOL_TRACKER_BIG_PAGES
//nt!_POOL_TRACKER_BIG_PAGES
//+ 0x000 Va               : Uint8B
//+ 0x008 Key : Uint4B
//+ 0x00c Pattern : Pos 0, 8 Bits
//+ 0x00c PoolType : Pos 8, 12 Bits
//+ 0x00c SlushSize : Pos 20, 12 Bits
//+ 0x010 NumberOfBytes : Uint8B

class CBigPoolHeader
{
public:
	CBigPoolHeader(uint8_t* data)
	{
		memcpy(this, data, 0x18);
	}

	size_t va;
	char tag[4];
	uint32_t pattern : 8;
	uint32_t pool_type : 12;
	uint32_t slush_size : 12;
	size_t size;
};

/****************************************************
*                               8          5   2 1 0
*                               |          |   | | |
*                               V          V   V V V
* ___________________________________________________
* |                            | |        | | | | | |
* |____________________________|_|________|_|_|_|_|_|
*                               ^          ^   ^ ^ ^
*                               |          |   | | |
*                               |          |   | | 0: Non-Paged Pool
*                               |          |   | | 1: Paged Pool
*                               |          |   | |
*                               |          |   | 1: Must Succeed (0x02)
*                               |          |   |
*                               |          |   |
*                               |          |   1: Cache Aligned (0x04)
*                               |          |
*                               |          |
*                               |          1: Session Pool (0x20)
*                               |
*                               |
*                               1: NX, No Execute (0x200)
*
*****************************************************/

#define NonPagedPool                            0x0000
#define NonPagedPoolExecute                     0x0000
#define NonPagedPoolBase                        0x0000

#define PagedPool                               0x0001

#define NonPagedPoolMustSucceed                 0x0002
#define NonPagedPoolBaseMustSucceed             0x0002

#define DontUseThisType                         0x0003

#define NonPagedPoolCacheAligned                0x0004
#define NonPagedPoolBaseCacheAligned            0x0004

#define PagedPoolCacheAligned                   0x0005

#define NonPagedPoolCacheAlignedMustS           0x0006
#define NonPagedPoolBaseCacheAlignedMustS       0x0006

#define MaxPoolType                             0x0007

#define NonPagedPoolSession                     0x0020
#define PagedPoolSession                        0x0021
#define NonPagedPoolMustSucceedSession          0x0022
#define DontUseThisTypeSession                  0x0023
#define NonPagedPoolCacheAlignedSession         0x0024
#define PagedPoolCacheAlignedSession            0x0025
#define NonPagedPoolCacheAlignedMustSSession    0x0026

#define NonPagedPoolNx                          0x0200
#define NonPagedPoolNxCacheAligned              0x0204

#define NonPagedPoolSessionNx                   0x0220

class POOL_METRICS
{
public:
	size_t _pool_addr;
	size_t _pool_index;
	size_t _pool_type;
	size_t _pool_start;
	size_t _pool_end;
	size_t _total_pages;
	size_t _total_bytes;
	size_t _total_big_pages;
	string _comment;
	vector<size_t> _pending_frees;
	map<size_t, vector<size_t>> _free_lists;
};

void dump_free_pool(size_t size);
void dump_big_pool();
void dump_pool_track();
void dump_pool_range();
void dump_pool_metrics();
void tpool(size_t addr);
void poolhdr(size_t addr);
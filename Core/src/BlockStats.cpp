#include "BlockStats.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

template <typename T>
static T max(const T &a, const T &b) {
  return (a > b)? a : b;
}

////////////////////////////////////////////////////////////////////////////////
//
// BlockStat implementation
//
////////////////////////////////////////////////////////////////////////////////

BlockStat::BlockStat(const CHAR *statName, int stat) : m_IntStat(stat)
{
  strncpy(m_StatName, statName, kStatNameSz);
}

BlockStat::BlockStat(const CHAR *statName, double stat) : m_FloatStat(stat)
{
  strncpy(m_StatName, statName, kStatNameSz);
}

BlockStat::BlockStat(const BlockStat &other) {
  memcpy(this, &other, sizeof(*this));
}

BlockStat &BlockStat::operator=(const BlockStat &other) {
  memcpy(this, &other, sizeof(*this));
}

////////////////////////////////////////////////////////////////////////////////
//
// BlockStat Manager Implementation
//
////////////////////////////////////////////////////////////////////////////////

BlockStatManager::BlockStatManager(int nBlocks) 
  : m_BlockStatListSz(max(nBlocks, 0))
  , m_NextBlock(0)
{
  m_BlockStatList = new BlockStatList[m_BlockStatListSz];
  if(!m_BlockStatList) {
    fprintf(stderr, "Out of memory!\n");
    assert(false);
    exit(1);
  }
}

BlockStatManager::~BlockStatManager() {
  if(m_Counter.GetRefCount() == 0) {
    delete [] m_BlockStatList;
  }
}

uint32 BlockStatManager::BeginBlock() {
  if((m_NextBlock + 1) == m_BlockStatListSz) {
    fprintf(stderr, "WARNING -- BlockStatManager::BeginBlock(), reached end of block list.\n");
    assert(false);
    return m_NextBlock;
  }

  TCLock lock(m_Mutex);
  return m_NextBlock++;
}

void BlockStatManager::AddStat(uint32 blockIdx, const BlockStat &stat) {
  if(blockIdx >= m_BlockStatListSz) {
    fprintf(stderr, "WARNING -- BlockStatManager::AddStat(), block index out of bounds!\n");
    assert(false);
    return;
  }

  m_BlockStatList[blockIdx].AddStat(stat);
}

////////////////////////////////////////////////////////////////////////////////
//
// BlockStat List Implementation
//
////////////////////////////////////////////////////////////////////////////////
static const CHAR *kNullBlockString = "NULL_BLOCK_STAT";
static const uint32 kNullBlockStringLength = strlen(kNullBlockString);

BlockStatManager::BlockStatList::BlockStatList() 
  : m_Tail(0)
  , m_Stat(kNullBlockString, 0.0)
{ }

BlockStatManager::BlockStatList::BlockStatList(const BlockStat &stat) 
  : m_Tail(0)
  , m_Stat(stat)
{
}

BlockStatManager::BlockStatList::~BlockStatList() {
  if(m_Counter.GetRefCount() == 0 && m_Tail) {
    delete m_Tail;
  }
}

void BlockStatManager::BlockStatList::AddStat(const BlockStat &stat) {
  if(strncmp(stat.m_StatName, m_Stat.m_StatName, BlockStat::kStatNameSz) == 0) {
    m_Stat = stat;
  }
  else if(!m_Tail) {
    m_Tail->AddStat(stat);
  }
  else {
    if(strncmp(m_Stat.m_StatName, kNullBlockString, kNullBlockStringLength) == 0) {
      m_Stat = stat;
    }
    else {
      m_Tail = new BlockStatList(stat);
    }
  }
}
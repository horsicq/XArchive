/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* ===== Begin embedded xppmd7_local.c ===== */
#define ShrinkUnits X_Ppmd7_ShrinkUnits
/* Local renamed copies of the 7-Zip PPMd7 decoder C entry points. */

#include "xalgo_local.h"

#define Ppmd7_Construct X_Ppmd7_Construct
#define Ppmd7_Alloc X_Ppmd7_Alloc
#define Ppmd7_Free X_Ppmd7_Free
#define Ppmd7_Init X_Ppmd7_Init
#define Ppmd7_Update1 X_Ppmd7_Update1
#define Ppmd7_Update1_0 X_Ppmd7_Update1_0
#define Ppmd7_Update2 X_Ppmd7_Update2
#define Ppmd7_UpdateBin X_Ppmd7_UpdateBin
#define Ppmd7_UpdateModel X_Ppmd7_UpdateModel
#define Ppmd7_MakeEscFreq X_Ppmd7_MakeEscFreq
#define Ppmd7z_RangeDec_Init X_Ppmd7z_RangeDec_Init
#define Ppmd7z_DecodeSymbol X_Ppmd7z_DecodeSymbol
#define Ppmd7z_DecodeSymbols X_Ppmd7z_DecodeSymbols
/* Ppmd7.c -- PPMdH codec
2023-09-07 : Igor Pavlov : Public domain
This code is based on PPMd var.H (2001): Dmitry Shkarin : Public domain */
#include <string.h>
/* define PPMD7_ORDER_0_SUPPPORT to suport order-0 mode, unsupported by orignal PPMd var.H. code */
// #define PPMD7_ORDER_0_SUPPPORT
 
MY_ALIGN(16)
static const Byte PPMD7_kExpEscape[16] = { 25, 14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
MY_ALIGN(16)
static const UInt16 PPMD7_kInitBinEsc[] = { 0x3CDD, 0x1F3F, 0x59BF, 0x48F3, 0x64A1, 0x5ABC, 0x6632, 0x6051};

#define MAX_FREQ 124
#define UNIT_SIZE 12

#define U2B(nu) ((UInt32)(nu) * UNIT_SIZE)
#define U2I(nu) (p->Units2Indx[(size_t)(nu) - 1])
#define I2U(indx) ((unsigned)p->Indx2Units[indx])
#define I2U_UInt16(indx) ((UInt16)p->Indx2Units[indx])

#define REF(ptr) Ppmd_GetRef(p, ptr)

#define STATS_REF(ptr) ((CPpmd_State_Ref)REF(ptr))

#define CTX(ref) ((CPpmd7_Context *)Ppmd7_GetContext(p, ref))
#define STATS(ctx) Ppmd7_GetStats(p, ctx)
#define ONE_STATE(ctx) Ppmd7Context_OneState(ctx)
#define SUFFIX(ctx) CTX((ctx)->Suffix)

typedef CPpmd7_Context * PPMD7_CTX_PTR;

struct CPpmd7_Node_;

typedef Ppmd_Ref_Type(struct CPpmd7_Node_) CPpmd7_Node_Ref;

typedef struct CPpmd7_Node_
{
  UInt16 Stamp; /* must be at offset 0 as CPpmd7_Context::NumStats. Stamp=0 means free */
  UInt16 NU;
  CPpmd7_Node_Ref Next; /* must be at offset >= 4 */
  CPpmd7_Node_Ref Prev;
} CPpmd7_Node;

#define NODE(r)  Ppmd_GetPtr_Type(p, r, CPpmd7_Node)

void Ppmd7_Construct(CPpmd7 *p)
{
  unsigned i, k, m;

  p->Base = NULL;

  for (i = 0, k = 0; i < PPMD_NUM_INDEXES; i++)
  {
    unsigned step = (i >= 12 ? 4 : (i >> 2) + 1);
    do { p->Units2Indx[k++] = (Byte)i; } while (--step);
    p->Indx2Units[i] = (Byte)k;
  }

  p->NS2BSIndx[0] = (0 << 1);
  p->NS2BSIndx[1] = (1 << 1);
  memset(p->NS2BSIndx + 2, (2 << 1), 9);
  memset(p->NS2BSIndx + 11, (3 << 1), 256 - 11);

  for (i = 0; i < 3; i++)
    p->NS2Indx[i] = (Byte)i;

  for (m = i, k = 1; i < 256; i++)
  {
    p->NS2Indx[i] = (Byte)m;
    if (--k == 0)
      k = (++m) - 2;
  }

  memcpy(p->ExpEscape, PPMD7_kExpEscape, 16);
}


void Ppmd7_Free(CPpmd7 *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->Base);
  p->Size = 0;
  p->Base = NULL;
}


BoolInt Ppmd7_Alloc(CPpmd7 *p, UInt32 size, ISzAllocPtr alloc)
{
  if (!p->Base || p->Size != size)
  {
    Ppmd7_Free(p, alloc);
    p->AlignOffset = (4 - size) & 3;
    if ((p->Base = (Byte *)ISzAlloc_Alloc(alloc, p->AlignOffset + size)) == NULL)
      return SZ_False;
    p->Size = size;
  }
  return SZ_True;
}



// ---------- Internal Memory Allocator ----------

/* We can use CPpmd7_Node in list of free units (as in Ppmd8)
   But we still need one additional list walk pass in Ppmd7_GlueFreeBlocks().
   So we use simple CPpmd_Void_Ref instead of CPpmd7_Node in Ppmd7_InsertNode() / Ppmd7_RemoveNode()
*/

#define EMPTY_NODE 0


static void Ppmd7_InsertNode(CPpmd7 *p, void *node, unsigned indx)
{
  *((CPpmd_Void_Ref *)node) = p->FreeList[indx];
  // ((CPpmd7_Node *)node)->Next = (CPpmd7_Node_Ref)p->FreeList[indx];

  p->FreeList[indx] = REF(node);

}


static void *Ppmd7_RemoveNode(CPpmd7 *p, unsigned indx)
{
  CPpmd_Void_Ref *node = (CPpmd_Void_Ref *)Ppmd7_GetPtr(p, p->FreeList[indx]);
  p->FreeList[indx] = *node;
  // CPpmd7_Node *node = NODE((CPpmd7_Node_Ref)p->FreeList[indx]);
  // p->FreeList[indx] = node->Next;
  return node;
}


static void Ppmd7_SplitBlock(CPpmd7 *p, void *ptr, unsigned oldIndx, unsigned newIndx)
{
  unsigned i, nu = I2U(oldIndx) - I2U(newIndx);
  ptr = (Byte *)ptr + U2B(I2U(newIndx));
  if (I2U(i = U2I(nu)) != nu)
  {
    unsigned k = I2U(--i);
    Ppmd7_InsertNode(p, ((Byte *)ptr) + U2B(k), nu - k - 1);
  }
  Ppmd7_InsertNode(p, ptr, i);
}


/* we use CPpmd7_Node_Union union to solve XLC -O2 strict pointer aliasing problem */

typedef union
{
  CPpmd7_Node     Node;
  CPpmd7_Node_Ref NextRef;
} CPpmd7_Node_Union;

/* Original PPmdH (Ppmd7) code uses doubly linked list in Ppmd7_GlueFreeBlocks()
   we use single linked list similar to Ppmd8 code */


static void Ppmd7_GlueFreeBlocks(CPpmd7 *p)
{
  /*
  we use first UInt16 field of 12-bytes UNITs as record type stamp
    CPpmd_State    { Byte Symbol; Byte Freq; : Freq != 0
    CPpmd7_Context { UInt16 NumStats;        : NumStats != 0
    CPpmd7_Node    { UInt16 Stamp            : Stamp == 0 for free record
                                             : Stamp == 1 for head record and guard
    Last 12-bytes UNIT in array is always contains 12-bytes order-0 CPpmd7_Context record.
  */
  CPpmd7_Node_Ref head, n = 0;
 
  p->GlueCount = 255;

  
  /* we set guard NODE at LoUnit */
  if (p->LoUnit != p->HiUnit)
    ((CPpmd7_Node *)(void *)p->LoUnit)->Stamp = 1;

  {
    /* Create list of free blocks.
       We still need one additional list walk pass before Glue. */
    unsigned i;
    for (i = 0; i < PPMD_NUM_INDEXES; i++)
    {
      const UInt16 nu = I2U_UInt16(i);
      CPpmd7_Node_Ref next = (CPpmd7_Node_Ref)p->FreeList[i];
      p->FreeList[i] = 0;
      while (next != 0)
      {
        /* Don't change the order of the following commands: */
        CPpmd7_Node_Union *un = (CPpmd7_Node_Union *)NODE(next);
        const CPpmd7_Node_Ref tmp = next;
        next = un->NextRef;
        un->Node.Stamp = EMPTY_NODE;
        un->Node.NU = nu;
        un->Node.Next = n;
        n = tmp;
      }
    }
  }

  head = n;
  /* Glue and Fill must walk the list in same direction */
  {
    /* Glue free blocks */
    CPpmd7_Node_Ref *prev = &head;
    while (n)
    {
      CPpmd7_Node *node = NODE(n);
      UInt32 nu = node->NU;
      n = node->Next;
      if (nu == 0)
      {
        *prev = n;
        continue;
      }
      prev = &node->Next;
      for (;;)
      {
        CPpmd7_Node *node2 = node + nu;
        nu += node2->NU;
        if (node2->Stamp != EMPTY_NODE || nu >= 0x10000)
          break;
        node->NU = (UInt16)nu;
        node2->NU = 0;
      }
    }
  }

  /* Fill lists of free blocks */
  for (n = head; n != 0;)
  {
    CPpmd7_Node *node = NODE(n);
    UInt32 nu = node->NU;
    unsigned i;
    n = node->Next;
    if (nu == 0)
      continue;
    for (; nu > 128; nu -= 128, node += 128)
      Ppmd7_InsertNode(p, node, PPMD_NUM_INDEXES - 1);
    if (I2U(i = U2I(nu)) != nu)
    {
      unsigned k = I2U(--i);
      Ppmd7_InsertNode(p, node + k, (unsigned)nu - k - 1);
    }
    Ppmd7_InsertNode(p, node, i);
  }
}


Z7_NO_INLINE
static void *Ppmd7_AllocUnitsRare(CPpmd7 *p, unsigned indx)
{
  unsigned i;
  
  if (p->GlueCount == 0)
  {
    Ppmd7_GlueFreeBlocks(p);
    if (p->FreeList[indx] != 0)
      return Ppmd7_RemoveNode(p, indx);
  }
  
  i = indx;
  
  do
  {
    if (++i == PPMD_NUM_INDEXES)
    {
      UInt32 numBytes = U2B(I2U(indx));
      Byte *us = p->UnitsStart;
      p->GlueCount--;
      return ((UInt32)(us - p->Text) > numBytes) ? (p->UnitsStart = us - numBytes) : NULL;
    }
  }
  while (p->FreeList[i] == 0);

  {
    void *block = Ppmd7_RemoveNode(p, i);
    Ppmd7_SplitBlock(p, block, i, indx);
    return block;
  }
}


static void *Ppmd7_AllocUnits(CPpmd7 *p, unsigned indx)
{
  if (p->FreeList[indx] != 0)
    return Ppmd7_RemoveNode(p, indx);
  {
    UInt32 numBytes = U2B(I2U(indx));
    Byte *lo = p->LoUnit;
    if ((UInt32)(p->HiUnit - lo) >= numBytes)
    {
      p->LoUnit = lo + numBytes;
      return lo;
    }
  }
  return Ppmd7_AllocUnitsRare(p, indx);
}


#define MEM_12_CPY(dest, src, num) \
  { UInt32 *d = (UInt32 *)(dest); \
    const UInt32 *z = (const UInt32 *)(src); \
    unsigned n = (num); \
    do { \
      d[0] = z[0]; \
      d[1] = z[1]; \
      d[2] = z[2]; \
      z += 3; \
      d += 3; \
    } while (--n); \
  }


/*
static void *ShrinkUnits(CPpmd7 *p, void *oldPtr, unsigned oldNU, unsigned newNU)
{
  unsigned i0 = U2I(oldNU);
  unsigned i1 = U2I(newNU);
  if (i0 == i1)
    return oldPtr;
  if (p->FreeList[i1] != 0)
  {
    void *ptr = Ppmd7_RemoveNode(p, i1);
    MEM_12_CPY(ptr, oldPtr, newNU)
    Ppmd7_InsertNode(p, oldPtr, i0);
    return ptr;
  }
  Ppmd7_SplitBlock(p, oldPtr, i0, i1);
  return oldPtr;
}
*/


#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)
static void SetSuccessor(CPpmd_State *p, CPpmd_Void_Ref v)
{
  Ppmd_SET_SUCCESSOR(p, v)
}



Z7_NO_INLINE
static
void Ppmd7_RestartModel(CPpmd7 *p)
{
  unsigned i, k;

  memset(p->FreeList, 0, sizeof(p->FreeList));
  
  p->Text = p->Base + p->AlignOffset;
  p->HiUnit = p->Text + p->Size;
  p->LoUnit = p->UnitsStart = p->HiUnit - p->Size / 8 / UNIT_SIZE * 7 * UNIT_SIZE;
  p->GlueCount = 0;

  p->OrderFall = p->MaxOrder;
  p->RunLength = p->InitRL = -(Int32)((p->MaxOrder < 12) ? p->MaxOrder : 12) - 1;
  p->PrevSuccess = 0;

  {
    CPpmd7_Context *mc = (PPMD7_CTX_PTR)(void *)(p->HiUnit -= UNIT_SIZE); /* AllocContext(p); */
    CPpmd_State *s = (CPpmd_State *)p->LoUnit; /* Ppmd7_AllocUnits(p, PPMD_NUM_INDEXES - 1); */
    
    p->LoUnit += U2B(256 / 2);
    p->MaxContext = p->MinContext = mc;
    p->FoundState = s;

    mc->NumStats = 256;
    mc->Union2.SummFreq = 256 + 1;
    mc->Union4.Stats = REF(s);
    mc->Suffix = 0;

    for (i = 0; i < 256; i++, s++)
    {
      s->Symbol = (Byte)i;
      s->Freq = 1;
      SetSuccessor(s, 0);
    }

    #ifdef PPMD7_ORDER_0_SUPPPORT
    if (p->MaxOrder == 0)
    {
      CPpmd_Void_Ref r = REF(mc);
      s = p->FoundState;
      for (i = 0; i < 256; i++, s++)
        SetSuccessor(s, r);
      return;
    }
    #endif
  }

  for (i = 0; i < 128; i++)
    
    
    
    for (k = 0; k < 8; k++)
    {
      unsigned m;
      UInt16 *dest = p->BinSumm[i] + k;
      const UInt16 val = (UInt16)(PPMD_BIN_SCALE - PPMD7_kInitBinEsc[k] / (i + 2));
      for (m = 0; m < 64; m += 8)
        dest[m] = val;
    }

    
  for (i = 0; i < 25; i++)
  {

    CPpmd_See *s = p->See[i];
    
    
    
    unsigned summ = ((5 * i + 10) << (PPMD_PERIOD_BITS - 4));
    for (k = 0; k < 16; k++, s++)
    {
      s->Summ = (UInt16)summ;
      s->Shift = (PPMD_PERIOD_BITS - 4);
      s->Count = 4;
    }
  }
  
  p->DummySee.Summ = 0; /* unused */
  p->DummySee.Shift = PPMD_PERIOD_BITS;
  p->DummySee.Count = 64; /* unused */
}


void Ppmd7_Init(CPpmd7 *p, unsigned maxOrder)
{
  p->MaxOrder = maxOrder;

  Ppmd7_RestartModel(p);
}



/*
  Ppmd7_CreateSuccessors()
  It's called when (FoundState->Successor) is RAW-Successor,
  that is the link to position in Raw text.
  So we create Context records and write the links to
  FoundState->Successor and to identical RAW-Successors in suffix
  contexts of MinContex.
  
  The function returns:
  if (OrderFall == 0) then MinContext is already at MAX order,
    { return pointer to new or existing context of same MAX order }
  else
    { return pointer to new real context that will be (Order+1) in comparison with MinContext

  also it can return pointer to real context of same order,
*/

Z7_NO_INLINE
static PPMD7_CTX_PTR Ppmd7_CreateSuccessors(CPpmd7 *p)
{
  PPMD7_CTX_PTR c = p->MinContext;
  CPpmd_Byte_Ref upBranch = (CPpmd_Byte_Ref)SUCCESSOR(p->FoundState);
  Byte newSym, newFreq;
  unsigned numPs = 0;
  CPpmd_State *ps[PPMD7_MAX_ORDER];

  if (p->OrderFall != 0)
    ps[numPs++] = p->FoundState;
  
  while (c->Suffix)
  {
    CPpmd_Void_Ref successor;
    CPpmd_State *s;
    c = SUFFIX(c);
    

    if (c->NumStats != 1)
    {
      Byte sym = p->FoundState->Symbol;
      for (s = STATS(c); s->Symbol != sym; s++);

    }
    else
    {
      s = ONE_STATE(c);

    }
    successor = SUCCESSOR(s);
    if (successor != upBranch)
    {
      // (c) is real record Context here,
      c = CTX(successor);
      if (numPs == 0)
      {
        // (c) is real record MAX Order Context here,
        // So we don't need to create any new contexts.
        return c;
      }
      break;
    }
    ps[numPs++] = s;
  }
  
  // All created contexts will have single-symbol with new RAW-Successor
  // All new RAW-Successors will point to next position in RAW text
  // after FoundState->Successor

  newSym = *(const Byte *)Ppmd7_GetPtr(p, upBranch);
  upBranch++;
  
  
  if (c->NumStats == 1)
    newFreq = ONE_STATE(c)->Freq;
  else
  {
    UInt32 cf, s0;
    CPpmd_State *s;
    for (s = STATS(c); s->Symbol != newSym; s++);
    cf = (UInt32)s->Freq - 1;
    s0 = (UInt32)c->Union2.SummFreq - c->NumStats - cf;
    /*
      cf - is frequency of symbol that will be Successor in new context records.
      s0 - is commulative frequency sum of another symbols from parent context.
      max(newFreq)= (s->Freq + 1), when (s0 == 1)
      we have requirement (Ppmd7Context_OneState()->Freq <= 128) in BinSumm[]
      so (s->Freq < 128) - is requirement for multi-symbol contexts
    */
    newFreq = (Byte)(1 + ((2 * cf <= s0) ? (5 * cf > s0) : (2 * cf + s0 - 1) / (2 * s0) + 1));
  }

  // Create new single-symbol contexts from low order to high order in loop

  do
  {
    PPMD7_CTX_PTR c1;
    /* = AllocContext(p); */
    if (p->HiUnit != p->LoUnit)
      c1 = (PPMD7_CTX_PTR)(void *)(p->HiUnit -= UNIT_SIZE);
    else if (p->FreeList[0] != 0)
      c1 = (PPMD7_CTX_PTR)Ppmd7_RemoveNode(p, 0);
    else
    {
      c1 = (PPMD7_CTX_PTR)Ppmd7_AllocUnitsRare(p, 0);
      if (!c1)
        return NULL;
    }
    
    c1->NumStats = 1;
    ONE_STATE(c1)->Symbol = newSym;
    ONE_STATE(c1)->Freq = newFreq;
    SetSuccessor(ONE_STATE(c1), upBranch);
    c1->Suffix = REF(c);
    SetSuccessor(ps[--numPs], REF(c1));
    c = c1;
  }
  while (numPs != 0);
  
  return c;
}



#define SWAP_STATES(s) \
  { CPpmd_State tmp = s[0]; s[0] = s[-1]; s[-1] = tmp; }


void Ppmd7_UpdateModel(CPpmd7 *p);
Z7_NO_INLINE
void Ppmd7_UpdateModel(CPpmd7 *p)
{
  CPpmd_Void_Ref maxSuccessor, minSuccessor;
  PPMD7_CTX_PTR c, mc;
  unsigned s0, ns;



  if (p->FoundState->Freq < MAX_FREQ / 4 && p->MinContext->Suffix != 0)
  {
    /* Update Freqs in Suffix Context */

    c = SUFFIX(p->MinContext);
    
    if (c->NumStats == 1)
    {
      CPpmd_State *s = ONE_STATE(c);
      if (s->Freq < 32)
        s->Freq++;
    }
    else
    {
      CPpmd_State *s = STATS(c);
      Byte sym = p->FoundState->Symbol;
      
      if (s->Symbol != sym)
      {
        do
        {
          // s++; if (s->Symbol == sym) break;
          s++;
        }
        while (s->Symbol != sym);
        
        if (s[0].Freq >= s[-1].Freq)
        {
          SWAP_STATES(s)
          s--;
        }
      }

      if (s->Freq < MAX_FREQ - 9)
      {
        s->Freq = (Byte)(s->Freq + 2);
        c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 2);
      }
    }
  }

  
  if (p->OrderFall == 0)
  {
    /* MAX ORDER context */
    /* (FoundState->Successor) is RAW-Successor. */
    p->MaxContext = p->MinContext = Ppmd7_CreateSuccessors(p);
    if (!p->MinContext)
    {
      Ppmd7_RestartModel(p);
      return;
    }
    SetSuccessor(p->FoundState, REF(p->MinContext));
    return;
  }

  
  /* NON-MAX ORDER context */
  
  {
    Byte *text = p->Text;
    *text++ = p->FoundState->Symbol;
    p->Text = text;
    if (text >= p->UnitsStart)
    {
      Ppmd7_RestartModel(p);
      return;
    }
    maxSuccessor = REF(text);
  }
  
  minSuccessor = SUCCESSOR(p->FoundState);

  if (minSuccessor)
  {
    // there is Successor for FoundState in MinContext.
    // So the next context will be one order higher than MinContext.
    
    if (minSuccessor <= maxSuccessor)
    {
      // minSuccessor is RAW-Successor. So we will create real contexts records:
      PPMD7_CTX_PTR cs = Ppmd7_CreateSuccessors(p);
      if (!cs)
      {
        Ppmd7_RestartModel(p);
        return;
      }
      minSuccessor = REF(cs);
    }

    // minSuccessor now is real Context pointer that points to existing (Order+1) context
    
    if (--p->OrderFall == 0)
    {
      /*
      if we move to MaxOrder context, then minSuccessor will be common Succesor for both:
        MinContext that is (MaxOrder - 1)
        MaxContext that is (MaxOrder)
      so we don't need new RAW-Successor, and we can use real minSuccessor
      as succssors for both MinContext and MaxContext.
      */
      maxSuccessor = minSuccessor;
      
      /*
      if (MaxContext != MinContext)
      {
        there was order fall from MaxOrder and we don't need current symbol
        to transfer some RAW-Succesors to real contexts.
        So we roll back pointer in raw data for one position.
      }
      */
      p->Text -= (p->MaxContext != p->MinContext);
    }
  }
  else
  {
    /*
    FoundState has NULL-Successor here.
    And only root 0-order context can contain NULL-Successors.
    We change Successor in FoundState to RAW-Successor,
    And next context will be same 0-order root Context.
    */
    SetSuccessor(p->FoundState, maxSuccessor);
    minSuccessor = REF(p->MinContext);
  }

  mc = p->MinContext;
  c = p->MaxContext;

  p->MaxContext = p->MinContext = CTX(minSuccessor);

  if (c == mc)
    return;

  // s0 : is pure Escape Freq
  s0 = mc->Union2.SummFreq - (ns = mc->NumStats) - ((unsigned)p->FoundState->Freq - 1);

  do
  {
    unsigned ns1;
    UInt32 sum;
    
    if ((ns1 = c->NumStats) != 1)
    {
      if ((ns1 & 1) == 0)
      {
        /* Expand for one UNIT */
        const unsigned oldNU = ns1 >> 1;
        const unsigned i = U2I(oldNU);
        if (i != U2I((size_t)oldNU + 1))
        {
          void *ptr = Ppmd7_AllocUnits(p, i + 1);
          void *oldPtr;
          if (!ptr)
          {
            Ppmd7_RestartModel(p);
            return;
          }
          oldPtr = STATS(c);
          MEM_12_CPY(ptr, oldPtr, oldNU)
          Ppmd7_InsertNode(p, oldPtr, i);
          c->Union4.Stats = STATS_REF(ptr);
        }
      }
      sum = c->Union2.SummFreq;
      /* max increase of Escape_Freq is 3 here.
         total increase of Union2.SummFreq for all symbols is less than 256 here */
      sum += (UInt32)(unsigned)((2 * ns1 < ns) + 2 * ((unsigned)(4 * ns1 <= ns) & (sum <= 8 * ns1)));
      /* original PPMdH uses 16-bit variable for (sum) here.
         But (sum < 0x9000). So we don't truncate (sum) to 16-bit */
      // sum = (UInt16)sum;
    }
    else
    {
      // instead of One-symbol context we create 2-symbol context
      CPpmd_State *s = (CPpmd_State*)Ppmd7_AllocUnits(p, 0);
      if (!s)
      {
        Ppmd7_RestartModel(p);
        return;
      }
      {
        unsigned freq = c->Union2.State2.Freq;
        // s = *ONE_STATE(c);
        s->Symbol = c->Union2.State2.Symbol;
        s->Successor_0 = c->Union4.State4.Successor_0;
        s->Successor_1 = c->Union4.State4.Successor_1;
        // SetSuccessor(s, c->Union4.Stats);  // call it only for debug purposes to check the order of
                                              // (Successor_0 and Successor_1) in LE/BE.
        c->Union4.Stats = REF(s);
        if (freq < MAX_FREQ / 4 - 1)
          freq <<= 1;
        else
          freq = MAX_FREQ - 4;
        // (max(s->freq) == 120), when we convert from 1-symbol into 2-symbol context
        s->Freq = (Byte)freq;
        // max(InitEsc = PPMD7_kExpEscape[*]) is 25. So the max(escapeFreq) is 26 here
        sum = (UInt32)(freq + p->InitEsc + (ns > 3));
      }
    }
    
    {
      CPpmd_State *s = STATS(c) + ns1;
      UInt32 cf = 2 * (sum + 6) * (UInt32)p->FoundState->Freq;
      UInt32 sf = (UInt32)s0 + sum;
      s->Symbol = p->FoundState->Symbol;
      c->NumStats = (UInt16)(ns1 + 1);
      SetSuccessor(s, maxSuccessor);
      
      if (cf < 6 * sf)
      {
        cf = (UInt32)1 + (cf > sf) + (cf >= 4 * sf);
        sum += 3;
        /* It can add (0, 1, 2) to Escape_Freq */
      }
      else
      {
        cf = (UInt32)4 + (cf >= 9 * sf) + (cf >= 12 * sf) + (cf >= 15 * sf);
        sum += cf;
      }
     
      c->Union2.SummFreq = (UInt16)sum;
      s->Freq = (Byte)cf;
    }
    c = SUFFIX(c);
  }
  while (c != mc);
}
  


Z7_NO_INLINE
static void Ppmd7_Rescale(CPpmd7 *p)
{
  unsigned i, adder, sumFreq, escFreq;
  CPpmd_State *stats = STATS(p->MinContext);
  CPpmd_State *s = p->FoundState;

  /* Sort the list by Freq */
  if (s != stats)
  {
    CPpmd_State tmp = *s;
    do
      s[0] = s[-1];
    while (--s != stats);
    *s = tmp;
  }

  sumFreq = s->Freq;
  escFreq = p->MinContext->Union2.SummFreq - sumFreq;
  
  /*
  if (p->OrderFall == 0), adder = 0 : it's     allowed to remove symbol from     MAX Order context
  if (p->OrderFall != 0), adder = 1 : it's NOT allowed to remove symbol from NON-MAX Order context
  */

  adder = (p->OrderFall != 0);

  #ifdef PPMD7_ORDER_0_SUPPPORT
  adder |= (p->MaxOrder == 0); // we don't remove symbols from order-0 context
  #endif

  sumFreq = (sumFreq + 4 + adder) >> 1;
  i = (unsigned)p->MinContext->NumStats - 1;
  s->Freq = (Byte)sumFreq;
  
  do
  {
    unsigned freq = (++s)->Freq;
    escFreq -= freq;
    freq = (freq + adder) >> 1;
    sumFreq += freq;
    s->Freq = (Byte)freq;
    if (freq > s[-1].Freq)
    {
      CPpmd_State tmp = *s;
      CPpmd_State *s1 = s;
      do
      {
        s1[0] = s1[-1];
      }
      while (--s1 != stats && freq > s1[-1].Freq);
      *s1 = tmp;
    }
  }
  while (--i);
  
  if (s->Freq == 0)
  {
    /* Remove all items with Freq == 0 */
    CPpmd7_Context *mc;
    unsigned numStats, numStatsNew, n0, n1;
    
    i = 0; do { i++; } while ((--s)->Freq == 0);
    
    /* We increase (escFreq) for the number of removed symbols.
       So we will have (0.5) increase for Escape_Freq in avarage per
       removed symbol after Escape_Freq halving */
    escFreq += i;
    mc = p->MinContext;
    numStats = mc->NumStats;
    numStatsNew = numStats - i;
    mc->NumStats = (UInt16)(numStatsNew);
    n0 = (numStats + 1) >> 1;
    
    if (numStatsNew == 1)
    {
      /* Create Single-Symbol context */
      unsigned freq = stats->Freq;
      
      do
      {
        escFreq >>= 1;
        freq = (freq + 1) >> 1;
      }
      while (escFreq > 1);

      s = ONE_STATE(mc);
      *s = *stats;
      s->Freq = (Byte)freq; // (freq <= 260 / 4)
      p->FoundState = s;
      Ppmd7_InsertNode(p, stats, U2I(n0));
      return;
    }
    
    n1 = (numStatsNew + 1) >> 1;
    if (n0 != n1)
    {
      // p->MinContext->Union4.Stats = STATS_REF(ShrinkUnits(p, stats, n0, n1));
      unsigned i0 = U2I(n0);
      unsigned i1 = U2I(n1);
      if (i0 != i1)
      {
        if (p->FreeList[i1] != 0)
        {
          void *ptr = Ppmd7_RemoveNode(p, i1);
          p->MinContext->Union4.Stats = STATS_REF(ptr);
          MEM_12_CPY(ptr, (const void *)stats, n1)
          Ppmd7_InsertNode(p, stats, i0);
        }
        else
          Ppmd7_SplitBlock(p, stats, i0, i1);
      }
    }
  }
  {
    CPpmd7_Context *mc = p->MinContext;
    mc->Union2.SummFreq = (UInt16)(sumFreq + escFreq - (escFreq >> 1));
    // Escape_Freq halving here
    p->FoundState = STATS(mc);
  }
}


CPpmd_See *Ppmd7_MakeEscFreq(CPpmd7 *p, unsigned numMasked, UInt32 *escFreq)
{
  CPpmd_See *see;
  const CPpmd7_Context *mc = p->MinContext;
  unsigned numStats = mc->NumStats;
  if (numStats != 256)
  {
    unsigned nonMasked = numStats - numMasked;
    see = p->See[(unsigned)p->NS2Indx[(size_t)nonMasked - 1]]
        + (nonMasked < (unsigned)SUFFIX(mc)->NumStats - numStats)
        + 2 * (unsigned)(mc->Union2.SummFreq < 11 * numStats)
        + 4 * (unsigned)(numMasked > nonMasked) +
        p->HiBitsFlag;
    {
      // if (see->Summ) field is larger than 16-bit, we need only low 16 bits of Summ
      const unsigned summ = (UInt16)see->Summ; // & 0xFFFF
      const unsigned r = (summ >> see->Shift);
      see->Summ = (UInt16)(summ - r);
      *escFreq = (UInt32)(r + (r == 0));
    }
  }
  else
  {
    see = &p->DummySee;
    *escFreq = 1;
  }
  return see;
}


static void Ppmd7_NextContext(CPpmd7 *p)
{
  PPMD7_CTX_PTR c = CTX(SUCCESSOR(p->FoundState));
  if (p->OrderFall == 0 && (const Byte *)c > p->Text)
    p->MaxContext = p->MinContext = c;
  else
    Ppmd7_UpdateModel(p);
}


void Ppmd7_Update1(CPpmd7 *p)
{
  CPpmd_State *s = p->FoundState;
  unsigned freq = s->Freq;
  freq += 4;
  p->MinContext->Union2.SummFreq = (UInt16)(p->MinContext->Union2.SummFreq + 4);
  s->Freq = (Byte)freq;
  if (freq > s[-1].Freq)
  {
    SWAP_STATES(s)
    p->FoundState = --s;
    if (freq > MAX_FREQ)
      Ppmd7_Rescale(p);
  }
  Ppmd7_NextContext(p);
}


void Ppmd7_Update1_0(CPpmd7 *p)
{
  CPpmd_State *s = p->FoundState;
  CPpmd7_Context *mc = p->MinContext;
  unsigned freq = s->Freq;
  const unsigned summFreq = mc->Union2.SummFreq;
  p->PrevSuccess = (2 * freq > summFreq);
  p->RunLength += (Int32)p->PrevSuccess;
  mc->Union2.SummFreq = (UInt16)(summFreq + 4);
  freq += 4;
  s->Freq = (Byte)freq;
  if (freq > MAX_FREQ)
    Ppmd7_Rescale(p);
  Ppmd7_NextContext(p);
}


/*
void Ppmd7_UpdateBin(CPpmd7 *p)
{
  unsigned freq = p->FoundState->Freq;
  p->FoundState->Freq = (Byte)(freq + (freq < 128));
  p->PrevSuccess = 1;
  p->RunLength++;
  Ppmd7_NextContext(p);
}
*/

void Ppmd7_Update2(CPpmd7 *p)
{
  CPpmd_State *s = p->FoundState;
  unsigned freq = s->Freq;
  freq += 4;
  p->RunLength = p->InitRL;
  p->MinContext->Union2.SummFreq = (UInt16)(p->MinContext->Union2.SummFreq + 4);
  s->Freq = (Byte)freq;
  if (freq > MAX_FREQ)
    Ppmd7_Rescale(p);
  Ppmd7_UpdateModel(p);
}



/*
PPMd Memory Map:
{
  [ 0 ]           contains subset of original raw text, that is required to create context
                  records, Some symbols are not written, when max order context was reached
  [ Text ]        free area
  [ UnitsStart ]  CPpmd_State vectors and CPpmd7_Context records
  [ LoUnit ]      free  area for CPpmd_State and CPpmd7_Context items
[ HiUnit ]      CPpmd7_Context records
  [ Size ]        end of array
}

These addresses don't cross at any time.
And the following condtions is true for addresses:
  (0  <= Text < UnitsStart <= LoUnit <= HiUnit <= Size)

Raw text is BYTE--aligned.
the data in block [ UnitsStart ... Size ] contains 12-bytes aligned UNITs.

Last UNIT of array at offset (Size - 12) is root order-0 CPpmd7_Context record.
The code can free UNITs memory blocks that were allocated to store CPpmd_State vectors.
The code doesn't free UNITs allocated for CPpmd7_Context records.

The code calls Ppmd7_RestartModel(), when there is no free memory for allocation.
And Ppmd7_RestartModel() changes the state to orignal start state, with full free block.


The code allocates UNITs with the following order:

Allocation of 1 UNIT for Context record
  - from free space (HiUnit) down to (LoUnit)
  - from FreeList[0]
  - Ppmd7_AllocUnitsRare()

Ppmd7_AllocUnits() for CPpmd_State vectors:
  - from FreeList[i]
  - from free space (LoUnit) up to (HiUnit)
  - Ppmd7_AllocUnitsRare()

Ppmd7_AllocUnitsRare()
  - if (GlueCount == 0)
       {  Glue lists, GlueCount = 255, allocate from FreeList[i]] }
  - loop for all higher sized FreeList[...] lists
  - from (UnitsStart - Text), GlueCount--
  - ERROR


Each Record with Context contains the CPpmd_State vector, where each
CPpmd_State contains the link to Successor.
There are 3 types of Successor:
  1) NULL-Successor   - NULL pointer. NULL-Successor links can be stored
                        only in 0-order Root Context Record.
                        We use 0 value as NULL-Successor
  2) RAW-Successor    - the link to position in raw text,
                        that "RAW-Successor" is being created after first
                        occurrence of new symbol for some existing context record.
                        (RAW-Successor > 0).
  3) RECORD-Successor - the link to CPpmd7_Context record of (Order+1),
                        that record is being created when we go via RAW-Successor again.

For any successors at any time: the following condtions are true for Successor links:
(NULL-Successor < RAW-Successor < UnitsStart <= RECORD-Successor)


---------- Symbol Frequency, SummFreq and Range in Range_Coder ----------

CPpmd7_Context::SummFreq = Sum(Stats[].Freq) + Escape_Freq

The PPMd code tries to fulfill the condition:
  (SummFreq <= (256 * 128 = RC::kBot))

We have (Sum(Stats[].Freq) <= 256 * 124), because of (MAX_FREQ = 124)
So (4 = 128 - 124) is average reserve for Escape_Freq for each symbol.
If (CPpmd_State::Freq) is not aligned for 4, the reserve can be 5, 6 or 7.
SummFreq and Escape_Freq can be changed in Ppmd7_Rescale() and *Update*() functions.
Ppmd7_Rescale() can remove symbols only from max-order contexts. So Escape_Freq can increase after multiple calls of Ppmd7_Rescale() for
max-order context.

When the PPMd code still break (Total <= RC::Range) condition in range coder,
we have two ways to resolve that problem:
  1) we can report error, if we want to keep compatibility with original PPMd code that has no fix for such cases.
  2) we can reduce (Total) value to (RC::Range) by reducing (Escape_Freq) part of (Total) value.
*/

#undef MAX_FREQ
#undef UNIT_SIZE
#undef U2B
#undef U2I
#undef I2U
#undef I2U_UInt16
#undef REF
#undef STATS_REF
#undef CTX
#undef STATS
#undef ONE_STATE
#undef SUFFIX
#undef NODE
#undef EMPTY_NODE
#undef MEM_12_CPY
#undef SUCCESSOR
#undef SWAP_STATES

/* Ppmd7Dec.c -- Ppmd7z (PPMdH with 7z Range Coder) Decoder
2023-09-07 : Igor Pavlov : Public domain
This code is based on:
  PPMd var.H (2001): Dmitry Shkarin : Public domain */
#define kTopValue ((UInt32)1 << 24)


#define READ_BYTE(p) IByteIn_Read((p)->Stream)

BoolInt Ppmd7z_RangeDec_Init(CPpmd7_RangeDec *p)
{
  unsigned i;
  p->Code = 0;
  p->Range = 0xFFFFFFFF;
  if (READ_BYTE(p) != 0)
    return SZ_False;
  for (i = 0; i < 4; i++)
    p->Code = (p->Code << 8) | READ_BYTE(p);
  return (p->Code < 0xFFFFFFFF);
}

#define RC_NORM_BASE(p) if ((p)->Range < kTopValue) \
  { (p)->Code = ((p)->Code << 8) | READ_BYTE(p); (p)->Range <<= 8;

#define RC_NORM_1(p)  RC_NORM_BASE(p) }
#define RC_NORM(p)    RC_NORM_BASE(p) RC_NORM_BASE(p) }}

// we must use only one type of Normalization from two: LOCAL or REMOTE
#define RC_NORM_LOCAL(p)    // RC_NORM(p)
#define RC_NORM_REMOTE(p)   RC_NORM(p)

#define R (&p->rc.dec)

Z7_FORCE_INLINE
// Z7_NO_INLINE
static void Ppmd7z_RD_Decode(CPpmd7 *p, UInt32 start, UInt32 size)
{

  
  R->Code -= start * R->Range;
  R->Range *= size;
  RC_NORM_LOCAL(R)
}

#define RC_Decode(start, size)  Ppmd7z_RD_Decode(p, start, size);
#define RC_DecodeFinal(start, size)  RC_Decode(start, size)  RC_NORM_REMOTE(R)
#define RC_GetThreshold(total)  (R->Code / (R->Range /= (total)))


#define CTX(ref) ((CPpmd7_Context *)Ppmd7_GetContext(p, ref))
// typedef CPpmd7_Context * CTX_PTR;
#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)
void Ppmd7_UpdateModel(CPpmd7 *p);

#define MASK(sym)  ((Byte *)charMask)[sym]
// Z7_FORCE_INLINE
// static
int Ppmd7z_DecodeSymbol(CPpmd7 *p)
{
  size_t charMask[256 / sizeof(size_t)];

  if (p->MinContext->NumStats != 1)
  {
    CPpmd_State *s = Ppmd7_GetStats(p, p->MinContext);
    unsigned i;
    UInt32 count, hiCnt;
    const UInt32 summFreq = p->MinContext->Union2.SummFreq;

    
    
    
    count = RC_GetThreshold(summFreq);
    hiCnt = count;
    
    if ((Int32)(count -= s->Freq) < 0)
    {
      Byte sym;
      RC_DecodeFinal(0, s->Freq)
      p->FoundState = s;
      sym = s->Symbol;
      Ppmd7_Update1_0(p);
      return sym;
    }
  
    p->PrevSuccess = 0;
    i = (unsigned)p->MinContext->NumStats - 1;
    
    do
    {
      if ((Int32)(count -= (++s)->Freq) < 0)
      {
        Byte sym;
        RC_DecodeFinal((hiCnt - count) - s->Freq, s->Freq)
        p->FoundState = s;
        sym = s->Symbol;
        Ppmd7_Update1(p);
        return sym;
      }
    }
    while (--i);
    
    if (hiCnt >= summFreq)
      return PPMD7_SYM_ERROR;
    
    hiCnt -= count;
    RC_Decode(hiCnt, summFreq - hiCnt)

    p->HiBitsFlag = PPMD7_HiBitsFlag_3(p->FoundState->Symbol);
    PPMD_SetAllBitsIn256Bytes(charMask)
    // i = p->MinContext->NumStats - 1;
    // do { MASK((--s)->Symbol) = 0; } while (--i);
    {
      CPpmd_State *s2 = Ppmd7_GetStats(p, p->MinContext);
      MASK(s->Symbol) = 0;
      do
      {
        const unsigned sym0 = s2[0].Symbol;
        const unsigned sym1 = s2[1].Symbol;
        s2 += 2;
        MASK(sym0) = 0;
        MASK(sym1) = 0;
      }
      while (s2 < s);
    }
  }
  else
  {
    CPpmd_State *s = Ppmd7Context_OneState(p->MinContext);
    UInt16 *prob = Ppmd7_GetBinSumm(p);
    UInt32 pr = *prob;
    UInt32 size0 = (R->Range >> 14) * pr;
    pr = PPMD_UPDATE_PROB_1(pr);

    if (R->Code < size0)
    {
      Byte sym;
      *prob = (UInt16)(pr + (1 << PPMD_INT_BITS));
      
      // RangeDec_DecodeBit0(size0);
      R->Range = size0;
      RC_NORM_1(R)
      /* we can use single byte normalization here because of
         (min(BinSumm[][]) = 95) > (1 << (14 - 8)) */

      // sym = (p->FoundState = Ppmd7Context_OneState(p->MinContext))->Symbol;
      // Ppmd7_UpdateBin(p);
      {
        unsigned freq = s->Freq;
        CPpmd7_Context *c = CTX(SUCCESSOR(s));
        sym = s->Symbol;
        p->FoundState = s;
        p->PrevSuccess = 1;
        p->RunLength++;
        s->Freq = (Byte)(freq + (freq < 128));
        // NextContext(p);
        if (p->OrderFall == 0 && (const Byte *)c > p->Text)
          p->MaxContext = p->MinContext = c;
        else
          Ppmd7_UpdateModel(p);
      }
      return sym;
    }

    *prob = (UInt16)pr;
    p->InitEsc = p->ExpEscape[pr >> 10];

    // RangeDec_DecodeBit1(size0);
    
    R->Code -= size0;
    R->Range -= size0;
    RC_NORM_LOCAL(R)
    
    PPMD_SetAllBitsIn256Bytes(charMask)
    MASK(Ppmd7Context_OneState(p->MinContext)->Symbol) = 0;
    p->PrevSuccess = 0;
  }

  for (;;)
  {
    CPpmd_State *s, *s2;
    UInt32 freqSum, count, hiCnt;

    CPpmd_See *see;
    CPpmd7_Context *mc;
    unsigned numMasked;
    RC_NORM_REMOTE(R)
    mc = p->MinContext;
    numMasked = mc->NumStats;

    do
    {
      p->OrderFall++;
      if (!mc->Suffix)
        return PPMD7_SYM_END;
      mc = Ppmd7_GetContext(p, mc->Suffix);
    }
    while (mc->NumStats == numMasked);
    
    s = Ppmd7_GetStats(p, mc);

    {
      unsigned num = mc->NumStats;
      unsigned num2 = num / 2;
      
      num &= 1;
      hiCnt = (s->Freq & (UInt32)(MASK(s->Symbol))) & (0 - (UInt32)num);
      s += num;
      p->MinContext = mc;

      do
      {
        const unsigned sym0 = s[0].Symbol;
        const unsigned sym1 = s[1].Symbol;
        s += 2;
        hiCnt += (s[-2].Freq & (UInt32)(MASK(sym0)));
        hiCnt += (s[-1].Freq & (UInt32)(MASK(sym1)));
      }
      while (--num2);
    }

    see = Ppmd7_MakeEscFreq(p, numMasked, &freqSum);
    freqSum += hiCnt;




    count = RC_GetThreshold(freqSum);
    
    if (count < hiCnt)
    {
      Byte sym;

      s = Ppmd7_GetStats(p, p->MinContext);
      hiCnt = count;
      // count -= s->Freq & (UInt32)(MASK(s->Symbol));
      // if ((Int32)count >= 0)
      {
        for (;;)
        {
          count -= s->Freq & (UInt32)(MASK((s)->Symbol)); s++; if ((Int32)count < 0) break;
          // count -= s->Freq & (UInt32)(MASK((s)->Symbol)); s++; if ((Int32)count < 0) break;
        }
      }
      s--;
      RC_DecodeFinal((hiCnt - count) - s->Freq, s->Freq)

      // new (see->Summ) value can overflow over 16-bits in some rare cases
      Ppmd_See_UPDATE(see)
      p->FoundState = s;
      sym = s->Symbol;
      Ppmd7_Update2(p);
      return sym;
    }

    if (count >= freqSum)
      return PPMD7_SYM_ERROR;
    
    RC_Decode(hiCnt, freqSum - hiCnt)

    // We increase (see->Summ) for sum of Freqs of all non_Masked symbols.
    // new (see->Summ) value can overflow over 16-bits in some rare cases
    see->Summ = (UInt16)(see->Summ + freqSum);

    s = Ppmd7_GetStats(p, p->MinContext);
    s2 = s + p->MinContext->NumStats;
    do
    {
      MASK(s->Symbol) = 0;
      s++;
    }
    while (s != s2);
  }
}

/*
Byte *Ppmd7z_DecodeSymbols(CPpmd7 *p, Byte *buf, const Byte *lim)
{
  int sym = 0;
  if (buf != lim)
  do
  {
    sym = Ppmd7z_DecodeSymbol(p);
    if (sym < 0)
      break;
    *buf = (Byte)sym;
  }
  while (++buf < lim);
  p->LastSymbol = sym;
  return buf;
}
*/

#undef kTopValue
#undef READ_BYTE
#undef RC_NORM_BASE
#undef RC_NORM_1
#undef RC_NORM
#undef RC_NORM_LOCAL
#undef RC_NORM_REMOTE
#undef R
#undef RC_Decode
#undef RC_DecodeFinal
#undef RC_GetThreshold
#undef CTX
#undef SUCCESSOR
#undef MASK
#undef Ppmd7z_DecodeSymbols
#undef Ppmd7z_DecodeSymbol
#undef Ppmd7z_RangeDec_Init
#undef Ppmd7_MakeEscFreq
#undef Ppmd7_UpdateModel
#undef Ppmd7_UpdateBin
#undef Ppmd7_Update2
#undef Ppmd7_Update1_0
#undef Ppmd7_Update1
#undef Ppmd7_Init
#undef Ppmd7_Free
#undef Ppmd7_Alloc
#undef Ppmd7_Construct
#undef ShrinkUnits
/* ===== End embedded xppmd7_local.c ===== */

/* ===== Begin embedded xppmd8_local.c ===== */
#define ShrinkUnits X_Ppmd8_ShrinkUnits
/* Local renamed copies of the 7-Zip PPMd8 decoder C entry points. */

#include "xalgo_local.h"

#define Ppmd8_Construct X_Ppmd8_Construct
#define Ppmd8_Alloc X_Ppmd8_Alloc
#define Ppmd8_Free X_Ppmd8_Free
#define Ppmd8_Init X_Ppmd8_Init
#define Ppmd8_Update1 X_Ppmd8_Update1
#define Ppmd8_Update1_0 X_Ppmd8_Update1_0
#define Ppmd8_Update2 X_Ppmd8_Update2
#define Ppmd8_UpdateBin X_Ppmd8_UpdateBin
#define Ppmd8_UpdateModel X_Ppmd8_UpdateModel
#define Ppmd8_MakeEscFreq X_Ppmd8_MakeEscFreq
#define Ppmd8_Init_RangeDec X_Ppmd8_Init_RangeDec
#define Ppmd8_DecodeSymbol X_Ppmd8_DecodeSymbol
/* Ppmd8.c -- PPMdI codec
2023-09-07 : Igor Pavlov : Public domain
This code is based on PPMd var.I (2002): Dmitry Shkarin : Public domain */
#include <string.h>
MY_ALIGN(16)
static const Byte PPMD8_kExpEscape[16] = { 25, 14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
MY_ALIGN(16)
static const UInt16 PPMD8_kInitBinEsc[] = { 0x3CDD, 0x1F3F, 0x59BF, 0x48F3, 0x64A1, 0x5ABC, 0x6632, 0x6051};

#define MAX_FREQ 124
#define UNIT_SIZE 12

#define U2B(nu) ((UInt32)(nu) * UNIT_SIZE)
#define U2I(nu) (p->Units2Indx[(size_t)(nu) - 1])
#define I2U(indx) ((unsigned)p->Indx2Units[indx])


#define REF(ptr) Ppmd_GetRef(p, ptr)

#define STATS_REF(ptr) ((CPpmd_State_Ref)REF(ptr))

#define CTX(ref) ((CPpmd8_Context *)Ppmd8_GetContext(p, ref))
#define STATS(ctx) Ppmd8_GetStats(p, ctx)
#define ONE_STATE(ctx) Ppmd8Context_OneState(ctx)
#define SUFFIX(ctx) CTX((ctx)->Suffix)

typedef CPpmd8_Context * PPMD8_CTX_PTR;

struct CPpmd8_Node_;

typedef Ppmd_Ref_Type(struct CPpmd8_Node_) CPpmd8_Node_Ref;

typedef struct CPpmd8_Node_
{
  UInt32 Stamp;
  
  CPpmd8_Node_Ref Next;
  UInt32 NU;
} CPpmd8_Node;

#define NODE(r)  Ppmd_GetPtr_Type(p, r, CPpmd8_Node)

void Ppmd8_Construct(CPpmd8 *p)
{
  unsigned i, k, m;

  p->Base = NULL;

  for (i = 0, k = 0; i < PPMD_NUM_INDEXES; i++)
  {
    unsigned step = (i >= 12 ? 4 : (i >> 2) + 1);
    do { p->Units2Indx[k++] = (Byte)i; } while (--step);
    p->Indx2Units[i] = (Byte)k;
  }

  p->NS2BSIndx[0] = (0 << 1);
  p->NS2BSIndx[1] = (1 << 1);
  memset(p->NS2BSIndx + 2, (2 << 1), 9);
  memset(p->NS2BSIndx + 11, (3 << 1), 256 - 11);

  for (i = 0; i < 5; i++)
    p->NS2Indx[i] = (Byte)i;
  
  for (m = i, k = 1; i < 260; i++)
  {
    p->NS2Indx[i] = (Byte)m;
    if (--k == 0)
      k = (++m) - 4;
  }

  memcpy(p->ExpEscape, PPMD8_kExpEscape, 16);
}


void Ppmd8_Free(CPpmd8 *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->Base);
  p->Size = 0;
  p->Base = NULL;
}


BoolInt Ppmd8_Alloc(CPpmd8 *p, UInt32 size, ISzAllocPtr alloc)
{
  if (!p->Base || p->Size != size)
  {
    Ppmd8_Free(p, alloc);
    p->AlignOffset = (4 - size) & 3;
    if ((p->Base = (Byte *)ISzAlloc_Alloc(alloc, p->AlignOffset + size)) == NULL)
      return SZ_False;
    p->Size = size;
  }
  return SZ_True;
}



// ---------- Internal Memory Allocator ----------






#define EMPTY_NODE 0xFFFFFFFF


static void Ppmd8_InsertNode(CPpmd8 *p, void *node, unsigned indx)
{
  ((CPpmd8_Node *)node)->Stamp = EMPTY_NODE;
  ((CPpmd8_Node *)node)->Next = (CPpmd8_Node_Ref)p->FreeList[indx];
  ((CPpmd8_Node *)node)->NU = I2U(indx);
  p->FreeList[indx] = REF(node);
  p->Stamps[indx]++;
}


static void *Ppmd8_RemoveNode(CPpmd8 *p, unsigned indx)
{
  CPpmd8_Node *node = NODE((CPpmd8_Node_Ref)p->FreeList[indx]);
  p->FreeList[indx] = node->Next;
  p->Stamps[indx]--;

  return node;
}


static void Ppmd8_SplitBlock(CPpmd8 *p, void *ptr, unsigned oldIndx, unsigned newIndx)
{
  unsigned i, nu = I2U(oldIndx) - I2U(newIndx);
  ptr = (Byte *)ptr + U2B(I2U(newIndx));
  if (I2U(i = U2I(nu)) != nu)
  {
    unsigned k = I2U(--i);
    Ppmd8_InsertNode(p, ((Byte *)ptr) + U2B(k), nu - k - 1);
  }
  Ppmd8_InsertNode(p, ptr, i);
}














static void Ppmd8_GlueFreeBlocks(CPpmd8 *p)
{
  /*
  we use first UInt32 field of 12-bytes UNITs as record type stamp
    CPpmd_State    { Byte Symbol; Byte Freq; : Freq != 0xFF
    CPpmd8_Context { Byte NumStats; Byte Flags; UInt16 SummFreq;  : Flags != 0xFF ???
    CPpmd8_Node    { UInt32 Stamp            : Stamp == 0xFFFFFFFF for free record
                                             : Stamp == 0 for guard
    Last 12-bytes UNIT in array is always contains 12-bytes order-0 CPpmd8_Context record
  */
  CPpmd8_Node_Ref n;

  p->GlueCount = 1 << 13;
  memset(p->Stamps, 0, sizeof(p->Stamps));
  
  /* we set guard NODE at LoUnit */
  if (p->LoUnit != p->HiUnit)
    ((CPpmd8_Node *)(void *)p->LoUnit)->Stamp = 0;

  {
    /* Glue free blocks */
    CPpmd8_Node_Ref *prev = &n;
    unsigned i;
    for (i = 0; i < PPMD_NUM_INDEXES; i++)
    {

      CPpmd8_Node_Ref next = (CPpmd8_Node_Ref)p->FreeList[i];
      p->FreeList[i] = 0;
      while (next != 0)
      {
        CPpmd8_Node *node = NODE(next);
        UInt32 nu = node->NU;
        *prev = next;
        next = node->Next;
        if (nu != 0)
        {
          CPpmd8_Node *node2;
          prev = &(node->Next);
          while ((node2 = node + nu)->Stamp == EMPTY_NODE)
          {
            nu += node2->NU;
            node2->NU = 0;
            node->NU = nu;
          }
        }
      }
    }

    *prev = 0;
  }



  






  

  
  
  
  
  
  
  
  
  /* Fill lists of free blocks */
  while (n != 0)
  {
    CPpmd8_Node *node = NODE(n);
    UInt32 nu = node->NU;
    unsigned i;
    n = node->Next;
    if (nu == 0)
      continue;
    for (; nu > 128; nu -= 128, node += 128)
      Ppmd8_InsertNode(p, node, PPMD_NUM_INDEXES - 1);
    if (I2U(i = U2I(nu)) != nu)
    {
      unsigned k = I2U(--i);
      Ppmd8_InsertNode(p, node + k, (unsigned)nu - k - 1);
    }
    Ppmd8_InsertNode(p, node, i);
  }
}


Z7_NO_INLINE
static void *Ppmd8_AllocUnitsRare(CPpmd8 *p, unsigned indx)
{
  unsigned i;
  
  if (p->GlueCount == 0)
  {
    Ppmd8_GlueFreeBlocks(p);
    if (p->FreeList[indx] != 0)
      return Ppmd8_RemoveNode(p, indx);
  }
  
  i = indx;
  
  do
  {
    if (++i == PPMD_NUM_INDEXES)
    {
      UInt32 numBytes = U2B(I2U(indx));
      Byte *us = p->UnitsStart;
      p->GlueCount--;
      return ((UInt32)(us - p->Text) > numBytes) ? (p->UnitsStart = us - numBytes) : (NULL);
    }
  }
  while (p->FreeList[i] == 0);
  
  {
    void *block = Ppmd8_RemoveNode(p, i);
    Ppmd8_SplitBlock(p, block, i, indx);
    return block;
  }
}


static void *Ppmd8_AllocUnits(CPpmd8 *p, unsigned indx)
{
  if (p->FreeList[indx] != 0)
    return Ppmd8_RemoveNode(p, indx);
  {
    UInt32 numBytes = U2B(I2U(indx));
    Byte *lo = p->LoUnit;
    if ((UInt32)(p->HiUnit - lo) >= numBytes)
    {
      p->LoUnit = lo + numBytes;
      return lo;
    }
  }
  return Ppmd8_AllocUnitsRare(p, indx);
}


#define MEM_12_CPY(dest, src, num) \
  { UInt32 *d = (UInt32 *)(dest); \
    const UInt32 *z = (const UInt32 *)(src); \
    unsigned n = (num); \
    do { \
      d[0] = z[0]; \
      d[1] = z[1]; \
      d[2] = z[2]; \
      z += 3; \
      d += 3; \
    } while (--n); \
  }



static void *ShrinkUnits(CPpmd8 *p, void *oldPtr, unsigned oldNU, unsigned newNU)
{
  unsigned i0 = U2I(oldNU);
  unsigned i1 = U2I(newNU);
  if (i0 == i1)
    return oldPtr;
  if (p->FreeList[i1] != 0)
  {
    void *ptr = Ppmd8_RemoveNode(p, i1);
    MEM_12_CPY(ptr, oldPtr, newNU)
    Ppmd8_InsertNode(p, oldPtr, i0);
    return ptr;
  }
  Ppmd8_SplitBlock(p, oldPtr, i0, i1);
  return oldPtr;
}


static void FreeUnits(CPpmd8 *p, void *ptr, unsigned nu)
{
  Ppmd8_InsertNode(p, ptr, U2I(nu));
}


static void SpecialFreeUnit(CPpmd8 *p, void *ptr)
{
  if ((Byte *)ptr != p->UnitsStart)
    Ppmd8_InsertNode(p, ptr, 0);
  else
  {
    #ifdef PPMD8_FREEZE_SUPPORT
    *(UInt32 *)ptr = EMPTY_NODE; /* it's used for (Flags == 0xFF) check in RemoveBinContexts() */
    #endif
    p->UnitsStart += UNIT_SIZE;
  }
}


/*
static void *MoveUnitsUp(CPpmd8 *p, void *oldPtr, unsigned nu)
{
  unsigned indx = U2I(nu);
  void *ptr;
  if ((Byte *)oldPtr > p->UnitsStart + (1 << 14) || REF(oldPtr) > p->FreeList[indx])
    return oldPtr;
  ptr = Ppmd8_RemoveNode(p, indx);
  MEM_12_CPY(ptr, oldPtr, nu)
  if ((Byte *)oldPtr != p->UnitsStart)
    Ppmd8_InsertNode(p, oldPtr, indx);
  else
    p->UnitsStart += U2B(I2U(indx));
  return ptr;
}
*/

static void ExpandTextArea(CPpmd8 *p)
{
  UInt32 count[PPMD_NUM_INDEXES];
  unsigned i;
 
  memset(count, 0, sizeof(count));
  if (p->LoUnit != p->HiUnit)
    ((CPpmd8_Node *)(void *)p->LoUnit)->Stamp = 0;
  
  {
    CPpmd8_Node *node = (CPpmd8_Node *)(void *)p->UnitsStart;
    while (node->Stamp == EMPTY_NODE)
    {
      UInt32 nu = node->NU;
      node->Stamp = 0;
      count[U2I(nu)]++;
      node += nu;
    }
    p->UnitsStart = (Byte *)node;
  }
  
  for (i = 0; i < PPMD_NUM_INDEXES; i++)
  {
    UInt32 cnt = count[i];
    if (cnt == 0)
      continue;
    {
      CPpmd8_Node_Ref *prev = (CPpmd8_Node_Ref *)&p->FreeList[i];
      CPpmd8_Node_Ref n = *prev;
      p->Stamps[i] -= cnt;
      for (;;)
      {
        CPpmd8_Node *node = NODE(n);
        n = node->Next;
        if (node->Stamp != 0)
        {
          prev = &node->Next;
          continue;
        }
        *prev = n;
        if (--cnt == 0)
          break;
      }
    }
  }
}


#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)
static void Ppmd8State_SetSuccessor(CPpmd_State *p, CPpmd_Void_Ref v)
{
  Ppmd_SET_SUCCESSOR(p, v)
}

#define RESET_TEXT(offs) { p->Text = p->Base + p->AlignOffset + (offs); }

Z7_NO_INLINE
static
void Ppmd8_RestartModel(CPpmd8 *p)
{
  unsigned i, k, m;

  memset(p->FreeList, 0, sizeof(p->FreeList));
  memset(p->Stamps, 0, sizeof(p->Stamps));
  RESET_TEXT(0)
  p->HiUnit = p->Text + p->Size;
  p->LoUnit = p->UnitsStart = p->HiUnit - p->Size / 8 / UNIT_SIZE * 7 * UNIT_SIZE;
  p->GlueCount = 0;

  p->OrderFall = p->MaxOrder;
  p->RunLength = p->InitRL = -(Int32)((p->MaxOrder < 12) ? p->MaxOrder : 12) - 1;
  p->PrevSuccess = 0;

  {
    CPpmd8_Context *mc = (PPMD8_CTX_PTR)(void *)(p->HiUnit -= UNIT_SIZE); /* AllocContext(p); */
    CPpmd_State *s = (CPpmd_State *)p->LoUnit; /* Ppmd8_AllocUnits(p, PPMD_NUM_INDEXES - 1); */
    
    p->LoUnit += U2B(256 / 2);
    p->MaxContext = p->MinContext = mc;
    p->FoundState = s;
    mc->Flags = 0;
    mc->NumStats = 256 - 1;
    mc->Union2.SummFreq = 256 + 1;
    mc->Union4.Stats = REF(s);
    mc->Suffix = 0;

    for (i = 0; i < 256; i++, s++)
    {
      s->Symbol = (Byte)i;
      s->Freq = 1;
      Ppmd8State_SetSuccessor(s, 0);
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  for (i = m = 0; m < 25; m++)
  {
    while (p->NS2Indx[i] == m)
      i++;
    for (k = 0; k < 8; k++)
    {
      unsigned r;
      UInt16 *dest = p->BinSumm[m] + k;
      const UInt16 val = (UInt16)(PPMD_BIN_SCALE - PPMD8_kInitBinEsc[k] / (i + 1));
      for (r = 0; r < 64; r += 8)
        dest[r] = val;
    }
  }

  for (i = m = 0; m < 24; m++)
  {
    unsigned summ;
    CPpmd_See *s;
    while (p->NS2Indx[(size_t)i + 3] == m + 3)
      i++;
    s = p->See[m];
    summ = ((2 * i + 5) << (PPMD_PERIOD_BITS - 4));
    for (k = 0; k < 32; k++, s++)
    {
      s->Summ = (UInt16)summ;
      s->Shift = (PPMD_PERIOD_BITS - 4);
      s->Count = 7;
    }
  }

  p->DummySee.Summ = 0; /* unused */
  p->DummySee.Shift = PPMD_PERIOD_BITS;
  p->DummySee.Count = 64; /* unused */
}


void Ppmd8_Init(CPpmd8 *p, unsigned maxOrder, unsigned restoreMethod)
{
  p->MaxOrder = maxOrder;
  p->RestoreMethod = restoreMethod;
  Ppmd8_RestartModel(p);
}


#define FLAG_RESCALED  (1 << 2)
// #define FLAG_SYM_HIGH  (1 << 3)
#define FLAG_PREV_HIGH (1 << 4)

#define HiBits_Prepare(sym) ((unsigned)(sym) + 0xC0)

#define HiBits_Convert_3(flags) (((flags) >> (8 - 3)) & (1 << 3))
#define HiBits_Convert_4(flags) (((flags) >> (8 - 4)) & (1 << 4))

#define PPMD8_HiBitsFlag_3(sym) HiBits_Convert_3(HiBits_Prepare(sym))
#define PPMD8_HiBitsFlag_4(sym) HiBits_Convert_4(HiBits_Prepare(sym))

// #define PPMD8_HiBitsFlag_3(sym) (0x08 * ((sym) >= 0x40))
// #define PPMD8_HiBitsFlag_4(sym) (0x10 * ((sym) >= 0x40))

/*
Refresh() is called when we remove some symbols (successors) in context.
It increases Escape_Freq for sum of all removed symbols.
*/

static void Refresh(CPpmd8 *p, PPMD8_CTX_PTR ctx, unsigned oldNU, unsigned scale)
{
  unsigned i = ctx->NumStats, escFreq, sumFreq, flags;
  CPpmd_State *s = (CPpmd_State *)ShrinkUnits(p, STATS(ctx), oldNU, (i + 2) >> 1);
  ctx->Union4.Stats = REF(s);

  // #ifdef PPMD8_FREEZE_SUPPORT
  /*
    (ctx->Union2.SummFreq >= ((UInt32)1 << 15)) can be in FREEZE mode for some files.
    It's not good for range coder. So new versions of support fix:
       -   original PPMdI code rev.1
       +   original PPMdI code rev.2
       -   7-Zip default ((PPMD8_FREEZE_SUPPORT is not defined)
       +   7-Zip (p->RestoreMethod >= PPMD8_RESTORE_METHOD_FREEZE)
    if we       use that fixed line, we can lose compatibility with some files created before fix
    if we don't use that fixed line, the program can work incorrectly in FREEZE mode in rare case.
  */
  // if (p->RestoreMethod >= PPMD8_RESTORE_METHOD_FREEZE)
  {
    scale |= (ctx->Union2.SummFreq >= ((UInt32)1 << 15));
  }
  // #endif



  flags = HiBits_Prepare(s->Symbol);
  {
    unsigned freq = s->Freq;
    escFreq = ctx->Union2.SummFreq - freq;
    freq = (freq + scale) >> scale;
    sumFreq = freq;
    s->Freq = (Byte)freq;
  }
 
  do
  {
    unsigned freq = (++s)->Freq;
    escFreq -= freq;
    freq = (freq + scale) >> scale;
    sumFreq += freq;
    s->Freq = (Byte)freq;
    flags |= HiBits_Prepare(s->Symbol);
  }
  while (--i);
  
  ctx->Union2.SummFreq = (UInt16)(sumFreq + ((escFreq + scale) >> scale));
  ctx->Flags = (Byte)((ctx->Flags & (FLAG_PREV_HIGH + FLAG_RESCALED * scale)) + HiBits_Convert_3(flags));
}


static void SWAP_STATES(CPpmd_State *t1, CPpmd_State *t2)
{
  CPpmd_State tmp = *t1;
  *t1 = *t2;
  *t2 = tmp;
}


/*
CutOff() reduces contexts:
  It conversts Successors at MaxOrder to another Contexts to NULL-Successors
  It removes RAW-Successors and NULL-Successors that are not Order-0
      and it removes contexts when it has no Successors.
  if the (Union4.Stats) is close to (UnitsStart), it moves it up.
*/

static CPpmd_Void_Ref CutOff(CPpmd8 *p, PPMD8_CTX_PTR ctx, unsigned order)
{
  int ns = ctx->NumStats;
  unsigned nu;
  CPpmd_State *stats;
  
  if (ns == 0)
  {
    CPpmd_State *s = ONE_STATE(ctx);
    CPpmd_Void_Ref successor = SUCCESSOR(s);
    if ((Byte *)Ppmd8_GetPtr(p, successor) >= p->UnitsStart)
    {
      if (order < p->MaxOrder)
        successor = CutOff(p, CTX(successor), order + 1);
      else
        successor = 0;
      Ppmd8State_SetSuccessor(s, successor);
      if (successor || order <= 9) /* O_BOUND */
        return REF(ctx);
    }
    SpecialFreeUnit(p, ctx);
    return 0;
  }

  nu = ((unsigned)ns + 2) >> 1;
  // ctx->Union4.Stats = STATS_REF(MoveUnitsUp(p, STATS(ctx), nu));
  {
    unsigned indx = U2I(nu);
    stats = STATS(ctx);

    if ((UInt32)((Byte *)stats - p->UnitsStart) <= (1 << 14)
        && (CPpmd_Void_Ref)ctx->Union4.Stats <= p->FreeList[indx])
    {
      void *ptr = Ppmd8_RemoveNode(p, indx);
      ctx->Union4.Stats = STATS_REF(ptr);
      MEM_12_CPY(ptr, (const void *)stats, nu)
      if ((Byte *)stats != p->UnitsStart)
        Ppmd8_InsertNode(p, stats, indx);
      else
        p->UnitsStart += U2B(I2U(indx));
#ifdef __cplusplus
      stats = (CPpmd_State *)ptr;
#else
      stats = ptr;
#endif
    }
  }

  {
    CPpmd_State *s = stats + (unsigned)ns;
    do
    {
      CPpmd_Void_Ref successor = SUCCESSOR(s);
      if ((Byte *)Ppmd8_GetPtr(p, successor) < p->UnitsStart)
      {
        CPpmd_State *s2 = stats + (unsigned)(ns--);
        if (order)
        {
          if (s != s2)
            *s = *s2;
        }
        else
        {
          SWAP_STATES(s, s2);
          Ppmd8State_SetSuccessor(s2, 0);
        }
      }
      else
      {
        if (order < p->MaxOrder)
          Ppmd8State_SetSuccessor(s, CutOff(p, CTX(successor), order + 1));
        else
          Ppmd8State_SetSuccessor(s, 0);
      }
    }
    while (--s >= stats);
  }
  
  if (ns != ctx->NumStats && order)
  {
    if (ns < 0)
    {
      FreeUnits(p, stats, nu);
      SpecialFreeUnit(p, ctx);
      return 0;
    }
    ctx->NumStats = (Byte)ns;
    if (ns == 0)
    {
      const Byte sym = stats->Symbol;
      ctx->Flags = (Byte)((ctx->Flags & FLAG_PREV_HIGH) + PPMD8_HiBitsFlag_3(sym));
      // *ONE_STATE(ctx) = *stats;
      ctx->Union2.State2.Symbol = sym;
      ctx->Union2.State2.Freq = (Byte)(((unsigned)stats->Freq + 11) >> 3);
      ctx->Union4.State4.Successor_0 = stats->Successor_0;
      ctx->Union4.State4.Successor_1 = stats->Successor_1;
      FreeUnits(p, stats, nu);
    }
    else
    {
      Refresh(p, ctx, nu, ctx->Union2.SummFreq > 16 * (unsigned)ns);
    }
  }
  
  return REF(ctx);
}



#ifdef PPMD8_FREEZE_SUPPORT

/*
RemoveBinContexts()
  It conversts Successors at MaxOrder to another Contexts to NULL-Successors
  It changes RAW-Successors to NULL-Successors
  removes Bin Context without Successor, if suffix of that context is also binary.
*/

static CPpmd_Void_Ref RemoveBinContexts(CPpmd8 *p, PPMD8_CTX_PTR ctx, unsigned order)
{
  if (!ctx->NumStats)
  {
    CPpmd_State *s = ONE_STATE(ctx);
    CPpmd_Void_Ref successor = SUCCESSOR(s);
    if ((Byte *)Ppmd8_GetPtr(p, successor) >= p->UnitsStart && order < p->MaxOrder)
      successor = RemoveBinContexts(p, CTX(successor), order + 1);
    else
      successor = 0;
    Ppmd8State_SetSuccessor(s, successor);
    /* Suffix context can be removed already, since different (high-order)
       Successors may refer to same context. So we check Flags == 0xFF (Stamp == EMPTY_NODE) */
    if (!successor && (!SUFFIX(ctx)->NumStats || SUFFIX(ctx)->Flags == 0xFF))
    {
      FreeUnits(p, ctx, 1);
      return 0;
    }
  }
  else
  {
    CPpmd_State *s = STATS(ctx) + ctx->NumStats;
    do
    {
      CPpmd_Void_Ref successor = SUCCESSOR(s);
      if ((Byte *)Ppmd8_GetPtr(p, successor) >= p->UnitsStart && order < p->MaxOrder)
        Ppmd8State_SetSuccessor(s, RemoveBinContexts(p, CTX(successor), order + 1));
      else
        Ppmd8State_SetSuccessor(s, 0);
    }
    while (--s >= STATS(ctx));
  }
  
  return REF(ctx);
}

#endif



static UInt32 GetUsedMemory(const CPpmd8 *p)
{
  UInt32 v = 0;
  unsigned i;
  for (i = 0; i < PPMD_NUM_INDEXES; i++)
    v += p->Stamps[i] * I2U(i);
  return p->Size - (UInt32)(p->HiUnit - p->LoUnit) - (UInt32)(p->UnitsStart - p->Text) - U2B(v);
}

#ifdef PPMD8_FREEZE_SUPPORT
  #define RESTORE_MODEL(c1, fSuccessor) RestoreModel(p, c1, fSuccessor)
#else
  #define RESTORE_MODEL(c1, fSuccessor) RestoreModel(p, c1)
#endif


static void RestoreModel(CPpmd8 *p, PPMD8_CTX_PTR ctxError
    #ifdef PPMD8_FREEZE_SUPPORT
    , PPMD8_CTX_PTR fSuccessor
    #endif
    )
{
  PPMD8_CTX_PTR c;
  CPpmd_State *s;
  RESET_TEXT(0)

  // we go here in cases of error of allocation for context (c1)
  // Order(MinContext) < Order(ctxError) <= Order(MaxContext)
  
  // We remove last symbol from each of contexts [p->MaxContext ... ctxError) contexts
  // So we rollback all created (symbols) before error.
  for (c = p->MaxContext; c != ctxError; c = SUFFIX(c))
    if (--(c->NumStats) == 0)
    {
      s = STATS(c);
      c->Flags = (Byte)((c->Flags & FLAG_PREV_HIGH) + PPMD8_HiBitsFlag_3(s->Symbol));
      // *ONE_STATE(c) = *s;
      c->Union2.State2.Symbol = s->Symbol;
      c->Union2.State2.Freq = (Byte)(((unsigned)s->Freq + 11) >> 3);
      c->Union4.State4.Successor_0 = s->Successor_0;
      c->Union4.State4.Successor_1 = s->Successor_1;

      SpecialFreeUnit(p, s);
    }
    else
    {
      /* Refresh() can increase Escape_Freq on value of Freq of last symbol, that was added before error.
         so the largest possible increase for Escape_Freq is (8) from value before ModelUpoadet() */
      Refresh(p, c, ((unsigned)c->NumStats + 3) >> 1, 0);
    }
 
  // increase Escape Freq for context [ctxError ... p->MinContext)
  for (; c != p->MinContext; c = SUFFIX(c))
    if (c->NumStats == 0)
    {
      // ONE_STATE(c)
      c->Union2.State2.Freq = (Byte)(((unsigned)c->Union2.State2.Freq + 1) >> 1);
    }
    else if ((c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 4)) > 128 + 4 * c->NumStats)
      Refresh(p, c, ((unsigned)c->NumStats + 2) >> 1, 1);

  #ifdef PPMD8_FREEZE_SUPPORT
  if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
  {
    p->MaxContext = fSuccessor;
    p->GlueCount += !(p->Stamps[1] & 1); // why?
  }
  else if (p->RestoreMethod == PPMD8_RESTORE_METHOD_FREEZE)
  {
    while (p->MaxContext->Suffix)
      p->MaxContext = SUFFIX(p->MaxContext);
    RemoveBinContexts(p, p->MaxContext, 0);
    // we change the current mode to (PPMD8_RESTORE_METHOD_FREEZE + 1)
    p->RestoreMethod = PPMD8_RESTORE_METHOD_FREEZE + 1;
    p->GlueCount = 0;
    p->OrderFall = p->MaxOrder;
  }
  else
  #endif
  if (p->RestoreMethod == PPMD8_RESTORE_METHOD_RESTART || GetUsedMemory(p) < (p->Size >> 1))
    Ppmd8_RestartModel(p);
  else
  {
    while (p->MaxContext->Suffix)
      p->MaxContext = SUFFIX(p->MaxContext);
    do
    {
      CutOff(p, p->MaxContext, 0);
      ExpandTextArea(p);
    }
    while (GetUsedMemory(p) > 3 * (p->Size >> 2));
    p->GlueCount = 0;
    p->OrderFall = p->MaxOrder;
  }
  p->MinContext = p->MaxContext;
}



Z7_NO_INLINE
static PPMD8_CTX_PTR Ppmd8_CreateSuccessors(CPpmd8 *p, BoolInt skip, CPpmd_State *s1, PPMD8_CTX_PTR c)
{

  CPpmd_Byte_Ref upBranch = (CPpmd_Byte_Ref)SUCCESSOR(p->FoundState);
  Byte newSym, newFreq, flags;
  unsigned numPs = 0;
  CPpmd_State *ps[PPMD8_MAX_ORDER + 1]; /* fixed over Shkarin's code. Maybe it could work without + 1 too. */
  
  if (!skip)
    ps[numPs++] = p->FoundState;
  
  while (c->Suffix)
  {
    CPpmd_Void_Ref successor;
    CPpmd_State *s;
    c = SUFFIX(c);
    
    if (s1) { s = s1; s1 = NULL; }
    else if (c->NumStats != 0)
    {
      Byte sym = p->FoundState->Symbol;
      for (s = STATS(c); s->Symbol != sym; s++);
      if (s->Freq < MAX_FREQ - 9) { s->Freq++; c->Union2.SummFreq++; }
    }
    else
    {
      s = ONE_STATE(c);
      s->Freq = (Byte)(s->Freq + (!SUFFIX(c)->NumStats & (s->Freq < 24)));
    }
    successor = SUCCESSOR(s);
    if (successor != upBranch)
    {

      c = CTX(successor);
      if (numPs == 0)
      {
        
       
        return c;
      }
      break;
    }
    ps[numPs++] = s;
  }
  
  
  
  
  
  newSym = *(const Byte *)Ppmd8_GetPtr(p, upBranch);
  upBranch++;
  flags = (Byte)(PPMD8_HiBitsFlag_4(p->FoundState->Symbol) + PPMD8_HiBitsFlag_3(newSym));
  
  if (c->NumStats == 0)
    newFreq = c->Union2.State2.Freq;
  else
  {
    UInt32 cf, s0;
    CPpmd_State *s;
    for (s = STATS(c); s->Symbol != newSym; s++);
    cf = (UInt32)s->Freq - 1;
    s0 = (UInt32)c->Union2.SummFreq - c->NumStats - cf;
    /*
    

      max(newFreq)= (s->Freq - 1), when (s0 == 1)


    */
    newFreq = (Byte)(1 + ((2 * cf <= s0) ? (5 * cf > s0) : ((cf + 2 * s0 - 3) / s0)));
  }



  do
  {
    PPMD8_CTX_PTR c1;
    /* = AllocContext(p); */
    if (p->HiUnit != p->LoUnit)
      c1 = (PPMD8_CTX_PTR)(void *)(p->HiUnit -= UNIT_SIZE);
    else if (p->FreeList[0] != 0)
      c1 = (PPMD8_CTX_PTR)Ppmd8_RemoveNode(p, 0);
    else
    {
      c1 = (PPMD8_CTX_PTR)Ppmd8_AllocUnitsRare(p, 0);
      if (!c1)
        return NULL;
    }
    c1->Flags = flags;
    c1->NumStats = 0;
    c1->Union2.State2.Symbol = newSym;
    c1->Union2.State2.Freq = newFreq;
    Ppmd8State_SetSuccessor(ONE_STATE(c1), upBranch);
    c1->Suffix = REF(c);
    Ppmd8State_SetSuccessor(ps[--numPs], REF(c1));
    c = c1;
  }
  while (numPs != 0);
  
  return c;
}


static PPMD8_CTX_PTR ReduceOrder(CPpmd8 *p, CPpmd_State *s1, PPMD8_CTX_PTR c)
{
  CPpmd_State *s = NULL;
  PPMD8_CTX_PTR c1 = c;
  CPpmd_Void_Ref upBranch = REF(p->Text);
  
  #ifdef PPMD8_FREEZE_SUPPORT
  /* The BUG in Shkarin's code was fixed: ps could overflow in CUT_OFF mode. */
  CPpmd_State *ps[PPMD8_MAX_ORDER + 1];
  unsigned numPs = 0;
  ps[numPs++] = p->FoundState;
  #endif

  Ppmd8State_SetSuccessor(p->FoundState, upBranch);
  p->OrderFall++;

  for (;;)
  {
    if (s1)
    {
      c = SUFFIX(c);
      s = s1;
      s1 = NULL;
    }
    else
    {
      if (!c->Suffix)
      {
        #ifdef PPMD8_FREEZE_SUPPORT
        if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
        {
          do { Ppmd8State_SetSuccessor(ps[--numPs], REF(c)); } while (numPs);
          RESET_TEXT(1)
          p->OrderFall = 1;
        }
        #endif
        return c;
      }
      c = SUFFIX(c);
      if (c->NumStats)
      {
        if ((s = STATS(c))->Symbol != p->FoundState->Symbol)
          do { s++; } while (s->Symbol != p->FoundState->Symbol);
        if (s->Freq < MAX_FREQ - 9)
        {
          s->Freq = (Byte)(s->Freq + 2);
          c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 2);
        }
      }
      else
      {
        s = ONE_STATE(c);
        s->Freq = (Byte)(s->Freq + (s->Freq < 32));
      }
    }
    if (SUCCESSOR(s))
      break;
    #ifdef PPMD8_FREEZE_SUPPORT
    ps[numPs++] = s;
    #endif
    Ppmd8State_SetSuccessor(s, upBranch);
    p->OrderFall++;
  }
  
  #ifdef PPMD8_FREEZE_SUPPORT
  if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
  {
    c = CTX(SUCCESSOR(s));
    do { Ppmd8State_SetSuccessor(ps[--numPs], REF(c)); } while (numPs);
    RESET_TEXT(1)
    p->OrderFall = 1;
    return c;
  }
  else
  #endif
  if (SUCCESSOR(s) <= upBranch)
  {
    PPMD8_CTX_PTR successor;
    CPpmd_State *s2 = p->FoundState;
    p->FoundState = s;

    successor = Ppmd8_CreateSuccessors(p, SZ_False, NULL, c);
    if (!successor)
      Ppmd8State_SetSuccessor(s, 0);
    else
      Ppmd8State_SetSuccessor(s, REF(successor));
    p->FoundState = s2;
  }
  
  {
    CPpmd_Void_Ref successor = SUCCESSOR(s);
    if (p->OrderFall == 1 && c1 == p->MaxContext)
    {
      Ppmd8State_SetSuccessor(p->FoundState, successor);
      p->Text--;
    }
    if (successor == 0)
      return NULL;
    return CTX(successor);
  }
}



void Ppmd8_UpdateModel(CPpmd8 *p);
Z7_NO_INLINE
void Ppmd8_UpdateModel(CPpmd8 *p)
{
  CPpmd_Void_Ref maxSuccessor, minSuccessor = SUCCESSOR(p->FoundState);
  PPMD8_CTX_PTR c;
  unsigned s0, ns, fFreq = p->FoundState->Freq;
  Byte flag, fSymbol = p->FoundState->Symbol;
  {
  CPpmd_State *s = NULL;
  if (p->FoundState->Freq < MAX_FREQ / 4 && p->MinContext->Suffix != 0)
  {
    /* Update Freqs in Suffix Context */

    c = SUFFIX(p->MinContext);
    
    if (c->NumStats == 0)
    {
      s = ONE_STATE(c);
      if (s->Freq < 32)
        s->Freq++;
    }
    else
    {
      Byte sym = p->FoundState->Symbol;
      s = STATS(c);

      if (s->Symbol != sym)
      {
        do
        {
        
          s++;
        }
        while (s->Symbol != sym);
        
        if (s[0].Freq >= s[-1].Freq)
        {
          SWAP_STATES(&s[0], &s[-1]);
          s--;
        }
      }
      
      if (s->Freq < MAX_FREQ - 9)
      {
        s->Freq = (Byte)(s->Freq + 2);
        c->Union2.SummFreq = (UInt16)(c->Union2.SummFreq + 2);
      }
    }
  }
  
  c = p->MaxContext;
  if (p->OrderFall == 0 && minSuccessor)
  {
    PPMD8_CTX_PTR cs = Ppmd8_CreateSuccessors(p, SZ_True, s, p->MinContext);
    if (!cs)
    {
      Ppmd8State_SetSuccessor(p->FoundState, 0);
      RESTORE_MODEL(c, CTX(minSuccessor));
      return;
    }
    Ppmd8State_SetSuccessor(p->FoundState, REF(cs));
    p->MinContext = p->MaxContext = cs;
    return;
  }
  



  {
    Byte *text = p->Text;
    *text++ = p->FoundState->Symbol;
    p->Text = text;
    if (text >= p->UnitsStart)
    {
      RESTORE_MODEL(c, CTX(minSuccessor)); /* check it */
      return;
    }
    maxSuccessor = REF(text);
  }

  if (!minSuccessor)
  {
    PPMD8_CTX_PTR cs = ReduceOrder(p, s, p->MinContext);
    if (!cs)
    {
      RESTORE_MODEL(c, NULL);
      return;
    }
    minSuccessor = REF(cs);
  }
  else if ((Byte *)Ppmd8_GetPtr(p, minSuccessor) < p->UnitsStart)
  {
    PPMD8_CTX_PTR cs = Ppmd8_CreateSuccessors(p, SZ_False, s, p->MinContext);
    if (!cs)
    {
      RESTORE_MODEL(c, NULL);
      return;
    }
    minSuccessor = REF(cs);
  }
  
  if (--p->OrderFall == 0)
  {
    maxSuccessor = minSuccessor;
    p->Text -= (p->MaxContext != p->MinContext);
  }
  #ifdef PPMD8_FREEZE_SUPPORT
  else if (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE)
  {
    maxSuccessor = minSuccessor;
    RESET_TEXT(0)
    p->OrderFall = 0;
  }
  #endif
  }
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  flag = (Byte)(PPMD8_HiBitsFlag_3(fSymbol));
  s0 = p->MinContext->Union2.SummFreq - (ns = p->MinContext->NumStats) - fFreq;
  
  for (; c != p->MinContext; c = SUFFIX(c))
  {
    unsigned ns1;
    UInt32 sum;
    
    if ((ns1 = c->NumStats) != 0)
    {
      if ((ns1 & 1) != 0)
      {
        /* Expand for one UNIT */
        const unsigned oldNU = (ns1 + 1) >> 1;
        const unsigned i = U2I(oldNU);
        if (i != U2I((size_t)oldNU + 1))
        {
          void *ptr = Ppmd8_AllocUnits(p, i + 1);
          void *oldPtr;
          if (!ptr)
          {
            RESTORE_MODEL(c, CTX(minSuccessor));
            return;
          }
          oldPtr = STATS(c);
          MEM_12_CPY(ptr, oldPtr, oldNU)
          Ppmd8_InsertNode(p, oldPtr, i);
          c->Union4.Stats = STATS_REF(ptr);
        }
      }
      sum = c->Union2.SummFreq;
      /* max increase of Escape_Freq is 1 here.
         an average increase is 1/3 per symbol */
      sum += (UInt32)(unsigned)(3 * ns1 + 1 < ns);
      /* original PPMdH uses 16-bit variable for (sum) here.
         But (sum < ???). Do we need to truncate (sum) to 16-bit */
      // sum = (UInt16)sum;
    }
    else
    {
      
      CPpmd_State *s = (CPpmd_State*)Ppmd8_AllocUnits(p, 0);
      if (!s)
      {
        RESTORE_MODEL(c, CTX(minSuccessor));
        return;
      }
      {
        unsigned freq = c->Union2.State2.Freq;
        // s = *ONE_STATE(c);
        s->Symbol = c->Union2.State2.Symbol;
        s->Successor_0 = c->Union4.State4.Successor_0;
        s->Successor_1 = c->Union4.State4.Successor_1;
        // Ppmd8State_SetSuccessor(s, c->Union4.Stats);  // call it only for debug purposes to check the order of
                                              // (Successor_0 and Successor_1) in LE/BE.
        c->Union4.Stats = REF(s);
        if (freq < MAX_FREQ / 4 - 1)
          freq <<= 1;
        else
          freq = MAX_FREQ - 4;

        s->Freq = (Byte)freq;

        sum = (UInt32)(freq + p->InitEsc + (ns > 2));   // Ppmd8 (> 2)
      }
    }

    {
      CPpmd_State *s = STATS(c) + ns1 + 1;
      UInt32 cf = 2 * (sum + 6) * (UInt32)fFreq;
      UInt32 sf = (UInt32)s0 + sum;
      s->Symbol = fSymbol;
      c->NumStats = (Byte)(ns1 + 1);
      Ppmd8State_SetSuccessor(s, maxSuccessor);
      c->Flags |= flag;
      if (cf < 6 * sf)
      {
        cf = (unsigned)1 + (cf > sf) + (cf >= 4 * sf);
        sum += 4;
        /* It can add (1, 2, 3) to Escape_Freq */
      }
      else
      {
        cf = (unsigned)4 + (cf > 9 * sf) + (cf > 12 * sf) + (cf > 15 * sf);
        sum += cf;
      }

      c->Union2.SummFreq = (UInt16)sum;
      s->Freq = (Byte)cf;
    }

  }
  p->MaxContext = p->MinContext = CTX(minSuccessor);
}
  


Z7_NO_INLINE
static void Ppmd8_Rescale(CPpmd8 *p)
{
  unsigned i, adder, sumFreq, escFreq;
  CPpmd_State *stats = STATS(p->MinContext);
  CPpmd_State *s = p->FoundState;

  /* Sort the list by Freq */
  if (s != stats)
  {
    CPpmd_State tmp = *s;
    do
      s[0] = s[-1];
    while (--s != stats);
    *s = tmp;
  }

  sumFreq = s->Freq;
  escFreq = p->MinContext->Union2.SummFreq - sumFreq;


  
  
  
  
  adder = (p->OrderFall != 0);
  
  #ifdef PPMD8_FREEZE_SUPPORT
  adder |= (p->RestoreMethod > PPMD8_RESTORE_METHOD_FREEZE);
  #endif
  
  sumFreq = (sumFreq + 4 + adder) >> 1;
  i = p->MinContext->NumStats;
  s->Freq = (Byte)sumFreq;
  
  do
  {
    unsigned freq = (++s)->Freq;
    escFreq -= freq;
    freq = (freq + adder) >> 1;
    sumFreq += freq;
    s->Freq = (Byte)freq;
    if (freq > s[-1].Freq)
    {
      CPpmd_State tmp = *s;
      CPpmd_State *s1 = s;
      do
      {
        s1[0] = s1[-1];
      }
      while (--s1 != stats && freq > s1[-1].Freq);
      *s1 = tmp;
    }
  }
  while (--i);
  
  if (s->Freq == 0)
  {
    /* Remove all items with Freq == 0 */
    CPpmd8_Context *mc;
    unsigned numStats, numStatsNew, n0, n1;
    
    i = 0; do { i++; } while ((--s)->Freq == 0);
    
    

    
    escFreq += i;
    mc = p->MinContext;
    numStats = mc->NumStats;
    numStatsNew = numStats - i;
    mc->NumStats = (Byte)(numStatsNew);
    n0 = (numStats + 2) >> 1;
    
    if (numStatsNew == 0)
    {
    
      unsigned freq = (2 * (unsigned)stats->Freq + escFreq - 1) / escFreq;
      if (freq > MAX_FREQ / 3)
        freq = MAX_FREQ / 3;
      mc->Flags = (Byte)((mc->Flags & FLAG_PREV_HIGH) + PPMD8_HiBitsFlag_3(stats->Symbol));
      
      
      
      
      
      s = ONE_STATE(mc);
      *s = *stats;
      s->Freq = (Byte)freq;
      p->FoundState = s;
      Ppmd8_InsertNode(p, stats, U2I(n0));
      return;
    }

    n1 = (numStatsNew + 2) >> 1;
    if (n0 != n1)
      mc->Union4.Stats = STATS_REF(ShrinkUnits(p, stats, n0, n1));
    {
      // here we are for max order only. So Ppmd8_MakeEscFreq() doesn't use mc->Flags
      // but we still need current (Flags & FLAG_PREV_HIGH), if we will convert context to 1-symbol context later.
      /*
      unsigned flags = HiBits_Prepare((s = STATS(mc))->Symbol);
      i = mc->NumStats;
      do { flags |= HiBits_Prepare((++s)->Symbol); } while (--i);
      mc->Flags = (Byte)((mc->Flags & ~FLAG_SYM_HIGH) + HiBits_Convert_3(flags));
      */
    }
  }



  


  {
    CPpmd8_Context *mc = p->MinContext;
    mc->Union2.SummFreq = (UInt16)(sumFreq + escFreq - (escFreq >> 1));
    mc->Flags |= FLAG_RESCALED;
    p->FoundState = STATS(mc);
  }
}


CPpmd_See *Ppmd8_MakeEscFreq(CPpmd8 *p, unsigned numMasked1, UInt32 *escFreq)
{
  CPpmd_See *see;
  const CPpmd8_Context *mc = p->MinContext;
  unsigned numStats = mc->NumStats;
  if (numStats != 0xFF)
  {
    // (3 <= numStats + 2 <= 256)   (3 <= NS2Indx[3] and NS2Indx[256] === 26)
    see = p->See[(size_t)(unsigned)p->NS2Indx[(size_t)numStats + 2] - 3]
        + (mc->Union2.SummFreq > 11 * (numStats + 1))
        + 2 * (unsigned)(2 * numStats < ((unsigned)SUFFIX(mc)->NumStats + numMasked1))
        + mc->Flags;

    {
      // if (see->Summ) field is larger than 16-bit, we need only low 16 bits of Summ
      const unsigned summ = (UInt16)see->Summ; // & 0xFFFF
      const unsigned r = (summ >> see->Shift);
      see->Summ = (UInt16)(summ - r);
      *escFreq = (UInt32)(r + (r == 0));
    }
  }
  else
  {
    see = &p->DummySee;
    *escFreq = 1;
  }
  return see;
}

 
static void Ppmd8_NextContext(CPpmd8 *p)
{
  PPMD8_CTX_PTR c = CTX(SUCCESSOR(p->FoundState));
  if (p->OrderFall == 0 && (const Byte *)c >= p->UnitsStart)
    p->MaxContext = p->MinContext = c;
  else
    Ppmd8_UpdateModel(p);
}
 

void Ppmd8_Update1(CPpmd8 *p)
{
  CPpmd_State *s = p->FoundState;
  unsigned freq = s->Freq;
  freq += 4;
  p->MinContext->Union2.SummFreq = (UInt16)(p->MinContext->Union2.SummFreq + 4);
  s->Freq = (Byte)freq;
  if (freq > s[-1].Freq)
  {
    SWAP_STATES(s, &s[-1]);
    p->FoundState = --s;
    if (freq > MAX_FREQ)
      Ppmd8_Rescale(p);
  }
  Ppmd8_NextContext(p);
}


void Ppmd8_Update1_0(CPpmd8 *p)
{
  CPpmd_State *s = p->FoundState;
  CPpmd8_Context *mc = p->MinContext;
  unsigned freq = s->Freq;
  const unsigned summFreq = mc->Union2.SummFreq;
  p->PrevSuccess = (2 * freq >= summFreq); // Ppmd8 (>=)
  p->RunLength += (Int32)p->PrevSuccess;
  mc->Union2.SummFreq = (UInt16)(summFreq + 4);
  freq += 4;
  s->Freq = (Byte)freq;
  if (freq > MAX_FREQ)
    Ppmd8_Rescale(p);
  Ppmd8_NextContext(p);
}


/*
void Ppmd8_UpdateBin(CPpmd8 *p)
{
  unsigned freq = p->FoundState->Freq;
  p->FoundState->Freq = (Byte)(freq + (freq < 196)); // Ppmd8 (196)
  p->PrevSuccess = 1;
  p->RunLength++;
  Ppmd8_NextContext(p);
}
*/

void Ppmd8_Update2(CPpmd8 *p)
{
  CPpmd_State *s = p->FoundState;
  unsigned freq = s->Freq;
  freq += 4;
  p->RunLength = p->InitRL;
  p->MinContext->Union2.SummFreq = (UInt16)(p->MinContext->Union2.SummFreq + 4);
  s->Freq = (Byte)freq;
  if (freq > MAX_FREQ)
    Ppmd8_Rescale(p);
  Ppmd8_UpdateModel(p);
}

/* H->I changes:
  NS2Indx
  GlueCount, and Glue method
  BinSum
  See / EscFreq
  Ppmd8_CreateSuccessors updates more suffix contexts
  Ppmd8_UpdateModel consts.
  PrevSuccess Update

Flags:
  (1 << 2) - the Context was Rescaled
  (1 << 3) - there is symbol in Stats with (sym >= 0x40) in
  (1 << 4) - main symbol of context is (sym >= 0x40)
*/

#undef RESET_TEXT
#undef FLAG_RESCALED
#undef FLAG_PREV_HIGH
#undef HiBits_Prepare
#undef HiBits_Convert_3
#undef HiBits_Convert_4
#undef PPMD8_HiBitsFlag_3
#undef PPMD8_HiBitsFlag_4
#undef RESTORE_MODEL

#undef MAX_FREQ
#undef UNIT_SIZE
#undef U2B
#undef U2I
#undef I2U

#undef REF
#undef STATS_REF
#undef CTX
#undef STATS
#undef ONE_STATE
#undef SUFFIX
#undef NODE
#undef EMPTY_NODE
#undef MEM_12_CPY
#undef SUCCESSOR
#undef SWAP_STATES

/* Ppmd8Dec.c -- Ppmd8 (PPMdI) Decoder
2023-09-07 : Igor Pavlov : Public domain
This code is based on:
  PPMd var.I (2002): Dmitry Shkarin : Public domain
  Carryless rangecoder (1999): Dmitry Subbotin : Public domain */
#define kTop ((UInt32)1 << 24)
#define kBot ((UInt32)1 << 15)

#define READ_BYTE(p) IByteIn_Read((p)->Stream.In)

BoolInt Ppmd8_Init_RangeDec(CPpmd8 *p)
{
  unsigned i;
  p->Code = 0;
  p->Range = 0xFFFFFFFF;
  p->Low = 0;
  
  for (i = 0; i < 4; i++)
    p->Code = (p->Code << 8) | READ_BYTE(p);
  return (p->Code < 0xFFFFFFFF);
}

#define RC_NORM(p) \
  while ((p->Low ^ (p->Low + p->Range)) < kTop \
    || (p->Range < kBot && ((p->Range = (0 - p->Low) & (kBot - 1)), 1))) { \
      p->Code = (p->Code << 8) | READ_BYTE(p); \
      p->Range <<= 8; p->Low <<= 8; }

// we must use only one type of Normalization from two: LOCAL or REMOTE
#define RC_NORM_LOCAL(p)    // RC_NORM(p)
#define RC_NORM_REMOTE(p)   RC_NORM(p)

#define R p

Z7_FORCE_INLINE
// Z7_NO_INLINE
static void Ppmd8_RD_Decode(CPpmd8 *p, UInt32 start, UInt32 size)
{
  start *= R->Range;
  R->Low += start;
  R->Code -= start;
  R->Range *= size;
  RC_NORM_LOCAL(R)
}

#define RC_Decode(start, size)  Ppmd8_RD_Decode(p, start, size);
#define RC_DecodeFinal(start, size)  RC_Decode(start, size)  RC_NORM_REMOTE(R)
#define RC_GetThreshold(total)  (R->Code / (R->Range /= (total)))


#define CTX(ref) ((CPpmd8_Context *)Ppmd8_GetContext(p, ref))
// typedef CPpmd8_Context * CTX_PTR;
#define SUCCESSOR(p) Ppmd_GET_SUCCESSOR(p)
void Ppmd8_UpdateModel(CPpmd8 *p);

#define MASK(sym)  ((Byte *)charMask)[sym]


int Ppmd8_DecodeSymbol(CPpmd8 *p)
{
  size_t charMask[256 / sizeof(size_t)];

  if (p->MinContext->NumStats != 0)
  {
    CPpmd_State *s = Ppmd8_GetStats(p, p->MinContext);
    unsigned i;
    UInt32 count, hiCnt;
    UInt32 summFreq = p->MinContext->Union2.SummFreq;

    PPMD8_CORRECT_SUM_RANGE(p, summFreq)


    count = RC_GetThreshold(summFreq);
    hiCnt = count;
    
    if ((Int32)(count -= s->Freq) < 0)
    {
      Byte sym;
      RC_DecodeFinal(0, s->Freq)
      p->FoundState = s;
      sym = s->Symbol;
      Ppmd8_Update1_0(p);
      return sym;
    }
    
    p->PrevSuccess = 0;
    i = p->MinContext->NumStats;
    
    do
    {
      if ((Int32)(count -= (++s)->Freq) < 0)
      {
        Byte sym;
        RC_DecodeFinal((hiCnt - count) - s->Freq, s->Freq)
        p->FoundState = s;
        sym = s->Symbol;
        Ppmd8_Update1(p);
        return sym;
      }
    }
    while (--i);
    
    if (hiCnt >= summFreq)
      return PPMD8_SYM_ERROR;

    hiCnt -= count;
    RC_Decode(hiCnt, summFreq - hiCnt)
    
    
    PPMD_SetAllBitsIn256Bytes(charMask)
    // i = p->MinContext->NumStats - 1;
    // do { MASK((--s)->Symbol) = 0; } while (--i);
    {
      CPpmd_State *s2 = Ppmd8_GetStats(p, p->MinContext);
      MASK(s->Symbol) = 0;
      do
      {
        const unsigned sym0 = s2[0].Symbol;
        const unsigned sym1 = s2[1].Symbol;
        s2 += 2;
        MASK(sym0) = 0;
        MASK(sym1) = 0;
      }
      while (s2 < s);
    }
  }
  else
  {
    CPpmd_State *s = Ppmd8Context_OneState(p->MinContext);
    UInt16 *prob = Ppmd8_GetBinSumm(p);
    UInt32 pr = *prob;
    UInt32 size0 = (R->Range >> 14) * pr;
    pr = PPMD_UPDATE_PROB_1(pr);
    
    if (R->Code < size0)
    {
      Byte sym;
      *prob = (UInt16)(pr + (1 << PPMD_INT_BITS));
      
      // RangeDec_DecodeBit0(size0);
      R->Range = size0;
      RC_NORM(R)
      
      
        
      // sym = (p->FoundState = Ppmd8Context_OneState(p->MinContext))->Symbol;
      // Ppmd8_UpdateBin(p);
      {
        unsigned freq = s->Freq;
        CPpmd8_Context *c = CTX(SUCCESSOR(s));
        sym = s->Symbol;
        p->FoundState = s;
        p->PrevSuccess = 1;
        p->RunLength++;
        s->Freq = (Byte)(freq + (freq < 196));
        // NextContext(p);
        if (p->OrderFall == 0 && (const Byte *)c >= p->UnitsStart)
          p->MaxContext = p->MinContext = c;
        else
          Ppmd8_UpdateModel(p);
      }
      return sym;
    }
    
    *prob = (UInt16)pr;
    p->InitEsc = p->ExpEscape[pr >> 10];
    
    // RangeDec_DecodeBit1(rc2, size0);
    R->Low += size0;
    R->Code -= size0;
    R->Range = (R->Range & ~((UInt32)PPMD_BIN_SCALE - 1)) - size0;
    RC_NORM_LOCAL(R)
    
    PPMD_SetAllBitsIn256Bytes(charMask)
    MASK(Ppmd8Context_OneState(p->MinContext)->Symbol) = 0;
    p->PrevSuccess = 0;
  }
  
  for (;;)
  {
    CPpmd_State *s, *s2;
    UInt32 freqSum, count, hiCnt;
    UInt32 freqSum2;
    CPpmd_See *see;
    CPpmd8_Context *mc;
    unsigned numMasked;
    RC_NORM_REMOTE(R)
    mc = p->MinContext;
    numMasked = mc->NumStats;
    
    do
    {
      p->OrderFall++;
      if (!mc->Suffix)
        return PPMD8_SYM_END;
      mc = Ppmd8_GetContext(p, mc->Suffix);
    }
    while (mc->NumStats == numMasked);
    
    s = Ppmd8_GetStats(p, mc);

    {
      unsigned num = (unsigned)mc->NumStats + 1;
      unsigned num2 = num / 2;

      num &= 1;
      hiCnt = (s->Freq & (UInt32)(MASK(s->Symbol))) & (0 - (UInt32)num);
      s += num;
      p->MinContext = mc;

      do
      {
        const unsigned sym0 = s[0].Symbol;
        const unsigned sym1 = s[1].Symbol;
        s += 2;
        hiCnt += (s[-2].Freq & (UInt32)(MASK(sym0)));
        hiCnt += (s[-1].Freq & (UInt32)(MASK(sym1)));
      }
      while (--num2);
    }
    
    see = Ppmd8_MakeEscFreq(p, numMasked, &freqSum);
    freqSum += hiCnt;
    freqSum2 = freqSum;
    PPMD8_CORRECT_SUM_RANGE(R, freqSum2)


    count = RC_GetThreshold(freqSum2);
    
    if (count < hiCnt)
    {
      Byte sym;
      // Ppmd_See_UPDATE(see) // new (see->Summ) value can overflow over 16-bits in some rare cases
      s = Ppmd8_GetStats(p, p->MinContext);
      hiCnt = count;

      
      {
        for (;;)
        {
          count -= s->Freq & (UInt32)(MASK((s)->Symbol)); s++; if ((Int32)count < 0) break;
          // count -= s->Freq & (UInt32)(MASK((s)->Symbol)); s++; if ((Int32)count < 0) break;
        }
      }
      s--;
      RC_DecodeFinal((hiCnt - count) - s->Freq, s->Freq)

      // new (see->Summ) value can overflow over 16-bits in some rare cases
      Ppmd_See_UPDATE(see)
      p->FoundState = s;
      sym = s->Symbol;
      Ppmd8_Update2(p);
      return sym;
    }

    if (count >= freqSum2)
      return PPMD8_SYM_ERROR;
    
    RC_Decode(hiCnt, freqSum2 - hiCnt)
    
    // We increase (see->Summ) for sum of Freqs of all non_Masked symbols.
    // new (see->Summ) value can overflow over 16-bits in some rare cases
    see->Summ = (UInt16)(see->Summ + freqSum);
    
    s = Ppmd8_GetStats(p, p->MinContext);
    s2 = s + p->MinContext->NumStats + 1;
    do
    {
      MASK(s->Symbol) = 0;
      s++;
    }
    while (s != s2);
  }
}

#undef kTop
#undef kBot
#undef READ_BYTE
#undef RC_NORM_BASE
#undef RC_NORM_1
#undef RC_NORM
#undef RC_NORM_LOCAL
#undef RC_NORM_REMOTE
#undef R
#undef RC_Decode
#undef RC_DecodeFinal
#undef RC_GetThreshold
#undef CTX
#undef SUCCESSOR
#undef MASK
#undef Ppmd8_DecodeSymbol
#undef Ppmd8_Init_RangeDec
#undef Ppmd8_MakeEscFreq
#undef Ppmd8_UpdateModel
#undef Ppmd8_UpdateBin
#undef Ppmd8_Update2
#undef Ppmd8_Update1_0
#undef Ppmd8_Update1
#undef Ppmd8_Init
#undef Ppmd8_Free
#undef Ppmd8_Alloc
#undef Ppmd8_Construct
#undef ShrinkUnits
/* ===== End embedded xppmd8_local.c ===== */
#include "xppmddecoder.h"
#include "xppmdrangedecoder.h"
#include "xppmdmodel.h"
#include "xppmd7model.h"

XPPMdDecoder::XPPMdDecoder(QObject *pParent) : QObject(pParent)
{
}

bool XPPMdDecoder::decompressPPMD8(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    QIODevice *pSourceDevice = pDecompressState->pDeviceInput;
    QIODevice *pDestDevice = pDecompressState->pDeviceOutput;

    // Read PPMd parameters (2 bytes for ZIP method 98)
    quint8 nParamByte1 = 0;
    quint8 nParamByte2 = 0;

    if (pSourceDevice->read((char *)&nParamByte1, 1) != 1) {
        return false;
    }

    if (pSourceDevice->read((char *)&nParamByte2, 1) != 1) {
        return false;
    }

    pDecompressState->nCountInput += 2;

    // Extract parameters from 2-byte header
    // The encoding format (from 7-Zip PpmdZip.cpp):
    // val = (Order - 1) + ((MemSizeMB - 1) << 4) + (Restor << 12)
    // Stored as 2 bytes little-endian
    quint16 nVal = nParamByte1 | (nParamByte2 << 8);
    quint8 nOrder = (nVal & 0x0F) + 1;             // Bits 0-3: Order - 1
    quint8 nMemSizeMB = ((nVal >> 4) & 0xFF) + 1;  // Bits 4-11: MemSizeMB - 1
    quint8 nRestor = (nVal >> 12);                 // Bits 12-15: Restor method

    quint32 nMemSize = ((quint32)nMemSizeMB) << 20;  // Memory in bytes

    // Validate parameters (matching PpmdZip.cpp from 7-Zip)
    if ((nOrder < XPPMdModel::MIN_ORDER) || (nOrder > XPPMdModel::MAX_ORDER)) {
        return false;
    }

    if (nRestor > 2) {
        return false;
    }

    // Initialize PPMd8 decoder using wrapper classes
    XPPMdModel model;

    if (!model.allocate(nMemSize)) {
        return false;
    }

    // Initialize 7-Zip's internal range decoder (hybrid solution)
    model.setInputStream(pSourceDevice);  // Set input stream for 7-Zip's internal decoder
    model.init(nOrder, nRestor);

    // Decompress
    const qint32 N_BUFFER_SIZE = 0x4000;
    char sBufferOut[N_BUFFER_SIZE];

    qint64 nUncompressedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, -1).toLongLong();

    qint64 nDecompressed = 0;
    bool bSuccess = true;
    bool bEndOfStream = false;
    qint32 nIterations = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct) && !bEndOfStream) {
        qint32 nToDecompress = N_BUFFER_SIZE;

        // Limit to remaining size if known
        if (nUncompressedSize > 0) {
            qint64 nRemaining = nUncompressedSize - nDecompressed;
            if (nRemaining <= 0) {
                break;
            }
            nToDecompress = qMin((qint64)nToDecompress, nRemaining);
        }

        // Decompress chunk
        qint32 nActual = 0;
        for (qint32 i = 0; i < nToDecompress && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            int nSymbol = model.decodeSymbol();

            if (nSymbol < 0) {
                // End of stream or error
                if (nUncompressedSize > 0 && nDecompressed < nUncompressedSize) {
                    bSuccess = false;
                }
                bEndOfStream = true;
                break;
            }

            sBufferOut[nActual++] = (char)nSymbol;
        }

        if (nActual > 0) {
            if (!XBinary::_writeDevice(sBufferOut, nActual, pDecompressState)) {
                bSuccess = false;
                break;
            }

            nDecompressed += nActual;
        }

        nIterations++;

        // Only break if we got fewer bytes than requested AND we hit the end of stream
        // Don't break just because we filled the buffer
        if (nActual < nToDecompress && bEndOfStream) {
            break;
        }
    }

    // Cleanup
    model.free();

    // Check if we got the expected amount of data
    if (nUncompressedSize > 0) {
        if (nDecompressed != nUncompressedSize) {
            return false;
        }
        // If we got the exact expected size, it's a success
        return true;
    }

    // If size is unknown, check if we successfully decoded something and there was no write error
    return bSuccess && (nDecompressed > 0) && !pDecompressState->bWriteError;
}

bool XPPMdDecoder::decompressPPMD7(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    // 7z PPMd format: 5 bytes (order, mem0-3 little-endian)
    if (baProperty.size() != 5) {
        return false;
    }

    // Extract PPMd parameters from 5-byte property
    quint8 nOrder = (quint8)baProperty[0];
    quint32 nMemSize =
        ((quint8)baProperty[1]) | (((quint32)(quint8)baProperty[2]) << 8) | (((quint32)(quint8)baProperty[3]) << 16) | (((quint32)(quint8)baProperty[4]) << 24);

    // Validate parameters
    if ((nOrder < XPPMd7Model::MIN_ORDER) || (nOrder > XPPMd7Model::MAX_ORDER)) {
        return false;
    }

    if (nMemSize == 0) {
        return false;
    }

    // Initialize Ppmd7 model (PPMdH variant used by 7z)
    XPPMd7Model model;

    if (!model.allocate(nMemSize)) {
        return false;
    }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    // Set input stream to compressed data
    model.setInputStream(pDecompressState->pDeviceInput);
    model.init(nOrder);  // PPMdH (Ppmd7) only takes order parameter

    // Decompress symbol by symbol
    const qint32 N_BUFFER_SIZE = 0x4000;
    char sBufferOut[N_BUFFER_SIZE];

    qint64 nUncompressedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, -1).toLongLong();
    qint64 nDecompressed = 0;
    bool bResult = true;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nActual = 0;

        // Decode buffer
        for (qint32 i = 0; i < N_BUFFER_SIZE && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            if (nUncompressedSize > 0 && nDecompressed >= nUncompressedSize) {
                break;  // Reached expected size
            }

            qint32 nSymbol = model.decodeSymbol();

            if (nSymbol < 0) {
                // End of stream or error
                if (nUncompressedSize > 0 && nDecompressed < nUncompressedSize) {
                    bResult = false;  // Unexpected end
                }
                break;
            }

            sBufferOut[nActual++] = (char)nSymbol;
            nDecompressed++;
        }

        // Write decoded data
        if (nActual > 0) {
            if (!XBinary::_writeDevice(sBufferOut, nActual, pDecompressState)) {
                bResult = false;
                break;
            }
        } else {
            // No more data
            break;
        }
    }

    model.free();

    // Verify size if known
    if (nUncompressedSize > 0) {
        if (nDecompressed != nUncompressedSize) {
            return false;
        }
        return true;
    }

    // If size is unknown, check if we successfully decoded something and there was no write error
    return bResult && (nDecompressed > 0) && !pDecompressState->bWriteError;
}

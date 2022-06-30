/*
 * Priority Dvfs_Queue for Dvfs.
 *
 * In order to ensure the highest freqency node is always the first node
 * and the freqency with same pid will overwrite by it's latest value.
 *
 */

#include <mm_dvfs_queue.h>

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "mmsys-dvfs: queue %d %s : "\
        fmt, __LINE__, __func__

void InitPQueue(PQueue pQueue) {
    if (pQueue == NULL)
        return;
    pQueue->next = NULL;
}
PNode *CreateNode(Dvfs_Queue DvfsQ) {
    PNode *pTmp = kzalloc(sizeof(PNode), 0);

    pTmp->DvfsQ = DvfsQ;
    pTmp->next = NULL;
    return pTmp;
}

unsigned long GetFrontNodeFreq(PQueue pQueue) {
    if (pQueue->next != NULL)
        return pQueue->next->DvfsQ.freq;

    pr_err("dvfs queue is NULL");
    return 0;
}

void Destroy(PQueue pQueue) {
    PNode *pCur = pQueue->next;

    while (pCur != NULL) {
        pQueue = pCur->next;
        kfree(pCur);
        pCur = pQueue;
    }
}

void ShowQueue(PQueue pQueue) {
    PNode *pCur = pQueue->next;

    while (pCur != NULL) {
        pr_info("dvfs fre is %ld\n", pCur->DvfsQ.freq);
        pr_info("dvfs pid is %ld\n", pCur->DvfsQ.pid);
        pr_info("dvfs active is %d\n", pCur->DvfsQ.active);
        pCur = pCur->next;
    }
}

bool IsEmpty(PQueue pQueue) {
    if (pQueue->next == NULL)
        return true;
    else
        return false;
}

int MatchPid_RemoveInvalidNode(PQueue pQueue, Dvfs_Queue DvfsQ) {
    PNode *pPre = pQueue;
    PNode *pCur = pQueue->next;

    while (pCur != NULL) {
        if (pCur->DvfsQ.pid == DvfsQ.pid) {
            if (DvfsQ.active == 0) {
                DelNode(pPre);
            } else {
                DelNode(pPre);
                AddNode(pPre, DvfsQ);
            }
            return 0;
        } else {
            pPre = pCur;
            pCur = pCur->next;
        }
    }
    if (DvfsQ.active == 0)
        return 0;
    AddNode(pQueue, DvfsQ);
    return 0;
}

void DelNode(PQueue pQueue) {
    PNode *pPre = pQueue;
    PNode *pCur = pQueue->next;
    PNode *pTmp = pQueue;

    if (pPre != NULL) {
        pTmp = pCur;
        pPre->next = pCur->next;
        kfree(pTmp);
    }
}

void AddNode(PQueue pQueue, Dvfs_Queue DvfsQ) {
    PNode *pTmp = CreateNode(DvfsQ);
    PNode *pPre = pQueue;
    PNode *pCur = pQueue->next;

    while (pCur != NULL) {
        if (pCur->DvfsQ.freq < DvfsQ.freq) {
            pTmp->next = pCur;
            pPre->next = pTmp;
            return;
        }
        pPre = pCur;
        pCur = pCur->next;
    }
    pPre->next = pTmp;
}

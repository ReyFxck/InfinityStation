#include <types.h>
#include <irx.h>
#include <loadcore.h>
#include <thbase.h>
#include <intrman.h>
#include <sifrpc.h>
#include <stdio.h>
#include <sysclib.h>

IRX_ID("libsdpcm", 1, 0);

#define LIBSDPCM_RPC_ID 0x00001234

extern struct irx_export_table _exp_libsdpcm;

enum {
    LIBSDPCM_CMD_PING = 1,
    LIBSDPCM_CMD_INIT = 2,
    LIBSDPCM_CMD_QUIT = 3,
    LIBSDPCM_CMD_PUSH = 4
};

typedef struct {
    unsigned int cmd;
    unsigned int arg0;
    unsigned int arg1;
    unsigned int arg2;
    unsigned int arg3;
    int          result;
    unsigned int magic;
    unsigned int counter;
    unsigned char payload[2048];
} rpc_pkt_t;

static SifRpcDataQueue_t qd;
static SifRpcServerData_t sd0;
static rpc_pkt_t rpcbuf __attribute__((aligned(64)));

static void *libsdpcm_rpc_handler(int fno, void *data, int size)
{
    rpc_pkt_t *pkt = (rpc_pkt_t *)data;
    (void)fno;
    (void)size;

    pkt->magic = LIBSDPCM_RPC_ID;
    pkt->counter++;

    switch (pkt->cmd) {
        case LIBSDPCM_CMD_PING:
        case LIBSDPCM_CMD_INIT:
        case LIBSDPCM_CMD_QUIT:
        case LIBSDPCM_CMD_PUSH:
            pkt->result = 0;
            break;
        default:
            pkt->result = -1;
            break;
    }

    return data;
}

static void libsdpcm_thread(void *arg)
{
    (void)arg;

    SifInitRpc(0);
    SifSetRpcQueue(&qd, GetThreadId());
    SifRegisterRpc(&sd0, LIBSDPCM_RPC_ID, libsdpcm_rpc_handler, &rpcbuf, NULL, NULL, &qd);
    SifRpcLoop(&qd);
}

int _start(int argc, char *argv[])
{
    iop_thread_t t;
    int tid;

    (void)argc;
    (void)argv;

    FlushDcache();
    CpuEnableIntr();

    if (RegisterLibraryEntries(&_exp_libsdpcm) != 0)
        return MODULE_NO_RESIDENT_END;

    memset(&t, 0, sizeof(t));
    t.attr      = TH_C;
    t.thread    = libsdpcm_thread;
    t.priority  = 40;
    t.stacksize = 0x1000;
    t.option    = 0;

    tid = CreateThread(&t);
    if (tid <= 0) {
        ReleaseLibraryEntries(&_exp_libsdpcm);
        return MODULE_NO_RESIDENT_END;
    }

    if (StartThread(tid, 0) < 0) {
        ReleaseLibraryEntries(&_exp_libsdpcm);
        return MODULE_NO_RESIDENT_END;
    }

    return MODULE_RESIDENT_END;
}

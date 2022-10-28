#include "dbgdata.h"
#include <dbgeng.h>
#include "CmdExt.h"

size_t readDbgDataAddr(ULONG index)
{
    size_t addr_field = 0;
    EXT_D_IDebugDataSpaces->ReadDebuggerData(index, &addr_field, sizeof(size_t), NULL);
    return addr_field;
}


DEFINE_CMD(dbgdata)
{
    EXT_F_OUT("Debugger Data:\n");
    EXT_F_OUT("%30s 0x%0I64x\n", "kernel base",
        readDbgDataAddr(DEBUG_DATA_KernBase));

    EXT_F_OUT("%30s 0x%0I64x\n", "BreakpointWithStatusInstruction.",
        readDbgDataAddr(DEBUG_DATA_BreakpointWithStatusAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "KiCallUserMode.",
        readDbgDataAddr(DEBUG_DATA_KiCallUserModeAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "KeUserCallbackDispatcher.",
        readDbgDataAddr(DEBUG_DATA_KeUserCallbackDispatcherAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "PsLoadedModuleList.",
        readDbgDataAddr(DEBUG_DATA_PsLoadedModuleListAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "PsActiveProcessHead.",
        readDbgDataAddr(DEBUG_DATA_PsActiveProcessHeadAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "PspCidTable.",
        readDbgDataAddr(DEBUG_DATA_PspCidTableAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "ExpSystemResourcesList.",
        readDbgDataAddr(DEBUG_DATA_ExpSystemResourcesListAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "ExpPagedPoolDescriptor.",
        readDbgDataAddr(DEBUG_DATA_ExpPagedPoolDescriptorAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "ExpNumberOfPagedPools.",
        readDbgDataAddr(DEBUG_DATA_ExpNumberOfPagedPoolsAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "KeTimeIncrement.",
        readDbgDataAddr(DEBUG_DATA_KeTimeIncrementAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "KeBugCheckCallbackListHead.",
        readDbgDataAddr(DEBUG_DATA_KeBugCheckCallbackListHeadAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "KiBugCheckData.",
        readDbgDataAddr(DEBUG_DATA_KiBugcheckDataAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "IopErrorLogListHead.",
        readDbgDataAddr(DEBUG_DATA_IopErrorLogListHeadAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "ObpRootDirectoryObject.",
        readDbgDataAddr(DEBUG_DATA_ObpRootDirectoryObjectAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "ObpTypeObjectType.",
        readDbgDataAddr(DEBUG_DATA_ObpTypeObjectTypeAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmSystemCacheStart.",
        readDbgDataAddr(DEBUG_DATA_MmSystemCacheStartAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmSystemCacheEnd.",
        readDbgDataAddr(DEBUG_DATA_MmSystemCacheEndAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmSystemCacheWs.",
        readDbgDataAddr(DEBUG_DATA_MmSystemCacheWsAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmPfnDatabase.",
        readDbgDataAddr(DEBUG_DATA_MmPfnDatabaseAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmSystemPtesStart.",
        readDbgDataAddr(DEBUG_DATA_MmSystemPtesStartAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmSystemPtesEnd.",
        readDbgDataAddr(DEBUG_DATA_MmSystemPtesEndAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmSubsectionBase.",
        readDbgDataAddr(DEBUG_DATA_MmSubsectionBaseAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmNumberOfPagingFiles.",
        readDbgDataAddr(DEBUG_DATA_MmNumberOfPagingFilesAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmLowestPhysicalPage.",
        readDbgDataAddr(DEBUG_DATA_MmLowestPhysicalPageAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmHighestPhysicalPage.",
        readDbgDataAddr(DEBUG_DATA_MmHighestPhysicalPageAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmNumberOfPhysicalPages.",
        readDbgDataAddr(DEBUG_DATA_MmNumberOfPhysicalPagesAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmMaximumNonPagedPoolInBytes.",
        readDbgDataAddr(DEBUG_DATA_MmMaximumNonPagedPoolInBytesAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmNonPagedSystemStart.",
        readDbgDataAddr(DEBUG_DATA_MmNonPagedSystemStartAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmNonPagedPoolStart.",
        readDbgDataAddr(DEBUG_DATA_MmNonPagedPoolStartAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmNonPagedPoolEnd.",
        readDbgDataAddr(DEBUG_DATA_MmNonPagedPoolEndAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmPagedPoolStart.",
        readDbgDataAddr(DEBUG_DATA_MmPagedPoolStartAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmPagedPoolEnd.",
        readDbgDataAddr(DEBUG_DATA_MmPagedPoolEndAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmPagedPoolInfo.",
        readDbgDataAddr(DEBUG_DATA_MmPagedPoolInformationAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmSizeOfPagedPoolInBytes.",
        readDbgDataAddr(DEBUG_DATA_MmSizeOfPagedPoolInBytesAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmTotalCommitLimit.",
        readDbgDataAddr(DEBUG_DATA_MmTotalCommitLimitAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmTotalCommittedPages.",
        readDbgDataAddr(DEBUG_DATA_MmTotalCommittedPagesAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmSharedCommit.",
        readDbgDataAddr(DEBUG_DATA_MmSharedCommitAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmDriverCommit.",
        readDbgDataAddr(DEBUG_DATA_MmDriverCommitAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmProcessCommit.",
        readDbgDataAddr(DEBUG_DATA_MmProcessCommitAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmPagedPoolCommit.",
        readDbgDataAddr(DEBUG_DATA_MmPagedPoolCommitAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmExtendedCommit..",
        readDbgDataAddr(DEBUG_DATA_MmExtendedCommitAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmZeroedPageListHead.",
        readDbgDataAddr(DEBUG_DATA_MmZeroedPageListHeadAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmFreePageListHead.",
        readDbgDataAddr(DEBUG_DATA_MmFreePageListHeadAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmStandbyPageListHead.",
        readDbgDataAddr(DEBUG_DATA_MmStandbyPageListHeadAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmModifiedPageListHead.",
        readDbgDataAddr(DEBUG_DATA_MmModifiedPageListHeadAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmModifiedNoWritePageListHead.",
        readDbgDataAddr(DEBUG_DATA_MmModifiedNoWritePageListHeadAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmAvailablePages.",
        readDbgDataAddr(DEBUG_DATA_MmAvailablePagesAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmResidentAvailablePages.",
        readDbgDataAddr(DEBUG_DATA_MmResidentAvailablePagesAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "PoolTrackTable.",
        readDbgDataAddr(DEBUG_DATA_PoolTrackTableAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "NonPagedPoolDescriptor.",
        readDbgDataAddr(DEBUG_DATA_NonPagedPoolDescriptorAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmHighestUserAddress.",
        readDbgDataAddr(DEBUG_DATA_MmHighestUserAddressAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmSystemRangeStart.",
        readDbgDataAddr(DEBUG_DATA_MmSystemRangeStartAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmUserProbeAddress.",
        readDbgDataAddr(DEBUG_DATA_MmUserProbeAddressAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "KdPrintDefaultCircularBuffer.",
        readDbgDataAddr(DEBUG_DATA_KdPrintCircularBufferAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "KdPrintDefaultCircularBuffer",
        readDbgDataAddr(DEBUG_DATA_KdPrintCircularBufferEndAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "KdPrintWritePointer.",
        readDbgDataAddr(DEBUG_DATA_KdPrintWritePointerAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "KdPrintRolloverCount.",
        readDbgDataAddr(DEBUG_DATA_KdPrintRolloverCountAddr));

    EXT_F_OUT("%30s 0x%0I64x\n", "MmLoadedUserImageList.",
        readDbgDataAddr(DEBUG_DATA_MmLoadedUserImageListAddr));
}

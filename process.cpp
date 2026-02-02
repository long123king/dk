#include "process.h"
#include "token.h"
#include "CmdExt.h"

#include <iomanip>


struct PS_FLAGS
{
	uint32_t CreateReported : 1;
	uint32_t NoDebugInherit : 1;
	uint32_t ProcessExiting : 1;
	uint32_t ProcessDelete : 1;
	uint32_t ControlFlowGuardEnabled : 1;
	uint32_t VmDeleted : 1;
	uint32_t OutswapEnabled : 1;
	uint32_t Outswapped : 1;
	uint32_t FailFastOnCommitFail : 1;
	uint32_t Wow64VaSpace4Gb : 1;
	uint32_t AddressSpaceInitialized : 2;
	uint32_t SetTimerResolution : 1;
	uint32_t BreakOnTermination : 1;
	uint32_t DeprioritizeViews : 1;
	uint32_t WriteWatch : 1;
	uint32_t ProcessInSession : 1;
	uint32_t OverrideAddressSpace : 1;
	uint32_t HasAddressSpace : 1;
	uint32_t LaunchPrefetched : 1;
	uint32_t Background : 1;
	uint32_t VmTopDown : 1;
	uint32_t ImageNotifyDone : 1;
	uint32_t PdeUpdateNeeded : 1;
	uint32_t VdmAllowed : 1;
	uint32_t ProcessRundown : 1;
	uint32_t ProcessInserted : 1;
	uint32_t DefaultIoPriority : 3;
	uint32_t ProcessSelfDelete : 1;
	uint32_t SetTimerResolutionLink : 1;


	PS_FLAGS(uint32_t source)
	{
		memcpy(this, &source, 4);
	}

	std::string str()
	{
		std::stringstream ss;

		if (CreateReported != 0) ss << std::setw(50) << " CreateReported \n";
		if (NoDebugInherit != 0) ss << std::setw(50) << " NoDebugInherit \n";
		if (ProcessExiting != 0) ss << std::setw(50) << " ProcessExiting \n";
		if (ProcessDelete != 0) ss << std::setw(50) << " ProcessDelete \n";
		if (ControlFlowGuardEnabled != 0) ss << std::setw(50) << " ControlFlowGuardEnabled " << "\t\t[Mitigation]\n";
		if (VmDeleted != 0) ss << std::setw(50) << " VmDeleted \n";
		if (OutswapEnabled != 0) ss << std::setw(50) << " OutswapEnabled \n";
		if (Outswapped != 0) ss << std::setw(50) << " Outswapped \n";
		if (FailFastOnCommitFail != 0) ss << std::setw(50) << " FailFastOnCommitFail \n";
		if (Wow64VaSpace4Gb != 0) ss << std::setw(50) << " Wow64VaSpace4Gb \n";

		if (SetTimerResolution != 0) ss << std::setw(50) << " SetTimerResolution \n";
		if (BreakOnTermination != 0) ss << std::setw(50) << " BreakOnTermination \n";
		if (DeprioritizeViews != 0) ss << std::setw(50) << " DeprioritizeViews \n";
		if (WriteWatch != 0) ss << std::setw(50) << " WriteWatch \n";
		if (ProcessInSession != 0) ss << std::setw(50) << " ProcessInSession \n";
		if (OverrideAddressSpace != 0) ss << std::setw(50) << " OverrideAddressSpace \n";
		if (HasAddressSpace != 0) ss << std::setw(50) << " HasAddressSpace \n";
		if (LaunchPrefetched != 0) ss << std::setw(50) << " LaunchPrefetched \n";
		if (Background != 0) ss << std::setw(50) << " Background \n";
		if (VmTopDown != 0) ss << std::setw(50) << " VmTopDown \n";
		if (ImageNotifyDone != 0) ss << std::setw(50) << " ImageNotifyDone \n";
		if (PdeUpdateNeeded != 0) ss << std::setw(50) << " PdeUpdateNeeded \n";
		if (VdmAllowed != 0) ss << std::setw(50) << " VdmAllowed \n";
		if (ProcessRundown != 0) ss << std::setw(50) << " ProcessRundown \n";
		if (ProcessInserted != 0) ss << std::setw(50) << " ProcessInserted \n";

		if (ProcessSelfDelete != 0) ss << std::setw(50) << " ProcessSelfDelete \n";
		if (SetTimerResolutionLink != 0) ss << std::setw(50) << " SetTimerResolutionLink \n";

		ss << std::setw(50) << "AddressSpaceInitialized : " << std::hex << std::setw(8) << AddressSpaceInitialized << "\n"
			<< std::setw(50) << "DefaultIoPriority : " << std::hex << std::setw(8) << DefaultIoPriority;

		return ss.str();
	}
};

struct PS_FLAGS3
{
	uint32_t Minimal : 1;
	uint32_t ReplacingPageRoot : 1;
	uint32_t DisableNonSystemFonts : 1;
	uint32_t AuditNonSystemFontLoading : 1;
	uint32_t Crashed : 1;
	uint32_t JobVadsAreTracked : 1;
	uint32_t VadTrackingDisabled : 1;
	uint32_t AuxiliaryProcess : 1;
	uint32_t SubsystemProcess : 1;
	uint32_t IndirectCpuSets : 1;
	uint32_t InPrivate : 1;
	uint32_t ProhibitRemoteImageMap : 1;
	uint32_t ProhibitLowILImageMap : 1;
	uint32_t SignatureMitigationOptIn : 1;
	uint32_t DisableDynamicCodeAllowOptOut : 1;
	uint32_t EnableFilteredWin32kAPIs : 1;
	uint32_t AuditFilteredWin32kAPIs : 1;
	uint32_t PreferSystem32Images : 1;
	uint32_t RelinquishedCommit : 1;
	uint32_t AutomaticallyOverrideChildProcessPolicy : 1;
	uint32_t HighGraphicsPriority : 1;
	uint32_t CommitFailLogged : 1;
	uint32_t ReserveFailLogged : 1;

	PS_FLAGS3(uint32_t source)
	{
		memcpy(this, &source, 4);
	}

	std::string str()
	{
		std::stringstream ss;

		if (Minimal != 0) ss << std::setw(50) << " Minimal \n";
		if (ReplacingPageRoot != 0) ss << std::setw(50) << " ReplacingPageRoot \n";
		if (DisableNonSystemFonts != 0) ss << std::setw(50) << " DisableNonSystemFonts " << "\t\t[Mitigation]\n";
		if (AuditNonSystemFontLoading != 0) ss << std::setw(50) << " AuditNonSystemFontLoading \n";
		if (Crashed != 0) ss << std::setw(50) << " Crashed \n";
		if (JobVadsAreTracked != 0) ss << std::setw(50) << " JobVadsAreTracked \n";
		if (VadTrackingDisabled != 0) ss << std::setw(50) << " VadTrackingDisabled \n";
		if (AuxiliaryProcess != 0) ss << std::setw(50) << " AuxiliaryProcess \n";
		if (SubsystemProcess != 0) ss << std::setw(50) << " SubsystemProcess \n";
		if (IndirectCpuSets != 0) ss << std::setw(50) << " IndirectCpuSets \n";
		if (InPrivate != 0) ss << std::setw(50) << " InPrivate \n";
		if (ProhibitRemoteImageMap != 0) ss << std::setw(50) << " ProhibitRemoteImageMap " << "\t\t[Mitigation]\n";
		if (ProhibitLowILImageMap != 0) ss << std::setw(50) << " ProhibitLowILImageMap " << "\t\t[Mitigation]\n";
		if (SignatureMitigationOptIn != 0) ss << std::setw(50) << " SignatureMitigationOptIn \n";
		if (DisableDynamicCodeAllowOptOut != 0) ss << std::setw(50) << " DisableDynamicCodeAllowOptOut \n";
		if (EnableFilteredWin32kAPIs != 0) ss << std::setw(50) << " EnableFilteredWin32kAPIs " << "\t\t[Mitigation]\n";
		if (AuditFilteredWin32kAPIs != 0) ss << std::setw(50) << " AuditFilteredWin32kAPIs \n";
		if (PreferSystem32Images != 0) ss << std::setw(50) << " PreferSystem32Images " << "\t\t[Mitigation]\n";
		if (RelinquishedCommit != 0) ss << std::setw(50) << " RelinquishedCommit \n";
		if (AutomaticallyOverrideChildProcessPolicy != 0) ss << std::setw(50) << " AutomaticallyOverrideChildProcessPolicy \n";
		if (HighGraphicsPriority != 0) ss << std::setw(50) << " HighGraphicsPriority \n";
		if (CommitFailLogged != 0) ss << std::setw(50) << " CommitFailLogged \n";
		if (ReserveFailLogged != 0) ss << std::setw(50) << " ReserveFailLogged \n";

		return ss.str();
	}
};

struct PS_FLAGS2
{
	uint32_t JobNotReallyActive : 1;
	uint32_t AccountingFolded : 1;
	uint32_t NewProcessReported : 1;
	uint32_t ExitProcessReported : 1;
	uint32_t ReportCommitChanges : 1;
	uint32_t LastReportMemory : 1;
	uint32_t ForceWakeCharge : 1;
	uint32_t CrossSessionCreate : 1;
	uint32_t NeedsHandleRundown : 1;
	uint32_t RefTraceEnabled : 1;
	uint32_t DisableDynamicCode : 1;
	uint32_t EmptyJobEvaluated : 1;
	uint32_t DefaultPagePriority : 3;
	uint32_t PrimaryTokenFrozen : 1;
	uint32_t ProcessVerifierTarget : 1;
	uint32_t StackRandomizationDisabled : 1;
	uint32_t AffinityPermanent : 1;
	uint32_t AffinityUpdateEnable : 1;
	uint32_t PropagateNode : 1;
	uint32_t ExplicitAffinity : 1;
	uint32_t ProcessExecutionState : 2;
	uint32_t DisallowStrippedImages : 1;
	uint32_t HighEntropyASLREnabled : 1;
	uint32_t ExtensionPointDisable : 1;
	uint32_t ForceRelocateImages : 1;
	uint32_t ProcessStateChangeRequest : 2;
	uint32_t ProcessStateChangeInProgress : 1;
	uint32_t DisallowWin32kSystemCalls : 1;

	PS_FLAGS2(uint32_t source)
	{
		memcpy(this, &source, 4);
	}

	std::string str()
	{
		std::stringstream ss;

		if (JobNotReallyActive != 0) ss << std::setw(50) << " JobNotReallyActive \n";
		if (AccountingFolded != 0) ss << std::setw(50) << " AccountingFolded \n";
		if (NewProcessReported != 0) ss << std::setw(50) << " NewProcessReported \n";
		if (ExitProcessReported != 0) ss << std::setw(50) << " ExitProcessReported \n";
		if (ReportCommitChanges != 0) ss << std::setw(50) << " ReportCommitChanges \n";
		if (LastReportMemory != 0) ss << std::setw(50) << " LastReportMemory \n";
		if (ForceWakeCharge != 0) ss << std::setw(50) << " ForceWakeCharge \n";
		if (CrossSessionCreate != 0) ss << std::setw(50) << " CrossSessionCreate \n";
		if (NeedsHandleRundown != 0) ss << std::setw(50) << " NeedsHandleRundown \n";
		if (RefTraceEnabled != 0) ss << std::setw(50) << " RefTraceEnabled \n";
		if (DisableDynamicCode != 0) ss << std::setw(50) << " DisableDynamicCode \n";
		if (EmptyJobEvaluated != 0) ss << std::setw(50) << " EmptyJobEvaluated \n";

		if (PrimaryTokenFrozen != 0) ss << std::setw(50) << " PrimaryTokenFrozen \n";
		if (ProcessVerifierTarget != 0) ss << std::setw(50) << " ProcessVerifierTarget \n";
		if (StackRandomizationDisabled != 0) ss << std::setw(50) << " StackRandomizationDisabled \n";
		if (AffinityPermanent != 0) ss << std::setw(50) << " AffinityPermanent \n";
		if (AffinityUpdateEnable != 0) ss << std::setw(50) << " AffinityUpdateEnable \n";
		if (PropagateNode != 0) ss << std::setw(50) << " PropagateNode \n";
		if (ExplicitAffinity != 0) ss << std::setw(50) << " ExplicitAffinity \n";

		if (DisallowStrippedImages != 0) ss << std::setw(50) << " DisallowStrippedImages " << "\t\t[Mitigation]\n";
		if (HighEntropyASLREnabled != 0) ss << std::setw(50) << " HighEntropyASLREnabled " << "\t\t[Mitigation]\n";
		if (ExtensionPointDisable != 0) ss << std::setw(50) << " ExtensionPointDisable " << "\t\t[Mitigation]\n";
		if (ForceRelocateImages != 0) ss << std::setw(50) << " ForceRelocateImages " << "\t\t[Mitigation]\n";

		if (ProcessStateChangeInProgress != 0) ss << std::setw(50) << " ProcessStateChangeInProgress \n";
		if (DisallowWin32kSystemCalls != 0) ss << std::setw(50) << " DisallowWin32kSystemCalls " << "\t\t[Mitigation]\n";

		ss << std::setw(50) << " DefaultPagePriority : " << std::hex << std::setw(8) << DefaultPagePriority << "\n"
			<< std::setw(50) << "ProcessExecutionState : " << std::hex << std::setw(8) << ProcessExecutionState << "\n"
			<< std::setw(50) << "ProcessStateChangeRequest : " << std::hex << std::setw(8) << ProcessStateChangeRequest;

		return ss.str();
	}
};

struct MITIGATION_FLAGS
{
	uint32_t ControlFlowGuardEnabled : 1;
	uint32_t ControlFlowGuardExportSuppressionEnabled : 1;
	uint32_t ControlFlowGuardStrict : 1;
	uint32_t DisallowStrippedImages : 1;
	uint32_t ForceRelocateImages : 1;
	uint32_t HighEntropyASLREnabled : 1;
	uint32_t StackRandomizationDisabled : 1;
	uint32_t ExtensionPointDisable : 1;
	uint32_t DisableDynamicCode : 1;
	uint32_t DisableDynamicCodeAllowOptOut : 1;
	uint32_t DisableDynamicCodeAllowRemoteDowngrade : 1;
	uint32_t AuditDisableDynamicCode : 1;
	uint32_t DisallowWin32kSystemCalls : 1;
	uint32_t AuditDisallowWin32kSystemCalls : 1;
	uint32_t EnableFilteredWin32kAPIs : 1;
	uint32_t AuditFilteredWin32kAPIs : 1;
	uint32_t DisableNonSystemFonts : 1;
	uint32_t AuditNonSystemFontLoading : 1;
	uint32_t PreferSystem32Images : 1;
	uint32_t ProhibitRemoteImageMap : 1;
	uint32_t AuditProhibitRemoteImageMap : 1;
	uint32_t ProhibitLowILImageMap : 1;
	uint32_t AuditProhibitLowILImageMap : 1;
	uint32_t SignatureMitigationOptIn : 1;
	uint32_t AuditBlockNonMicrosoftBinaries : 1;
	uint32_t AuditBlockNonMicrosoftBinariesAllowStore : 1;
	uint32_t LoaderIntegrityContinuityEnabled : 1;
	uint32_t AuditLoaderIntegrityContinuity : 1;
	uint32_t EnableModuleTamperingProtection : 1;
	uint32_t EnableModuleTamperingProtectionNoInherit : 1;
	uint32_t RestrictIndirectBranchPrediction : 1;
	uint32_t IsolateSecurityDomain : 1;

	MITIGATION_FLAGS(uint32_t source)
	{
		memcpy(this, &source, 4);
	}

	std::string str()
	{
		std::stringstream ss;

		if (ControlFlowGuardEnabled != 0) ss << std::setw(50) << "ControlFlowGuardEnabled \n";
		if (ControlFlowGuardExportSuppressionEnabled != 0) ss << std::setw(50) << "ControlFlowGuardExportSuppressionEnabled \n";
		if (ControlFlowGuardStrict != 0) ss << std::setw(50) << "ControlFlowGuardStrict \n";
		if (DisallowStrippedImages != 0) ss << std::setw(50) << "DisallowStrippedImages \n";
		if (ForceRelocateImages != 0) ss << std::setw(50) << "ForceRelocateImages \n";
		if (HighEntropyASLREnabled != 0) ss << std::setw(50) << "HighEntropyASLREnabled \n";
		if (StackRandomizationDisabled != 0) ss << std::setw(50) << "StackRandomizationDisabled \n";
		if (ExtensionPointDisable != 0) ss << std::setw(50) << "ExtensionPointDisable \n";
		if (DisableDynamicCode != 0) ss << std::setw(50) << "DisableDynamicCode \n";
		if (DisableDynamicCodeAllowOptOut != 0) ss << std::setw(50) << "DisableDynamicCodeAllowOptOut \n";
		if (DisableDynamicCodeAllowRemoteDowngrade != 0) ss << std::setw(50) << "DisableDynamicCodeAllowRemoteDowngrade \n";
		if (AuditDisableDynamicCode != 0) ss << std::setw(50) << "AuditDisableDynamicCode \n";
		if (DisallowWin32kSystemCalls != 0) ss << std::setw(50) << "DisallowWin32kSystemCalls \n";
		if (AuditDisallowWin32kSystemCalls != 0) ss << std::setw(50) << "AuditDisallowWin32kSystemCalls \n";
		if (EnableFilteredWin32kAPIs != 0) ss << std::setw(50) << "EnableFilteredWin32kAPIs \n";
		if (AuditFilteredWin32kAPIs != 0) ss << std::setw(50) << "AuditFilteredWin32kAPIs \n";
		if (DisableNonSystemFonts != 0) ss << std::setw(50) << "DisableNonSystemFonts \n";
		if (AuditNonSystemFontLoading != 0) ss << std::setw(50) << "AuditNonSystemFontLoading \n";
		if (PreferSystem32Images != 0) ss << std::setw(50) << "PreferSystem32Images \n";
		if (ProhibitRemoteImageMap != 0) ss << std::setw(50) << "ProhibitRemoteImageMap \n";
		if (AuditProhibitRemoteImageMap != 0) ss << std::setw(50) << "AuditProhibitRemoteImageMap \n";
		if (ProhibitLowILImageMap != 0) ss << std::setw(50) << "ProhibitLowILImageMap \n";
		if (AuditProhibitLowILImageMap != 0) ss << std::setw(50) << "AuditProhibitLowILImageMap \n";
		if (SignatureMitigationOptIn != 0) ss << std::setw(50) << "SignatureMitigationOptIn \n";
		if (AuditBlockNonMicrosoftBinaries != 0) ss << std::setw(50) << "AuditBlockNonMicrosoftBinaries \n";
		if (AuditBlockNonMicrosoftBinariesAllowStore != 0) ss << std::setw(50) << "AuditBlockNonMicrosoftBinariesAllowStore \n";
		if (LoaderIntegrityContinuityEnabled != 0) ss << std::setw(50) << "LoaderIntegrityContinuityEnabled \n";
		if (AuditLoaderIntegrityContinuity != 0) ss << std::setw(50) << "AuditLoaderIntegrityContinuity \n";
		if (EnableModuleTamperingProtection != 0) ss << std::setw(50) << "EnableModuleTamperingProtection \n";
		if (EnableModuleTamperingProtectionNoInherit != 0) ss << std::setw(50) << "EnableModuleTamperingProtectionNoInherit \n";
		if (RestrictIndirectBranchPrediction != 0) ss << std::setw(50) << "RestrictIndirectBranchPrediction \n";
		if (IsolateSecurityDomain != 0) ss << std::setw(50) << "IsolateSecurityDomain \n";

		return ss.str();
	}
};

struct MITIGATION_FLAGS2
{
	uint32_t EnableExportAddressFilter : 1;
	uint32_t AuditExportAddressFilter : 1;
	uint32_t EnableExportAddressFilterPlus : 1;
	uint32_t AuditExportAddressFilterPlus : 1;
	uint32_t EnableRopStackPivot : 1;
	uint32_t AuditRopStackPivot : 1;
	uint32_t EnableRopCallerCheck : 1;
	uint32_t AuditRopCallerCheck : 1;
	uint32_t EnableRopSimExec : 1;
	uint32_t AuditRopSimExec : 1;
	uint32_t EnableImportAddressFilter : 1;
	uint32_t AuditImportAddressFilter : 1;
	uint32_t DisablePageCombine : 1;
	uint32_t SpeculativeStoreBypassDisable : 1;
	uint32_t CetUserShadowStacks : 1;
	uint32_t AuditCetUserShadowStacks : 1;
	uint32_t AuditCetUserShadowStacksLogged : 1;
	uint32_t UserCetSetContextIpValidation : 1;
	uint32_t AuditUserCetSetContextIpValidation : 1;
	uint32_t AuditUserCetSetContextIpValidationLogged : 1;
	uint32_t CetUserShadowStacksStrictMode : 1;
	uint32_t BlockNonCetBinaries : 1;
	uint32_t BlockNonCetBinariesNonEhcont : 1;
	uint32_t AuditBlockNonCetBinaries : 1;
	uint32_t AuditBlockNonCetBinariesLogged : 1;
	uint32_t Reserved1 : 1;
	uint32_t Reserved2 : 1;
	uint32_t Reserved3 : 1;
	uint32_t Reserved4 : 1;
	uint32_t Reserved5 : 1;
	uint32_t CetDynamicApisOutOfProcOnly : 1;
	uint32_t UserCetSetContextIpValidationRelaxedMode : 1;

	MITIGATION_FLAGS2(uint32_t source)
	{
		memcpy(this, &source, 4);
	}

	std::string str()
	{
		std::stringstream ss;

		if (EnableExportAddressFilter != 0) ss << std::setw(50) << "EnableExportAddressFilter \n";
		if (AuditExportAddressFilter != 0) ss << std::setw(50) << "AuditExportAddressFilter \n";
		if (EnableExportAddressFilterPlus != 0) ss << std::setw(50) << "EnableExportAddressFilterPlus \n";
		if (AuditExportAddressFilterPlus != 0) ss << std::setw(50) << "AuditExportAddressFilterPlus \n";
		if (EnableRopStackPivot != 0) ss << std::setw(50) << "EnableRopStackPivot \n";
		if (AuditRopStackPivot != 0) ss << std::setw(50) << "AuditRopStackPivot \n";
		if (EnableRopCallerCheck != 0) ss << std::setw(50) << "EnableRopCallerCheck \n";
		if (AuditRopCallerCheck != 0) ss << std::setw(50) << "AuditRopCallerCheck \n";
		if (EnableRopSimExec != 0) ss << std::setw(50) << "EnableRopSimExec \n";
		if (AuditRopSimExec != 0) ss << std::setw(50) << "AuditRopSimExec \n";
		if (EnableImportAddressFilter != 0) ss << std::setw(50) << "EnableImportAddressFilter \n";
		if (AuditImportAddressFilter != 0) ss << std::setw(50) << "AuditImportAddressFilter \n";
		if (DisablePageCombine != 0) ss << std::setw(50) << "DisablePageCombine \n";
		if (SpeculativeStoreBypassDisable != 0) ss << std::setw(50) << "SpeculativeStoreBypassDisable \n";
		if (CetUserShadowStacks != 0) ss << std::setw(50) << "CetUserShadowStacks \n";
		if (AuditCetUserShadowStacks != 0) ss << std::setw(50) << "AuditCetUserShadowStacks \n";
		if (AuditCetUserShadowStacksLogged != 0) ss << std::setw(50) << "AuditCetUserShadowStacksLogged \n";
		if (UserCetSetContextIpValidation != 0) ss << std::setw(50) << "UserCetSetContextIpValidation \n";
		if (AuditUserCetSetContextIpValidation != 0) ss << std::setw(50) << "AuditUserCetSetContextIpValidation \n";
		if (AuditUserCetSetContextIpValidationLogged != 0) ss << std::setw(50) << "AuditUserCetSetContextIpValidationLogged \n";
		if (CetUserShadowStacksStrictMode != 0) ss << std::setw(50) << "CetUserShadowStacksStrictMode \n";
		if (BlockNonCetBinaries != 0) ss << std::setw(50) << "BlockNonCetBinaries \n";
		if (BlockNonCetBinariesNonEhcont != 0) ss << std::setw(50) << "BlockNonCetBinariesNonEhcont \n";
		if (AuditBlockNonCetBinaries != 0) ss << std::setw(50) << "AuditBlockNonCetBinaries \n";
		if (AuditBlockNonCetBinariesLogged != 0) ss << std::setw(50) << "AuditBlockNonCetBinariesLogged \n";
		if (Reserved1 != 0) ss << std::setw(50) << "Reserved1 \n";
		if (Reserved2 != 0) ss << std::setw(50) << "Reserved2 \n";
		if (Reserved3 != 0) ss << std::setw(50) << "Reserved3 \n";
		if (Reserved4 != 0) ss << std::setw(50) << "Reserved4 \n";
		if (Reserved5 != 0) ss << std::setw(50) << "Reserved5 \n";
		if (CetDynamicApisOutOfProcOnly != 0) ss << std::setw(50) << "CetDynamicApisOutOfProcOnly \n";
		if (UserCetSetContextIpValidationRelaxedMode != 0) ss << std::setw(50) << "UserCetSetContextIpValidationRelaxedMode \n";

		return ss.str();
	}
};



DEFINE_CMD(pses)
{
    try
    {
        ExtRemoteTypedList pses_list = ExtNtOsInformation::GetKernelProcessList();

        for (pses_list.StartHead(); pses_list.HasNode(); pses_list.Next())
        {
            /*m_Control->ControlledOutput(DEBUG_OUTCTL_ALL_OTHER_CLIENTS
                , DEBUG_OUTPUT_NORMAL, "ps\n");*/
            dump_process(pses_list.GetNodeOffset());
        }
    }FC;
}

DEFINE_CMD(ps_flags)
{
	size_t addr = EXT_F_IntArg(args, 1, 0);
	dump_ps_flags(addr);
}

DEFINE_CMD(kill)
{
	size_t proc_addr = EXT_F_IntArg(args, 1, curr_proc());
	kill_process(proc_addr);
}

DEFINE_CMD(process)
{
	size_t proc_addr = EXT_F_IntArg(args, 1, curr_proc());
	dump_process(proc_addr);
}

void dump_process(size_t process_addr)
{
	try
	{
		ExtRemoteTyped ps("(nt!_EPROCESS*)@$extin", process_addr);
		size_t name_addr = process_addr + ps.GetFieldOffset("ImageFileName");
		char name[16] = { 0, };
		EXT_D_IDebugDataSpaces->ReadVirtual(name_addr, name, 15, NULL);
		size_t pid = ps.Field("UniqueProcessId").GetUlongPtr();
		uint8_t protection = s_CmdExt->read<uint8_t>(process_addr + ps.GetFieldOffset("Protection"));
		uint8_t signing_level = s_CmdExt->read<uint8_t>(process_addr + ps.GetFieldOffset("SignatureLevel"));
		uint8_t dll_signing_level = s_CmdExt->read<uint8_t>(process_addr + ps.GetFieldOffset("SectionSignatureLevel"));
		size_t token_addr = ps.Field("Token.Object").GetUlong64();

		size_t pico_context = ps.Field("PicoContext").GetUlongPtr();
		//	size_t trustlet_id = ps.Field("TrustletIdentity").GetUlong64();

		uint32_t flags = ps.Field("Flags").GetUlong();
		uint32_t flags2 = ps.Field("Flags2").GetUlong();
		uint32_t flags3 = ps.Field("Flags3").GetUlong();

		uint32_t m_flags = ps.Field("MitigationFlags").GetUlong();
		uint32_t m_flags2 = ps.Field("MitigationFlags2").GetUlong();

		size_t silo_addr = get_process_silo(process_addr);

		PS_FLAGS ps_flags(flags);
		PS_FLAGS2 ps_flags2(flags2);
		PS_FLAGS3 ps_flags3(flags3);
		MITIGATION_FLAGS ps_mflags(m_flags);
		MITIGATION_FLAGS2 ps_mflags2(m_flags2);

		bool b_minimal = (ps_flags3.Minimal != 0);
		bool b_cfg = (ps_mflags.ControlFlowGuardEnabled != 0);
		bool b_acg = (ps_mflags.DisableDynamicCode != 0);
		bool b_win32k_filter = (ps_mflags.EnableFilteredWin32kAPIs != 0);
		bool b_no_win32k = (ps_mflags.DisallowWin32kSystemCalls != 0);
		bool b_spectre = (ps_mflags.RestrictIndirectBranchPrediction != 0);
		bool b_import_filter = (ps_mflags2.EnableImportAddressFilter != 0);
		bool b_export_filter = (ps_mflags2.EnableExportAddressFilter != 0);


		size_t dir_table_base = ps.Field("Pcb.DirectoryTableBase").GetUlong64();

		std::string il_str = getTokenIL(token_addr & 0xFFFFFFFFFFFFFFF0).c_str();

		//Out("\tPico: 0x%I64x, Trustlet ID: 0x%I64x\t, Minimal: %s\t", pico_context, trustlet_id, b_minimal ? "T":" ");

		std::stringstream ss;

		ss << std::hex << std::showbase
			<< "<link cmd=\"!process " << process_addr << "\">" << process_addr << "</link> "
			<< "<link cmd=\"dt nt!_EPROCESS " << process_addr << "\">dt</link> ";

		char il = ' ';
		if (!il_str.empty())
		{
			il = *(++il_str.rbegin());
		}

		if (il == '0' || il == '1' || il == '2')
			ss << "<link cmd=\".kill " << process_addr << ";g;\">kill</link> ";
		else
			ss << "     ";

		ss << "<link cmd=\"!dk handles " << process_addr << "\">handles</link> ";

		ss << "<link cmd=\"!dk token " << (token_addr & 0xFFFFFFFFFFFFFFF0) << "\">token</link> ";

		ss << "<link cmd=\"!dk obj " << process_addr - 0x30 << "\">detail</link> ";

		ss << "<link cmd=\"!dk threads " << process_addr << "\">threads</link> ";

		ss << "<link cmd=\".process /i " << process_addr << "\">switch</link> ";

		//ss << il_str << " " << il_str.length() << " ";

		if (silo_addr != NULL)
		{
			ss << "silo: "
				<< std::setw(18) << silo_addr << " ";
		}
		else
		{
			ss << "silo: "
				<< std::setw(24) << " ";
		}


		ss << std::setw(16) << name << " "
			<< std::setw(8) << std::noshowbase << std::hex << pid << "(" << std::dec << std::setw(8) << pid << ")   " << std::hex
			<< std::setw(2) << (uint16_t)protection << "(" << std::setw(2) << (uint16_t)signing_level << ", " << std::setw(2) << (uint16_t)dll_signing_level << ") "
			<< std::setw(16) << getProtectionText(protection) << " "
			<< std::setw(14) << getTokenIL(token_addr & 0xFFFFFFFFFFFFFFF0) << " "
			<< std::showbase << std::setw(12) << dir_table_base << "  "
			<< "<link cmd=\"!dk ps_flags " << std::showbase << std::hex << process_addr << "\">Flags</link> " << std::showbase;

		if (pico_context != 0)
			ss << " Pico: " << pico_context << " ";

		//if (trustlet_id != 0)
		//	ss << " Trustlet Id: " << trustlet_id << " ";

		if (b_minimal)			ss << " Minimal ";
		if (b_cfg)  			ss << " CFG ";
		if (b_acg)              ss << " ACG ";
		if (b_win32k_filter)    ss << " Win32k_Filter ";
		if (b_no_win32k)        ss << " No_Win32k ";
		if (b_spectre)          ss << " Spectre ";
		if (b_import_filter)    ss << " Import_Filter ";
		if (b_export_filter)    ss << " Export_Filter ";

		ss << std::endl;


		//if (!il_str.empty())
		//{
		//    auto il = *(++il_str.rbegin());
		//    if (il == '0' || il == '1' || il == '2')
		//        Dml("<link cmd=\"!process 0x%0I64x\">0x%0I64x</link> <link cmd=\".kill 0x%0I64x;g;\">kill</link> %8x %02x(%02x, %02x) %20s %40s %50s %0I64x\n",
		//            process_addr, process_addr, process_addr, pid, protection, signing_level, dll_signing_level, name,
		//            getProtectionText(protection).c_str(),
		//            getTokenIL(token_addr & 0xFFFFFFFFFFFFFFF0).c_str(),
		//            dir_table_base);
		//    else
		//    {
		//        Dml("<link cmd=\"!process 0x%0I64x\">0x%0I64x</link>      %8x %02x(%02x, %02x) %20s %40s %50s %0I64x\n",
		//            process_addr, process_addr, pid, protection, signing_level, dll_signing_level, name,
		//            getProtectionText(protection).c_str(),
		//            getTokenIL(token_addr & 0xFFFFFFFFFFFFFFF0).c_str(),
		//            dir_table_base);
		//    }
		//}
		//else
		//{
		//    Dml("<link cmd=\"!process 0x%0I64x\">0x%0I64x</link>      %8x %02x(%02x, %02x) %20s %40s %50s %0I64x\n",
		//        process_addr, process_addr, pid, protection, signing_level, dll_signing_level, name,
		//        getProtectionText(protection).c_str(),
		//        getTokenIL(token_addr & 0xFFFFFFFFFFFFFFF0).c_str(),
		//        dir_table_base);
		//}

		EXT_F_DML(ss.str().c_str());
	}
	FC;
}

size_t get_process_silo(size_t process_addr)
{
	try
	{
		ExtRemoteTyped proc("(nt!_EPROCESS*)@$extin", process_addr);

		size_t job_addr = proc.Field("Job").GetUlongPtr();

		size_t silo_addr = get_job_silo(job_addr);

		return silo_addr;
	}
	FC;

	return NULL;
}

size_t get_job_silo(size_t job_addr)
{
	try
	{
		while (job_addr != NULL)
		{
			ExtRemoteTyped job("(nt!_EJOB*)@$extin", job_addr);

			uint32_t flags = job.Field("JobFlags").GetUlong();

			if ((flags & (1 << 30)) != 0)
			{
				return job_addr;
			}

			job_addr = job.Field("ParentJob").GetUlongPtr();
		};
	}
	FC;
	return NULL;
}

std::string getTokenIL(size_t token_addr)
{
	try
	{
		ExtRemoteTyped token("(nt!_TOKEN*)@$extin", token_addr);
		size_t il_sid_addr = token.Field("IntegrityLevelSidValue").GetUlongPtr();
		if (il_sid_addr != 0)
			return get<1>(dump_sid(il_sid_addr));

		size_t il_index = token.Field("IntegrityLevelIndex").GetUlong();
		size_t il_addr = get_sid_attr_hash_item(token_addr + token.GetFieldOffset("SidHash"), il_index);

		auto tupleIl = dump_sid(il_addr);
		return get<1>(tupleIl);
	}
	FC;

	return "[Error IL]";
}

std::string getProtectionText(uint8_t protection)
{
	std::string text;

	switch (protection & 0x0F)
	{
	case 0x01:
		text += "[PPL ";
		break;
	case 0x02:
		text += "[PP  ";
		break;
	default:
		return "";
		break;
	}

	switch (protection & 0xF0)
	{
	case 0x00:
		text += "None]";
		break;
	case 0x10:
		text += "AuthCode]";
		break;
	case 0x20:
		text += "CodeGen]";
		break;
	case 0x30:
		text += "AntiMal]";
		break;
	case 0x40:
		text += "Lsa]";
		break;
	case 0x50:
		text += "Windows]";
		break;
	case 0x60:
		text += "Tcb]";
		break;
	case 0x70:
		text += "System]";
		break;
	default:
		return "";
		break;
	}
	return text;
}

size_t curr_proc()
{
	size_t curr_proc_addr = 0;
	EXT_D_IDebugSystemObjects->GetCurrentProcessDataOffset(&curr_proc_addr);
	return curr_proc_addr;
}

size_t curr_thread()
{
	size_t curr_thread_addr = 0;
	EXT_D_IDebugSystemObjects->GetCurrentThreadDataOffset(&curr_thread_addr);
	return curr_thread_addr;
}

size_t curr_tid()
{
	try
	{
		ExtRemoteTyped curr_ethread("(nt!_ETHREAD*)@$extin", curr_thread());
		return curr_ethread.Field("Cid.UniqueThread").GetUlong64();
	}
	FC;

	return 0;
}

size_t curr_token()
{
	try
	{
		ExtRemoteTyped ps("(nt!_EPROCESS*)@$extin", curr_proc());
		size_t token_addr = ps.Field("Token.Object").GetUlong64();

		return token_addr & 0xFFFFFFFFFFFFFFF0;
	}
	FC;

	return 0;
}

void dump_ps_flags(size_t addr)
{
	try
	{
		std::stringstream ss;

		ExtRemoteTyped ps("(nt!_EPROCESS*)@$extin", addr);

		uint32_t flags = ps.Field("Flags").GetUlong();
		uint32_t flags2 = ps.Field("Flags2").GetUlong();
		uint32_t flags3 = ps.Field("Flags3").GetUlong();
		uint32_t m_flags = ps.Field("MitigationFlags").GetUlong();
		uint32_t m_flags2 = ps.Field("MitigationFlags2").GetUlong();

		PS_FLAGS ps_flags(flags);
		PS_FLAGS2 ps_flags2(flags2);
		PS_FLAGS3 ps_flags3(flags3);
		MITIGATION_FLAGS ps_mflags(m_flags);
		MITIGATION_FLAGS2 ps_mflags2(m_flags2);

		dump_process(addr);

		ss << std::string(50, '-') << std::setw(12) << "Flags: [ " << std::hex << std::showbase << std::setw(10) << flags << " ]" << std::string(50, '-') << std::endl
			<< ps_flags.str() << std::endl;
		ss << std::string(50, '-') << std::setw(12) << "Flags2: [" << std::hex << std::showbase << std::setw(10) << flags2 << " ]" << std::string(50, '-') << std::endl
			<< ps_flags2.str() << std::endl;
		ss << std::string(50, '-') << std::setw(12) << "Flags3: [" << std::hex << std::showbase << std::setw(10) << flags3 << " ]" << std::string(50, '-') << std::endl
			<< ps_flags3.str() << std::endl;
		ss << std::string(50, '-') << std::setw(12) << "Mitigation Flags: [" << std::hex << std::showbase << std::setw(10) << m_flags << " ]" << std::string(50, '-') << std::endl
			<< ps_mflags.str() << std::endl;
		ss << std::string(50, '-') << std::setw(12) << "Mitigation Flags2: [" << std::hex << std::showbase << std::setw(10) << m_flags2 << " ]" << std::string(50, '-') << std::endl
			<< ps_mflags2.str() << std::endl;

		EXT_F_OUT(ss.str().c_str());
	}
	FC;
}

size_t get_cr3()
{
	try
	{
		size_t proc_addr = curr_proc();

		size_t cr3 = EXT_F_READ<size_t>(proc_addr + 0x28);

		return cr3;
	}
	FC;

	return 0;
}

void kill_process(size_t proc_addr)
{
	try
	{
		std::stringstream ss;
		ss << ".kill " << std::hex << std::showbase << proc_addr << ";g;";

		EXT_F_EXEC(ss.str().c_str());
	}
	FC;
}
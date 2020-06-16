#pragma once

#include<Windows.h>


class TrampolineHook
{
public:
	TrampolineHook(uintptr_t originalAddress, uintptr_t hookAddress, int prologueSize)
	{
		m_prologueSize = prologueSize;
		InsertTrampolineHook(originalAddress, hookAddress);
	}
	uintptr_t getGatewayAddress()
	{
		return gatewayAddress;
	}
	void UnHook()
	{
		PatchDetour();
	}
private:
	void InsertDetour(uintptr_t originalFunctionAddress, uintptr_t hookFunctionAddress)
	{
		DWORD oldProt;
		VirtualProtect((void*)originalFunctionAddress, m_prologueSize, PAGE_EXECUTE_READWRITE, &oldProt);
		memset((void*)originalFunctionAddress, 0x90, m_prologueSize);
		uintptr_t relativeDetourJumpAddress = hookFunctionAddress - originalFunctionAddress - 5;	//Relative address to get from original function to hooked one.
		*(char*)(originalFunctionAddress) = 0xE9;
		*(uintptr_t*)(originalFunctionAddress + 1) = relativeDetourJumpAddress;
		VirtualProtect((void*)originalFunctionAddress, m_prologueSize, oldProt, NULL);
	}

	void InsertTrampolineHook(uintptr_t originalFunctionAddress, uintptr_t hookFunctionAddress)
	{
		memcpy(originalBytes, (void*)originalFunctionAddress, m_prologueSize);

		char* gateway = (char*)VirtualAlloc(0, m_prologueSize + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);	//Allocate gateway that is accessible from any other process (PAGE_EXECUTE_READWRITE)
		memcpy(gateway, (void*)originalFunctionAddress, m_prologueSize);	//Write the stolen bytes that we replace with detour into gateway. 

		uintptr_t relativeGatewayAddress = (uintptr_t)((originalFunctionAddress) - ((uintptr_t)gateway) - 5);	//Prologue size is the same so no need to add into calculation (as this is a relative direct jump), -5 at the end as jump is relative to next instruction. 
		*(gateway + m_prologueSize) = 0xE9;	//0xE9 = JMP
		*(uintptr_t*)(gateway + m_prologueSize + 1) = relativeGatewayAddress; //Insert relative address after, where the jump goes

		InsertDetour(originalFunctionAddress, hookFunctionAddress);
		gatewayAddress = (uintptr_t)gateway;
		originalAddress = originalFunctionAddress;
		hookAddress = hookFunctionAddress;

	}

	void PatchDetour()
	{
		DWORD oldProt;
		VirtualProtect((void*)originalAddress, m_prologueSize, PAGE_EXECUTE_READWRITE, &oldProt);
		memcpy((void*)originalAddress, originalBytes, m_prologueSize);
		VirtualProtect((void*)originalAddress, m_prologueSize, oldProt, NULL);
	}

	int m_prologueSize;
	BYTE originalBytes[32];
	uintptr_t originalAddress;
	uintptr_t hookAddress;
	uintptr_t gatewayAddress;
};

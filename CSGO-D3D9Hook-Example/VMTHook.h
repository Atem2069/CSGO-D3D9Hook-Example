#pragma once

#include <Windows.h>

class VMTHook
{
public:
	VMTHook(DWORD** dwVMTPtr)
	{
		vmtPointer = dwVMTPtr;
		oldVMT = *dwVMTPtr;	//Grab the original VMT by dereferencing base VMT pointer.
		GetVMTSize(oldVMT);	//Need to figure out how big the VMT is to be able to memcpy and check if indices are safe
		newVMT = new DWORD[dwSize];
		memcpy(newVMT, oldVMT, sizeof(DWORD)*dwSize);	//^^ copy over old VMT contents to new one (we own the new VMT so we don't have to VirtualProtect to modify it)

		*vmtPointer = newVMT;	//Now our new VMT will be used when virtual methods are called - so we can make our hooks.
	}

	DWORD Hook(DWORD funcBase, int index)
	{
		if (index >= 0 && index <= dwSize)
		{
			newVMT[index] = funcBase;	//Replace address with address of hooked function
			return oldVMT[index];		//We return the address of the original function - as we want to call the original one after our hook completes
		}

		return NULL;
	}

	void UnHook()
	{
		*vmtPointer = oldVMT;			//Simply swap what VMT is being used with the old one
	}

	void ReHook()
	{
		*vmtPointer = newVMT;			//Similar to above, but if we want to redo our hooks
	}

private:
	DWORD** vmtPointer;	//Pointer to VMT
	DWORD* newVMT;		//New VMT which we point to - this is where hooks are done
	DWORD* oldVMT;		//Old VMT - kept so it can be restored when we unhook
	DWORD dwSize;

	void GetVMTSize(PDWORD vmt)
	{
		DWORD dwIndex = 0;
		for (dwIndex = 0; vmt[dwIndex]; dwIndex++)
		{
			if (IS_INTRESOURCE((FARPROC)vmt[dwIndex]))	//Return when we get an invalid pointer
				break;
		}
		dwSize = dwIndex;
	}
};

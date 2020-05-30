#include <iostream>
#include <Windows.h>

#include<d3d9.h>
#include<d3dx9.h>
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")

#include "VMTHook.h"

//Simple text drawing with d3dx9
ID3DXFont* font = nullptr;

void drawText(IDirect3DDevice9* device, int xPos, int yPos, LPCSTR msg)
{
	if (!font)
		D3DXCreateFont(device, 17, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &font);

	RECT rect;
	rect.bottom = yPos + 200;
	rect.top = yPos;
	rect.left = xPos;
	rect.right = 1024;

	font->DrawTextA(NULL, msg, -1, &rect, 0, D3DCOLOR_ARGB(255, 0, 0, 255));
}


typedef HRESULT(APIENTRY* EndSceneFn)(LPDIRECT3DDEVICE9); //https://docs.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-endscene
EndSceneFn originalEndScene = nullptr;	//Use the above function prototype so we can store the address of original EndScene, and call it later

void APIENTRY hookedEndScene(IDirect3DDevice9* pDevice)
{
	//Our hooked EndScene - now we can draw stuff (D3DX9 from the DirectX SDK if you want to do text and stuff without hassle)
	D3DRECT clearRect{ 100,100,300,300 };
	pDevice->Clear(1, &clearRect, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 255, 0, 0), 0, 0);

	drawText(pDevice, 0, 0, "D3D9 Hook Test");

	originalEndScene(pDevice);	//Have to call original endScene though at the end
}


DWORD hackMain(HMODULE hModule)
{
	HWND csgoHwnd = FindWindow(NULL, "Counter-Strike: Global Offensive");	//There are better ways of finding the hwnd that CSGO uses, but this is simple enough. Should probably be in windowed mode to inject or this may fail^^

	DWORD dwppDirect3DDevice9 = 0xA7030;	//You may want to sigscan this or update the offset. Signatures and offsets can be found in hazedumper on github

	DWORD shaderAPIBase = (DWORD)GetModuleHandle("shaderapidx9.dll");

	IDirect3DDevice9* d3d9device = *(IDirect3DDevice9**)(shaderAPIBase + dwppDirect3DDevice9);	//If you use a detour or trampoline hook, then the dummy device method is the way to go. A plain VMT hook works if you know where the device is. 

	DWORD** d3d9Vtable = (DWORD**)d3d9device;
	VMTHook* d3d9DeviceHook = new VMTHook(d3d9Vtable);	//Get vtable from d3d9 device and use it to initialize our VMT hook instance

	originalEndScene = (EndSceneFn)d3d9DeviceHook->Hook((DWORD)hookedEndScene, 42);	//Pass through address of our hooked method - and the VMT index of EndScene, which is 42.
	if (!originalEndScene)
	{
		MessageBoxA(NULL, "Failed to hook EndScene.. ", "Error", MB_ICONERROR | MB_OK);
		FreeLibraryAndExitThread(hModule, -1);
	}

	bool shouldQuit = false;
	while (!shouldQuit)
	{
		//Keep thread alive - probably better way of doing this.

		if (GetAsyncKeyState(VK_NUMPAD0))
			shouldQuit = true;
	}


	d3d9DeviceHook->UnHook(); //Unhook and delete our VMT hook instance before we quit, otherwise CSGO will crash
	delete d3d9DeviceHook;

	Sleep(1000);
	FreeLibraryAndExitThread(hModule, 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hModule);
	if (fdwReason == DLL_PROCESS_ATTACH)
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)hackMain, hModule, 0, nullptr);	//run hackMain where we will do d3d9 hook
	return TRUE;
}

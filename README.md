gdetour
-------

gdetour is a barebones detour API. It allows for redirection of code execution via very basic assembly jumps.

It is similiar to several other projects:

* [Microsoft Detours](http://research.microsoft.com/en-us/projects/detours/)
* [EasyHook](http://code.google.com/p/easyhook-continuing-detours/)
* etc

Features
--------

* Only one callback no matter what detour is hit
* Can detour functions without knowing the types of the arguments
* No function typedef for every detour
* Is thread safe for incoming calls (applying / removing detours is not thread safe)
* Allows 'infinite' recursion. All details about the detour's state is stored on the stack.
* Allows inspection / modification of every register. No registers are overwritten.

Usage
-----
To create a detour, only three pieces of information need to be known:
* Function address
* Number of bytes that should be overwritten at address (must be 5 or greater)
* Number of bytes to pop on return (related to calling convention - mostly for stdcall)

Known Issues
------------
* Only supports x86 platforms (notably, no x64 support)
* Currently only supports Windows (uses VirtualProtect, etc)

i try implement generic interface - task.h

swe.dll exported 2 functions

HRESULT CreateTaskMngr(_Out_ IExecTask** ppExec);
HWND CreateShlWnd(_In_ HWND hWndParent, _In_ int nWidth, _In_ int nHeight);

example of usage - actx.exe ( AAA\mdi.cpp )

first need call CreateTaskMngr

IExecTask* pExec;

if (0 <= CreateTaskMngr(&pExec))
{
  if (0 <= pExec->Start())
  {
     ...
     pExec->Stop();
  }
  pExec->Release();
}

for start new process
1) pExec->Exec(app, cmd, dir, &pi)
2) if 1 ok, create self window (hWndParent) and my hwnd = CreateShlWnd(hWndParent, nWidth, nHeight);
3) if 2 ok, call pExec->EmbedTask(hwnd, &pi);
4) if 1 ok, call pExec->Cleanup(&pi);

if you close self window - as result my child window will be destroóed and all processes started by Exec will be terminated
if all processes started by Exec terminated - i close self window and post WM_CLOSE to your

about focus and visual effects - here exist unresolved problem. in normal case - if child window active - parent window is active too. if it is  frame - it is draw active caption, etc
but in case cross-thread parent-child - this is became false. if child active - parent not active and visa versa. activation of child (via mouse click) not activated parent. this good visible in my demo when we have 2 mdi window, both cross thread parent-child. usually if we click in bottom window (in client (child)) - this became top and overwrite second (initialy on top) window.
but here no effect - because activation of child not activate parent. need click direct on frame, for change frames z-order
from another side - of synthetic activate parent - child loses focus/caret, despite mouse activation before
if we focus parent - child not get focus. SetFocus will not work here and this is documented ("The window must be attached to the calling thread's message queue.")
again we can instead SetFocus use SetForegroundWindow or BringWindowToTop for activate child (which is really top level window).this work, but parent in this case lost activation and by fact became not working ( for instanse stop work move,size,close..) 
so any solution is bad, because for good solution need that BOTH parent and child will be active, but this is absolutely impossible by design
the current case, how it work now, think the almost best of possible and with only generic code

of course the best choise (like you this or not) - not embed someone else's window into your own. use only program for start another prorgams with parameters, spy on it end, but not try embed it gui, because this is wrong by design

this how i view situation

/*
ctrlobj.cpp

���������� ���������� ���������, ������� ��������� ���������� � ����

*/

/* Revision: 1.29 14.06.2001 $ */

/*
Modify:
  14.06.2001 OT
    ! "����" ;-)
  30.05.2001 OT
    ! ������� ���������� �� ��������������� ����� ������
  16.05.2001 DJ
    ! proof-of-concept
  15.05.2001 OT
    ! NWZ -> NFZ
  12.05.2001 DJ
    ! FrameManager ������� �� CtrlObject
    ! ���������� ��������� �� CtrlObject �������� ����
  11.05.2001 OT
    ! ��������� Background
  07.05.2001 SVS
    ! SysLog(); -> _D(SysLog());
  06.05.2001 DJ
    ! �������� #include
  06.05.2001 ��
    ! �������������� Window � Frame :)
  05.05.2001 DJ
    + �������� NWZ
  29.04.2001 ��
    + ��������� NWZ �� ����������
  28.04.2001 VVM
    + KeyBar ���� ����� ������������ �������.
  22.04.2001 SVS
    ! �������� ������� - ����� �������� ���� �������� ��������
  02.04.2001 VVM
    + ��������� Opt.FlagPosixSemantics
  28.02.2001 IS
    ! �.�. CmdLine ������ ���������, �� ���������� ������
      "CmdLine." �� "CmdLine->" � ���������� ��������/������ �� � ������������
      � ����������� CtrlObject.
  09.02.2001 IS
    + ����������� ��������� ����� "���������� ������"
  09.01.2001 SVS
    + ����� ������� Opt.ShiftsKeyRules (WaitInFastFind)
  29.12.2000 IS
    + ��������� ��� ������, ��������� �� ��� ���������� �����. ���� ���, ��
      �� ������� �� ����.
  15.12.2000 SVS
    ! ����� ShowCopyright - public static & �������� Flags.
      ���� Flags&1, �� ������������ printf ������ ���������� �������
  25.11.2000 SVS
    ! Copyright � 2 ������
  27.09.2000 SVS
    ! Ctrl-Alt-Shift - ���������, ���� ����.
  19.09.2000 IS
    ! ��������� ������� �� ctrl-l|q|t ������ �������� �������� ������
  19.09.2000 SVS
    + Opt.PanelCtrlAltShiftRule ������ ��������� Ctrl-Alt-Shift ��� �������.
  19.09.2000 SVS
    + ��������� ������� ������ ���������� � ������� �� CtrlAltShift
  07.09.2000 tran 1.05
    + Current File
  15.07.2000 tran
    + � � ��� �������� :) ����� ����� ����� Redraw
  13.07.2000 SVS
    ! ��������� ��������� �� ���������� ���� ;-)
  11.07.2000 SVS
    ! ��������� ��� ����������� ���������� ��� BC & VC
  29.06.2000 tran
    ! �������� � ��������� ���������� �� copyright.inc
  25.06.2000 SVS
    ! ���������� Master Copy
    ! ��������� � �������� ���������������� ������
*/

#include "headers.hpp"
#pragma hdrstop

#include "ctrlobj.hpp"
#include "global.hpp"
#include "fn.hpp"
#include "lang.hpp"
#include "manager.hpp"
#include "cmdline.hpp"
#include "hilight.hpp"
#include "grpsort.hpp"
#include "poscache.hpp"
#include "history.hpp"
#include "treelist.hpp"
#include "filter.hpp"
#include "filepanels.hpp"

ControlObject *CtrlObject;

ControlObject::ControlObject()
{
  _OT(SysLog("[%p] ControlObject::ControlObject()", this));
  FPanels=0;
  CtrlObject=this;
  /* $ 06.05.2001 DJ
     ������� ����������� (��� ���������� dependencies)
  */
  HiFiles = new HighlightFiles;
  GrpSort = new GroupSort;
  ViewerPosCache = new FilePositionCache;
  EditorPosCache = new FilePositionCache;
  FrameManager = new Manager;
  /* DJ $ */
  ReadConfig();
  /* $ 28.02.2001 IS
       �������� ����������� ������ ����� ����, ��� ��������� ���������
  */
  CmdLine=new CommandLine;
  /* IS $ */
  CmdHistory=new History("SavedHistory",&Opt.SaveHistory,FALSE,FALSE);
  FolderHistory=new History("SavedFolderHistory",&Opt.SaveFoldersHistory,FALSE,TRUE);
  ViewHistory=new History("SavedViewHistory",&Opt.SaveViewHistory,TRUE,TRUE);
  FolderHistory->SetAddMode(TRUE,2,TRUE);
  ViewHistory->SetAddMode(TRUE,Opt.FlagPosixSemantics?1:2,TRUE);
  if (Opt.SaveHistory)
    CmdHistory->ReadHistory();
  if (Opt.SaveFoldersHistory)
    FolderHistory->ReadHistory();
  if (Opt.SaveViewHistory)
    ViewHistory->ReadHistory();
  RegVer=-1;
}


void ControlObject::Init()
{
  TreeList::ClearCache(0);
  PanelFilter::InitFilter();

  SetColor(F_LIGHTGRAY|B_BLACK);
  GotoXY(0,ScrY-3);
  while (RegVer==-1)
    Sleep(0);
  ShowCopyright();
  GotoXY(0,ScrY-2);

  char TruncRegName[512];
  strcpy(TruncRegName,RegName);
  char *CountPtr=strstr(TruncRegName," - (");
  if (CountPtr!=NULL && isdigit(CountPtr[4]) && strchr(CountPtr+5,'/')!=NULL &&
      strchr(CountPtr+6,')')!=NULL)
    *CountPtr=0;
  if (RegVer)
    mprintf("%s: %s",MSG(MRegistered),TruncRegName);
  else
    Text(MShareware);

  CmdLine->SaveBackground(0,0,ScrX,ScrY);

  FPanels=new FilePanels();
  FPanels->Init();
  this->MainKeyBar=&(FPanels->MainKeyBar);
  this->TopMenuBar=&(FPanels->TopMenuBar);
  FPanels->SetScreenPosition();

  _beginthread(CheckVersion,0x10000,NULL);
  Cp()->LeftPanel->Update(0);
  Cp()->RightPanel->Update(0);

  /* $ 07.09.2000 tran
    + Config//Current File */
  if (Opt.AutoSaveSetup)
  {
      Cp()->LeftPanel->GoToFile(Opt.LeftCurFile);
      Cp()->RightPanel->GoToFile(Opt.RightCurFile);
  }
  /* tran 07.09.2000 $ */
  FrameManager->InsertFrame(FPanels);
  Plugins.LoadPlugins();
}

void ControlObject::CreateFilePanels()
{
  FPanels=new FilePanels();
}

ControlObject::~ControlObject()
{
  _OT(SysLog("[%p] ControlObject::~ControlObject()", this));
  if (Cp()->ActivePanel!=NULL)
  {
    if (Opt.AutoSaveSetup)
      SaveConfig(0);
    if (Cp()->ActivePanel->GetMode()!=PLUGIN_PANEL)
    {
      char CurDir[NM];
      Cp()->ActivePanel->GetCurDir(CurDir);
      FolderHistory->AddToHistory(CurDir,NULL,0);
    }
  }
  if (Opt.SaveEditorPos)
    EditorPosCache->Save("Editor\\LastPositions");
  if (Opt.SaveViewerPos)
    ViewerPosCache->Save("Viewer\\LastPositions");

  FrameManager->CloseAll();
  FPanels=NULL;

  Plugins.SendExit();
  PanelFilter::CloseFilter();
  delete CmdHistory;
  delete FolderHistory;
  delete ViewHistory;
  delete CmdLine;
  /* $ 06.05.2001 DJ
     ������� ��, ��� ������� �����������
  */
  delete HiFiles;
  delete GrpSort;
  delete ViewerPosCache;
  delete EditorPosCache;
  delete FrameManager;
  /* DJ $ */
  Lang.Close();
  CtrlObject=NULL;
}


/* $ 25.11.2000 SVS
   Copyright � 2 ������
*/
/* $ 15.12.2000 SVS
 ����� ShowCopyright - public static & �������� Flags.
*/
void ControlObject::ShowCopyright(DWORD Flags)
{
/* $ 29.06.2000 tran
  ����� char *CopyRight �� inc ����� */
#include "copyright.inc"
/* tran $ */
  char Str[256];
  char *Line2=NULL;
  strcpy(Str,Copyright);
  char Xor=17;
  for (int I=0;Str[I];I++)
  {
    Str[I]=(Str[I]&0x7f)^Xor;
    Xor^=Str[I];
    if(Str[I] == '\n')
    {
      Line2=&Str[I+1];
      Str[I]='\0';
    }
  }
  if(Flags&1)
  {
    fprintf(stderr,"%s\n%s\n",Str,Line2);
  }
  else
  {
#ifdef BETA
    mprintf("Beta version %d.%02d.%d",BETA/1000,(BETA%1000)/10,BETA%10);
#else
    if(Line2)
    {
      GotoXY(0,ScrY-4);
      Text(Str);
      GotoXY(0,ScrY-3);
      Text(Line2);
    }
    else
      Text(Str);
#endif
  }
}
/* SVS $ */
/* SVS $ */


FilePanels* ControlObject::Cp()
{
    if ( FPanels==0 )
    {
        Message(MSG_WARNING,1,MSG(MError),"CtrlObject::Cp(), FPanels==0",MSG(MOk));
    }
    return FPanels;
}

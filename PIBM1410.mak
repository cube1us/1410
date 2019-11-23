# ---------------------------------------------------------------------------
VERSION = BCB.03
# ---------------------------------------------------------------------------
!ifndef BCB
BCB = $(MAKEDIR)\..
!endif
# ---------------------------------------------------------------------------
PROJECT = PIBM1410.exe
OBJFILES = PIBM1410.obj UI14101.obj UI1415IO.obj ubcd.obj UI1415L.obj UI1410CPUT.obj \
  UI1410CPU.obj UI1410PWR.obj UI1410DEBUG.obj UI1415CE.obj UI1410INST.obj \
  UI1410ARITH.obj UI1410DATA.obj UI1410BRANCH.obj UI1410MISC.obj \
  UI1410CHANNEL.obj UITAPEUNIT.obj UITAPETAU.obj UI729TAPE.obj UI1403.obj \
  UIPRINTER.obj UI1402.obj UIREADER.obj UIHOPPER.obj UIPUNCH.obj
RESFILES = PIBM1410.res
RESDEPEN = $(RESFILES) UI14101.dfm UI1415IO.dfm UI1415L.dfm UI1410PWR.dfm UI1410DEBUG.dfm \
  UI1415CE.dfm UI729TAPE.dfm UI1403.dfm UI1402.dfm
LIBFILES =
LIBRARIES = vcldbx35.lib vcldb35.lib vclx35.lib vcl35.lib
SPARELIBS = vcl35.lib vclx35.lib vcldb35.lib vcldbx35.lib
PACKAGES = VCLX35.bpi VCL35.bpi VCLDB35.bpi VCLDBX35.bpi bcbsmp35.bpi dclocx35.bpi \
  QRPT35.bpi TEEUI35.bpi TEEDB35.bpi TEE35.bpi ibsmp35.bpi NMFAST35.bpi \
  INETDB35.bpi INET35.bpi
PATHASM = .;
PATHCPP = .;
PATHPAS = .;
PATHRC = .;
DEBUGLIBPATH = $(BCB)\lib\debug
RELEASELIBPATH = $(BCB)\lib\release
DEFFILE =
# ---------------------------------------------------------------------------
CFLAG1 = -Od -Hc -w -Ve -r- -k -y -v -vi- -c -b- -w-par -w-inl -Vx
CFLAG2 = -If:\pro\borland\cbuilder3\projects;$(BCB)\projects;$(BCB)\include;$(BCB)\include\vcl \
  -H=$(BCB)\lib\vcld.csm
CFLAG3 =
PFLAGS = -AWinTypes=Windows;WinProcs=Windows;DbiTypes=BDE;DbiProcs=BDE;DbiErrs=BDE \
  -Uf:\pro\borland\cbuilder3\projects;$(BCB)\projects;$(BCB)\lib\obj;$(BCB)\lib;$(DEBUGLIBPATH) \
  -If:\pro\borland\cbuilder3\projects;$(BCB)\projects;$(BCB)\include;$(BCB)\include\vcl \
  -$Y -$W -$O- -v -JPHNV -M
RFLAGS = -if:\pro\borland\cbuilder3\projects;$(BCB)\projects;$(BCB)\include;$(BCB)\include\vcl
AFLAGS = /if:\pro\borland\cbuilder3\projects /i$(BCB)\projects /i$(BCB)\include \
  /i$(BCB)\include\vcl /mx /w2 /zd
LFLAGS = -Lf:\pro\borland\cbuilder3\projects;$(BCB)\projects;$(BCB)\lib\obj;$(BCB)\lib;$(DEBUGLIBPATH) \
  -aa -Tpe -x -v
IFLAGS =
LINKER = ilink32
# ---------------------------------------------------------------------------
ALLOBJ = c0w32.obj sysinit.obj $(OBJFILES)
ALLRES = $(RESFILES)
ALLLIB = $(LIBFILES) $(LIBRARIES) import32.lib cp32mt.lib
# ---------------------------------------------------------------------------
.autodepend

!ifdef IDEOPTIONS

[Version Info]
IncludeVerInfo=0
AutoIncBuild=0
MajorVer=1
MinorVer=0
Release=0
Build=0
Debug=0
PreRelease=0
Special=0
Private=0
DLL=0
Locale=1033
CodePage=1252

[Debugging]
DebugSourceDirs=

[Parameters]
RunParams=
HostApplication=

!endif

$(PROJECT): $(OBJFILES) $(RESDEPEN) $(DEFFILE)
    $(BCB)\BIN\$(LINKER) @&&!
    $(LFLAGS) +
    $(ALLOBJ), +
    $(PROJECT),, +
    $(ALLLIB), +
    $(DEFFILE), +
    $(ALLRES) 
!

.pas.hpp:
    $(BCB)\BIN\dcc32 $(PFLAGS) { $** }

.pas.obj:
    $(BCB)\BIN\dcc32 $(PFLAGS) { $** }

.cpp.obj:
    $(BCB)\BIN\bcc32 $(CFLAG1) $(CFLAG2) -o$* $* 

.c.obj:
    $(BCB)\BIN\bcc32 $(CFLAG1) $(CFLAG2) -o$* $**

.rc.res:
    $(BCB)\BIN\brcc32 $(RFLAGS) $<
#-----------------------------------------------------------------------------

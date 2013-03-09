UNIT APMLibP;
{$B-}{$I-}

INTERFACE


CONST
	APM_Reboot_Warm   = 1;
	APM_Reboot_Cold   = 2;
	APM_Suspend       = 3;
	APM_CtrlAltDel    = 4;
	APM_ShutDown      = 5;

	APM_Nothing       = 0;
	APM_Not_Installed = 1;
	APM_Error_Flushing= 2;
	APM_Suspend_Ok    = 3;



Function  FlushAllCaches: word;
{ Flushes the existing caches.
  RETURN:  0:    ok
           else: bipmap of caches that couldn't be flushed:
                  bit0: CDBlitz
                  bit1: PC-Cache (this never fails ;-))
                  bit2: Quick Cache
                  bit3: Super PC-Kwik  (PC-Tools, Qualitas QCache, ...)
                  bit4: MS-SmartDrv.EXE (this never fails ;-))
                  bit5: MS-SmartDrv.SYS}

Procedure Reboot (warm: boolean);
{ Reboots the system.
  INPUT:  Warm: indicates if a cold boot or warm boot will be performed
  NOTE:   Caches MUST be flushed before calling this procedure}

Procedure ShutDown;
{ Turns the system off
  NOTES:  - Caches MUST be flushed before calling this procedure
          - This will only work under BIOSes supporting APM v.1.1 or later}

Procedure CtrlAltDel;
{ Performs a Ctrl+Alt+Del
  NOTE:   - Caches MUST be flushed before calling this procedure}

Procedure Suspend;
{ Suspends the system to a status of minimum power until a key is pressed
  NOTE:   - This will only work under BIOSes supporting APM v.1.1 or later}

Function  APMIsInstalled: boolean;
{ Checks if APMIsInstalled
  RETURN:  TRUE if APM is installed}

Procedure APMGetVer (var Max,Min: byte);
{ Returns the version of the APM, if found
  NOTE:  APMIsInstalled MUST be called first, otherwise it won't return
         a correct version number}

Function APMLib (funktion: byte): byte;
{ Generic APMLib function; use the constant for the function and result}



IMPLEMENTATION


Uses
    DOS {$ifdef FPK},Go32{$endif};


Var
   APMVerMax, APMVerMin : byte;

Function FlushAllCaches: word;
Var
   Regs   : Registers;
   Status : word;
   FF     : File;
   SmartDrvBuf : byte;
Begin
     Status := 0;

     {1.- CDBLITZ  }
     With Regs do Begin
          AX := $1500;
          CH := $90;
          BX := $1234;
     End;
     Intr ($2F, Regs);
     If Regs.CX=$1234 Then Begin
        With Regs do Begin
             WriteLn ('Flushing CDBLITZ v.',DH,'.',DL,'...');
             AX := $1500;
             BX := $1234;
             CH := $96;
        End;
        Intr ($2F, Regs);
        If (Regs.Flags and FCarry ) <> 0 Then Status := Status +1 {Error!}
                                     Else WriteLn ('Success');
     End;

     {2.- PC-Cache }
     With Regs do Begin
          AX := $FFA5;
          CX := $1111;
     End;
     Intr ($16, Regs);
     If hi(Regs.CX)=0 Then Begin
        With Regs do Begin
             WriteLn ('Flushing PC-Cache...');
             AX := $FFA5;
             CX := $FFFF;
        End;
        Intr ($16, Regs);
        {In practice, never reports error = 2}
        WriteLn ('Success');
     End;

     {3.- Quick Cache}
     With Regs do Begin
          AH := $27;
          BX := 0;
     End;
     Intr ($13, Regs);
     If (Regs.AX = 0) and (Regs.BX<>0) Then Begin
        With Regs do Begin
             WriteLn ('Flushing Quick Cache v.',BH,'.',BL,'...');
             AX := $21;
        End;
        Intr ($13, Regs);
        If Regs.AX<>0 Then Status := Status + 4 {Error!}
                                     Else WriteLn ('Success');
     End;

     {4.- PC - Kwik: PC-Tools, Qualitas QCache...}
     With Regs do Begin
          AH := $2B;
          CX := $4358;
     End;
     MsDOS (Regs);
     If Regs.AL = 0 Then Begin
        With Regs do Begin
             WriteLn ('Flushing Super PC-Kwik v.',DH,'.',DL,'...');
             AX := $A1;
             SI := $4358;
        End;
        Intr ($13, Regs);
        If (Regs.Flags and FCarry ) <> 0 Then Status := Status +8 {Error!}
                                     Else WriteLn ('Success');
     End;

     {5.- Microsoft SmartDRV.EXE }
     With Regs do Begin
          AX := $4A10;
          BX := 0;
          CX := $EBAB;
     End;
     Intr ($2F, Regs);
     If Regs.AX = $BABE Then Begin
        With Regs do Begin
             WriteLn ('Flushing Microsoft SmardDrv.EXE v.',Hi(BP),'.',Lo(BP),'...');
             AX := $4A01;
             BX := $0001;
        End;
        Intr ($2F, Regs);
        {In practice, never reports error = 16}
        WriteLn ('Success');
     End;

     {6.- Microsoft SmartDrive.SYS}
     Assign (FF,'SMARTAAR');
     ReSet (FF);
     If IOResult = 0 then with regs do begin
        WriteLn ('Flushing Microsoft SmartDrv.SYS');
        SmartdrvBuf := 2;
        AX := $4403;
        BX := FileRec (FF).Handle;
        CX := 0;
        DS := Seg (SmartDrvBuf);
        DX := Ofs (SmartDrvBuf);
        MSDOS (Regs);
        If ((Flags and FCarry ) <> 0) or (AX=0)
                        Then Status := Status +32 {Error!}
                        Else WriteLn ('Success');
     End;


     WriteLn ('Reseting disks system...');

     {B1.- Disks reset}
     Regs.AX := $0D shl 8;
     MsDos (Regs);

     {B2.- Reset Disk system}
     Regs.AX := 0;
     Regs.DX := 1 shl 7;
     Intr ($13, Regs);
     If (Regs.Flags and FCarry) <>0 Then Status := Status or (1 shl 15) {error!}
                                    Else WriteLn ('Success');

     FlushAllCaches := Status
End;



Procedure  Reboot (warm: boolean);
Type
    TProcedure = Procedure;
    PProcedure = ^TProcedure;
Var
   PLongJmp : PProcedure;
   Regs   : Registers;
Begin
     {1.- Nice reboot: using AT Keyboard}
     If Not Warm Then Begin
        Regs.AX := $1200;
        Intr ($16, Regs);
        If Regs.AX <> $1200 THen {AT MF-II Keyboard present}
           {$ifdef FPK}
           outportb ($64, $FE);
           {$else}
           port [$64] := $FE
           {$endif}
     End;

     {2.- No AT Keyboard present}
     PLongJmp := Ptr ($FFFF,$0000);
     Mem [$0040:$0072] := $1234 * ord(warm);
     {$F+}
     PLongJmp^;
     {$F-}
End;



Procedure ShutDown;
Var
   Regs   : Registers;
Begin
       {RealMode Interface connect}
       With Regs do Begin
          AX := $5301;
          BX := $0000;
       End;
       Intr ($15, Regs);

       {Engage power management}
       With Regs do Begin
          AX := $530F;
          BX := $0001;
          CX := $0001;
       End;
       Intr ($15, Regs);

       {Enable APM for all devices}
       With Regs do Begin
          AX := $5308;
          BX := $0001;
          CX := $0001;
       End;
       Intr ($15, Regs);

       {Force version 1.1 compatibility}
       With Regs do Begin
          AX := $530E;
          BX := $0000;
          CX := $0101;
       End;
       Intr ($15, Regs);

       {1.- First attempt: using APM 1.1 or later}
       {Shutdown all the devices supported by AMP}
       With Regs do Begin
          AX := $5307;
          BX := $0001;
          CX := $0003;
       End;
       Intr ($15, Regs);

       {2.- Second attempt: using APM 1.0}
       {Shutdown only the system BIOS}
       With Regs do Begin
          AX := $5307;
          BX := $0000;
          CX := $0003;
       End;
       Intr ($15, Regs);
End;


Procedure CtrlAltDel;
Var
   Regs   : Registers;
Begin
     Intr ($19, Regs);
End;


Procedure  Suspend;
Var
   Regs: Registers;
Begin
       {RealMode Interface connect}
       With Regs do Begin
          AX := $5301;
          BX := $0000;
       End;
       Intr ($15, Regs);

       {Engage power management}
       With Regs do Begin
          AX := $530F;
          BX := $0001;
          CX := $0001;
       End;
       Intr ($15, Regs);

       {Enable APM for all devices}
       With Regs do Begin
          AX := $5308;
          BX := $0001;
          CX := $0001;
       End;
       Intr ($15, Regs);

       {Force version 1.1 compatibility}
       With Regs do Begin
          AX := $530E;
          BX := $0000;
          CX := $0101;
       End;
       Intr ($15, Regs);

     {Suspend}
     With Regs do
       Begin
          AX := $5307;
          BX := $0001;
          CX := $0002;
       End;
     Intr ($15, Regs)
End;



Function  APMIsInstalled: boolean;
Var
   Regs   : Registers;
Begin
       With Regs do Begin
          AX := $5300;
          BX := $0000;
       End;
       Intr ($15, Regs);

       If (Regs.Flags and FCarry)=0 Then
         Begin
            APMVerMax := lo(Regs.AX);
            APMVerMin := lo(Regs.AL);
            APMIsInstalled := TRUE
         End
       Else
           APMIsInstalled := FALSE
End;



Procedure APMGetVer (var Max,Min: byte);
Begin
     Max := APMVerMax;
     Min := APMVerMin
End;


Function APMLib (funktion: byte): byte;
Var
	a: word;
	max, min: byte;
Begin
	If NOT APMIsInstalled THEN
 	   Begin
	       APMLib := APM_NOT_INSTALLED;
	       exit
	   End;

        APMGetVer ( max, min);

	IF FlushAllCaches>0 THEN BEGIN
		APMLib := APM_ERROR_FLUSHING;
		exit
	End;

	CASE funktion OF
			APM_REBOOT_WARM  : Reboot (TRUE);
			APM_REBOOT_COLD  : Reboot (FALSE);
			APM_CTRLALTDEL   : CtrlAltDel;
			APM_SUSPEND      : BEGIN
						Suspend;
						APMLib := APM_Suspend_Ok;
						exit
					   END;
			APM_SHUTDOWN     : ShutDown;
	END;

	APMLib := APM_NOTHING;
END;




BEGIN
END.

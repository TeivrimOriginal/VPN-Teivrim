; VPN-TEIVRIM Inno Setup installer — One-Click (P0 #3)
; Requires Inno Setup 6.x
#define MyAppName "VPN-TEIVRIM"
#define MyAppVersion "2.4.1"
#define MyAppPublisher "TEIVRIM"
#define MyAppExeName "VPN-TEIVRIM.exe"

[Setup]
AppId={{8E7F3A2B-4C1D-4E9A-B5F2-9A3C7D1E6F08}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName=C:\VPN-TEIVRIM
DefaultGroupName={#MyAppName}
OutputDir=release
OutputBaseFilename=VPN-TEIVRIM-v{#MyAppVersion}-setup
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=commandline
ArchitecturesInstallIn64BitMode=x64
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart"; Description: "Запускать при старте Windows"; GroupDescription: "Автозапуск:"

[Files]
Source: "build\VPN-TEIVRIM.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "vpn-*.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "vpn-*.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: "install.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: "uninstall.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "VPN-TEIVRIM"; ValueData: """{app}\{#MyAppExeName}"""; Flags: uninsdeletevalue; Tasks: autostart

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\*.log"

[Code]
function IsWireGuardInstalled(): Boolean;
begin
  Result := FileExists('C:\Program Files\WireGuard\wg.exe');
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    // Create firewall rules silently
    Exec('netsh', 'advfirewall firewall add rule name="VPN-TEIVRIM-In" dir=in action=allow protocol=udp localport=51820', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    Exec('netsh', 'advfirewall firewall add rule name="VPN-TEIVRIM-Out" dir=out action=allow protocol=udp remoteport=51820', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;

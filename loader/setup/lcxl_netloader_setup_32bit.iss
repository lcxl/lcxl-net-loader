; 脚本用 Inno Setup 脚本向导 生成。
; 查阅文档获取创建 INNO SETUP 脚本文件的详细资料！

#define MyAppName "LCXL Netloader 负载均衡器"
#define MyAppVersion "1.5"
#define MyAppPublisher "LCXBox"
#define MyAppURL "http://www.lcxbox.com/"
#define MyConfigExeName "LCXLNetLoaderService.exe"

[Setup]
; 注意: AppId 的值是唯一识别这个程序的标志。
; 不要在其他程序中使用相同的 AppId 值。
; (在编译器中点击菜单“工具 -> 产生 GUID”可以产生一个新的 GUID)
AppId={{51B1C863-B202-45F1-9468-267D9DD0B489}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
LicenseFile=许可文件.rtf
InfoBeforeFile=LCXL Netloader介绍.rtf
InfoAfterFile=安装后.txt
OutputDir=..\bin
OutputBaseFilename=lcxl_netloader_setup_32bit
SetupIconFile=SetupIcon.ico
Compression=lzma
SolidCompression=yes

[Messages]
BeveledLabel=版权所有 (C) 2013-2014 罗澄曦

[Languages]
Name: "default"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimp.isl"
Name: "chinesetrad"; MessagesFile: "compiler:Languages\ChineseTrad.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "english"; MessagesFile: "compiler:Languages\English.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "greek"; MessagesFile: "compiler:Languages\Greek.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "serbiancyrillic"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "serbianlatin"; MessagesFile: "compiler:Languages\SerbianLatin.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[Files]
Source: "..\bin\Win32\LCXLNetLoaderService.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\Win32\netloader.inf"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\Win32\netloader.sys"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\Win32\netloader_interface.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\Win32\windows标准库\msvcp120.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\Win32\windows标准库\msvcr120.dll"; DestDir: "{app}"; Flags: ignoreversion
; 注意: 不要在任何共享的系统文件使用 "Flags: ignoreversion"

[Run]
Filename: "{app}\{#MyConfigExeName}"; Parameters: "install";StatusMsg: "正在安装驱动和相关服务..."
Filename: "{app}\{#MyConfigExeName}"; Parameters: "startservice";StatusMsg: "正在启动相关服务..."
[UninstallRun]
Filename: "{app}\{#MyConfigExeName}"; Parameters: "uninstall";StatusMsg: "正在卸载驱动和相关服务..."

[Code]
const
  MF_BYPOSITION=$400;
  function DeleteMenu(HMENU: HWND; uPosition: UINT; uFlags: UINT): BOOL; external 'DeleteMenu@user32.dll stdcall';
  function GetSystemMenu(HWND: hWnd; bRevert: BOOL): HWND; external 'GetSystemMenu@user32.dll stdcall';

procedure InitializeWizard();
begin
 //删除构建信息
  DeleteMenu(GetSystemMenu(wizardform.handle,false),8,MF_BYPOSITION);
  DeleteMenu(GetSystemMenu(wizardform.handle,false),7,MF_BYPOSITION);
end;

function UninstallNeedRestart(): Boolean;
begin
  Result := True;
end;
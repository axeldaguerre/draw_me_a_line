@echo off

IF NOT EXIST build mkdir build
pushd build

where /q cl && (
  call cl -arch:AVX2 -Gm- -Zi -W4 -nologo -wd4100 -wd4505 ..\%1 -Fe%~n1_dm.exe /link -INCREMENTAL:NO user32.lib gdi32.lib winmm.lib
  call cl -arch:AVX2 -Gm- -O2 -Zi -W4 -nologo -wd4100 -wd4505 ..\%1 -Fe%~n1_rm.exe -Fm%~n1_rm.map /link -INCREMENTAL:NO user32.lib gdi32.lib winmm.lib
)

@REM where /q clang++ && (
@REM   call clang++ -mavx2 -g -Wall -fuse-ld=lld ..\%1 -o %~n1_dc.exe
@REM   call clang++ -mavx2 -O3 -g -Wall -fuse-ld=lld ..\%1 -o %~n1_rc.exe
@REM )

popd

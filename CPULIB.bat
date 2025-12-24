del cpulib\include\*.h
del cpulib\lib\*.lib
copy vs\cpu-engine\*.h cpulib\include\
del cpulib\include\stdafx.h
copy vs\x64\Debug\cpu-engine.lib cpulib\lib\cpu-engine-debug.lib
copy vs\x64\Release\cpu-engine.lib cpulib\lib\

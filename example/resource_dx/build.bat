rem mmd shader

fxc.exe /T vs_5_0 /E VSMain /Fo mmd.vso mmd.fx
fxc.exe /T ps_5_0 /E PSMain /Fo mmd.pso mmd.fx

python ..\..\tools\bin2h.py mmd.vso
python ..\..\tools\bin2h.py mmd.pso

del mmd.vso
del mmd.pso

rem mmd_edge shader

fxc.exe /T vs_5_0 /E VSMain /Fo mmd_edge.vso mmd_edge.fx
fxc.exe /T ps_5_0 /E PSMain /Fo mmd_edge.pso mmd_edge.fx

python ..\..\tools\bin2h.py mmd_edge.vso
python ..\..\tools\bin2h.py mmd_edge.pso

del mmd_edge.vso
del mmd_edge.pso

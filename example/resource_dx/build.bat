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

rem mmd_ground_shadow shader

fxc.exe /T vs_5_0 /E VSMain /Fo mmd_ground_shadow.vso mmd_ground_shadow.fx
fxc.exe /T ps_5_0 /E PSMain /Fo mmd_ground_shadow.pso mmd_ground_shadow.fx

python ..\..\tools\bin2h.py mmd_ground_shadow.vso
python ..\..\tools\bin2h.py mmd_ground_shadow.pso

del mmd_ground_shadow.vso
del mmd_ground_shadow.pso

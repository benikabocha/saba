fxc.exe /T vs_5_0 /E VSMain /Fo mmd.vso mmd.fx
fxc.exe /T ps_5_0 /E PSMain /Fo mmd.pso mmd.fx

python ..\..\tools\bin2h.py mmd.vso
python ..\..\tools\bin2h.py mmd.pso

del mmd.vso
del mmd.pso

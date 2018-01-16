rem mmd
glslangValidator.exe -V -o mmd.vert.spv mmd.vert
glslangValidator.exe -V -o mmd.frag.spv mmd.frag

python ..\..\tools\bin2h.py mmd.vert.spv
python ..\..\tools\bin2h.py mmd.frag.spv

del mmd.vert.spv
del mmd.frag.spv

rem mmd
glslangValidator.exe -V -o mmd.vert.spv mmd.vert
glslangValidator.exe -V -o mmd.frag.spv mmd.frag

python ..\..\tools\bin2h.py mmd.vert.spv
python ..\..\tools\bin2h.py mmd.frag.spv

del mmd.vert.spv
del mmd.frag.spv

rem mmd_edge
glslangValidator.exe -V -o mmd_edge.vert.spv mmd_edge.vert
glslangValidator.exe -V -o mmd_edge.frag.spv mmd_edge.frag

python ..\..\tools\bin2h.py mmd_edge.vert.spv
python ..\..\tools\bin2h.py mmd_edge.frag.spv

del mmd_edge.vert.spv
del mmd_edge.frag.spv

rem mmd_ground_shadow
glslangValidator.exe -V -o mmd_ground_shadow.vert.spv mmd_ground_shadow.vert
glslangValidator.exe -V -o mmd_ground_shadow.frag.spv mmd_ground_shadow.frag

python ..\..\tools\bin2h.py mmd_ground_shadow.vert.spv
python ..\..\tools\bin2h.py mmd_ground_shadow.frag.spv

del mmd_ground_shadow.vert.spv
del mmd_ground_shadow.frag.spv

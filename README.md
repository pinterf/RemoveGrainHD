# RemoveGrainHD

A collection of  Avisynth denoising filters by Rainer Wittmann

Port of classic RemoveGrainHD 0.5 to Avisynth v2.6 interface (x86/x64)

Note: Previous v0.5 DLL version named RemoveGraindHDS.dll must be deleted to avoid collision

Avisynth wiki: http://avisynth.nl/index.php/RemoveGrainHD

Build instructions
==================
VS2019: 
  use IDE

Windows GCC (mingw installed by msys2):
  from the 'build' folder under project root:

  del ..\CMakeCache.txt
  cmake .. -G "MinGW Makefiles" -DENABLE_INTEL_SIMD:bool=on
  @rem test: cmake .. -G "MinGW Makefiles" -DENABLE_INTEL_SIMD:bool=off
  cmake --build . --config Release  

Linux
  from the 'build' folder under project root:
  ENABLE_INTEL_SIMD is automatically off for non x86 arhitectures
  
* Clone repo and build
    
        git clone https://github.com/pinterf/RemoveGrainHD
        cd RemoveGrainHD
        cmake -B build -S .
        cmake --build build

  Useful hints:        
   build after clean:

        cmake --build build --clean-first

   Force no asm support*
   (just a placeholder; at the moment of v0.7 this plugin has no assembler inside)

        cmake -B build -S . -DENABLE_INTEL_SIMD:bool=off

   delete cmake cache

        rm build/CMakeCache.txt

* Find binaries at
    
        build/RemoveGrainHD/libremovegrainhd.so

* Install binaries

        cd build
        sudo make install
  



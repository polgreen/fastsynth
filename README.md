# fastsynth

## Compilation instructions

git clone git@github.com:polgreen/fastsynth.git

cd fastsynth

git checkout enumerative_program_generator

cd lib/cbmc

git clone https://github.com/diffblue/cbmc.git .

git checkout develop

compile CBMC by following these instructions: http://www.cprover.org/svn/cbmc/trunk/COMPILING

navigate to directory fastsynth/src

make -j8

# fastsynth

## Compilation instructions
Clone the repo
~~~
git clone git@github.com:polgreen/fastsynth.git
cd fastsynth
git checkout enumerative_program_generator
cd lib/cbmc
~~~
clone CBMC into libraries
~~~
git clone https://github.com/diffblue/cbmc.git .
git checkout develop
~~~
compile CBMC by following these instructions: http://www.cprover.org/svn/cbmc/trunk/COMPILING
navigate to directory fastsynth/src
~~~
make -j8
~~~
## Running with neural network
~~~
fastsynth filename.sl --neural-network 
~~~
Neural network interface is currently only set up to work with a beam size of 1.  

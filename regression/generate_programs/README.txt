The program generator only works for 32 bit bitvectors.


The program generator generates programs randomly. The random number generator used is std::uniform_int_distribution.
It is seeded with std::random_device, as in the example here.
http://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution


The commands used are:
--generate-N-programs <N> = generates <N> programs randomly
--number-of-constants <N> = use <N> constants
--program-size <N> = generate programs with <N> instructions
--number-of-params <N> = generate programs that take N arguments
--seed <N> = seed the random number generator with N


fastsynth --generate-N-programs 10 --program-size 3 --number-of-constants 1 --number-of-params 2 --seed 0

This generates 10 programs with 3 instructions, with 2 input arguments, and using 1 constant. The random number generator is seeded with 0.

The program generator will produce duplicates, especially if the number of programs requested is close to the number of possible programs at that length.




#include <iostream>

#include <util/cmdline.h>
#include <util/suffix.h>

#include "c_frontend.h"
#include "program_generator_frontend.h"
#include "smt2_frontend.h"
#include "sygus_frontend.h"

#define FASTSYNTH_OPTIONS                                                      \
  "(max-program-size):"                                                        \
  "(incremental)"                                                              \
  "(simplifying-solver)"                                                       \
  "(fm)"                                                                       \
  "(no-bitwise)"                                                               \
  "(verbosity):"                                                               \
  "(smt)"                                                                      \
  "(literals)"                                                                 \
  "(enable-division)"                                                          \
  "(generate-N-programs):"                                                     \
  "(program-size):"                                                            \
  "(bool-return)"                                                              \
  "(use-mult-and-divide)"                                                      \
  "(enumerative-solver)"                                                       \
  "(number-of-constants):"                                                     \
  "(use-zero-and-one)"                                                         \
  "(seed):"                                                                    \
  "(neural-network)"                                                           \
  "(simple)"                                                                   \
  "(generate-N-programs-old):"                                                 \
  "(number-of-params):"                                                        \
  "(beam-size):"                                                               \
  "(help)"

void help(std::ostream &out)
{
  // clang-format off
  out <<
     "\n"
     "* *                       Fastsynth                          * *\n "
     "* *         CounterExample Guided Inductive Synthesis        * *\n ";
  out  <<
     "* *              Daniel Kroening, Pascal Kesseli             * *\n"
     "* *           Elizabeth Polgreen, Cristina David             * *\n"
     "* *      Oxford University, Computer Science Department      * *\n"
     "* *                  kroening@kroening.com                   * *\n"
     "* *                                                          * *\n"
     "\n"
     "Usage:                       Purpose:\n"
     "\n"
     " fastsynth [-?] [-h] [--help]      show help\n"
     " fastsynth file.c ...              source file names\n"
     " fastsynth file.sl ...             source file names\n"
     "\n"
     "Solver options:\n"
     " --incremental                     use incremental minisat for synthesis\n" // NOLINT(*)
     " --simplifying-solver              use incremental minisat with simplification for synthesis\n" // NOLINT(*)
     " --smt                             use smt solver for synthesis and verification\n" // NOLINT(*)
     " --smt                             use smt solver for synthesis and verification\n" // NOLINT(*)
     " --local-search                    use local-search based verification\n" // NOLINT(*)
     " --fm                              use fourier motzkin based verification\n\n" // NOLINT(*)
     "Solver options: sygus .sl files only\n"
     " --neural-network                  use neural network for synthesis\n" // NOLINT(*)
     " --simple                          use simpler neural network\n" // NOLINT(*)
     " --beam-size                       beam size to use with neural network\n" // NOLINT(*)
     " --enumerative-solver              use enumerative solver for synthesis\n"

     "\n"
     "Instruction set options:\n"
     " --max-program-size N              maximum size of synthesised program\n" // NOLINT(*)
     " --literals                        add literals from spec to instruction set\n" // NOLINT(*)
     " --no-bitwise                      don't include any bit-wise instructions in instruction set\n" // NOLINT(*)
     " --enable-division                 add division to instruction set\n" // NOLINT(*)
     "\n"
     "Enumerative program generator:\n"
     "--generate-N-programs N            generate N random programs\n"
     "--seed N                           seed random program generator with N\n"
     "--program-size N                   size of programs to be generated\n"
     "--number-of-params N               number of arguments accepted by the programs to be generated\n"// NOLINT(*)
     "--number-of-constants N            max number of constants per program\n"// NOLINT(*)
     "--use-zero-and-one                 use zero and one as constants in the programs \n"// NOLINT(*)
     "--bool-return                      generate programs that return a boolean\n"// NOLINT(*)
     "--use-mult-and-divide              generate programs that include multiplication and division \n";// NOLINT(*)
  // clang-format on
}

int main(int argc, const char *argv[])
{
  cmdlinet cmdline;
  if(cmdline.parse(argc, argv, FASTSYNTH_OPTIONS))
  {
    std::cerr << "Usage error\n";
    help(std::cerr);
    return 1;
  }

  if(cmdline.isset("generate-N-programs"))
  {
    assemble_program(cmdline);
    return 0;
  }

  if(cmdline.args.size() != 1)
  {
    std::cerr << "Usage error, file must be given\n";
    help(std::cerr);
    return 1;
  }

  if(cmdline.isset("help") || cmdline.isset("h") || cmdline.isset("?"))
    help(std::cout);

  if(cmdline.isset("generate-N-programs-old"))
  {
    if(has_suffix(cmdline.args.back(), ".sl"))
    {
      std::cout << "Generating random programs \n";
      generate_programs(
        cmdline, std::stol(cmdline.get_value("generate-N-programs-old")));
      return 0;
    }
    else
      std::cerr << "Error: generate programs must be given .sl file\n";
    return 1;
  }

  try
  {
    if(has_suffix(cmdline.args.back(), ".sl"))
      return sygus_frontend(cmdline);
    else if(has_suffix(cmdline.args.back(), ".smt2"))
      return smt2_frontend(cmdline);
    else if(has_suffix(cmdline.args.back(),".c"))
      return c_frontend(cmdline);
    else
      std::cerr<< "Error: unrecognised input file, input files should be either .sl, .smt or .c files\n";
  }
  catch(const char *s)
  {
    std::cerr << "Error: " << s << '\n';
  }
  catch(const std::string &s)
  {
    std::cerr << "Error: " << s << '\n';
  }
}

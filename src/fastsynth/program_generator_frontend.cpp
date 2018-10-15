/*
 * program_generator.cpp
 *
 *  Created on: 25 Apr 2018
 *      Author: elipol
 */

#include "program_generator_frontend.h"

#include <util/config.h>
#include <util/cout_message.h>
#include <util/namespace.h>
#include <util/symbol_table.h>

#include <ansi-c/ansi_c_language.h>
#include <langapi/language_util.h>
#include <langapi/mode.h>

#include "enumerative_learn.h"
#include "program_generator.h"
#include "sygus_parser.h"
#include "synth_encoding.h"

#include <fstream>
#include <iostream>
#include <random>

/// add default literals to the synth encoding
/// This is used to limit the number of constants allowed in the
/// randomly generated programs
/// \param synth_encoding
/// \param n number of constants
void add_literals(synth_encodingt &synth_encoding, std::size_t n)
{
  synth_encoding.literals.clear();
  for(std::size_t i = 0; i < n; i++)
  {
    irep_idt ID = "constant_value" + std::to_string(i);
    symbol_exprt expr(ID, unsignedbv_typet(32));
    synth_encoding.literals.insert(expr);
  }
}

int generate_programs(const cmdlinet &cmdline, std::size_t number_of_programs)
{
  PRECONDITION(cmdline.args.size() == 1);

  register_language(new_ansi_c_language);
  config.ansi_c.set_32();

  console_message_handlert message_handler;
  messaget message(message_handler);

  // this is our default verbosity
  unsigned int v = messaget::M_STATISTICS;

  if(cmdline.isset("verbosity"))
  {
    v = std::stol(cmdline.get_value("verbosity"));
    if(v > 10)
      v = 10;
  }

  message_handler.set_verbosity(v);

  std::ifstream in(cmdline.args.front());

  if(!in)
  {
    message.error() << "Failed to open input file" << messaget::eom;
    return 10;
  }

  sygus_parsert parser(in);
  parser.set_message_handler(message_handler);

  parser.parse();

  if(!parser)
    return 20;

  problemt problem;
  problem.constraints = parser.constraints;

  for(const auto &v : parser.variable_map)
    problem.free_variables.insert(symbol_exprt(v.first, v.second));

  for(const auto &v : parser.full_let_variable_map)
    problem.free_variables.insert(symbol_exprt(v.first, v.second));

  for(auto &c : problem.constraints)
    parser.expand_function_applications(c);

  symbol_tablet symbol_table;
  namespacet ns(symbol_table);
  int program_size = 5;
  int number_of_constants = 0;

  if(cmdline.isset("number-of-constants"))
  {
    number_of_constants = std::stol(cmdline.get_value("number-of-constants"));
  }
  else
  {
    message.warning() << " number of constants to use not specified "
                      << "using 0 constants" << messaget::eom;
  }

  if(cmdline.isset("program-size"))
  {
    program_size = std::stol(cmdline.get_value("program-size"));
    if(program_size == 0)
    {
      message.warning() << "program size of zero is impossible\n";
      return 1;
    }
    if(program_size == 2)
    {
      message.warning()
        << "program size of 2 has no programs, Using program size 3 \n";
    }
  }
  else
    message.warning() << "no program size given, "
                      << "generating programs for default size 5"
                      << messaget::eom;

  synth_encodingt synth_encoding;
  synth_encoding.program_size = program_size;
  synth_encoding.enable_bitwise = !cmdline.isset("no-bitwise");
  add_literals(synth_encoding, number_of_constants);

  enumerative_program_generatort program_generator(ns, synth_encoding, problem);

  message.status() << "Number of possible programs "
                   << program_generator.solver.number_of_options
                   << messaget::eom;

  if(number_of_programs >= program_generator.solver.number_of_options)
  {
    number_of_programs = program_generator.solver.number_of_options;
    message.warning() << "You have asked for more programs than is possible"
                      << " will return " << number_of_programs << "\n";
  }

  int seed;
  if(cmdline.isset("seed"))
    seed = std::stol(cmdline.get_value("seed"));
  else
  {
    message.warning() << "No seed given, seeding mersenne twister "
                      << "with std::random_device. Not guaranteed to produce "
                      << "random numbers on all systems\n";
    std::random_device rd;
    seed = rd();
  }

  std::mt19937 gen(seed);

  std::set<std::vector<std::size_t>> programs_generated;
  for(std::size_t i = 0; i < number_of_programs; i++)
  {
    program_generator.assignment_indices.clear();
    bool got_new_assignment = false;
    std::size_t max_iterations = 0;
    while(!got_new_assignment)
    {
      for(const auto &v : program_generator.solver.selector_variables)
      {
        std::uniform_int_distribution<> dis(0, v.size() - 1);
        program_generator.assignment_indices.push_back(dis(gen));
      }
      if(programs_generated.count(program_generator.assignment_indices))
      {
        if(max_iterations > 1000)
        {
          message.error() << "reached 1000 iterations without "
                          << "finding a new assignment" << messaget::eom;
          throw 0;
        }
        program_generator.assignment_indices.clear();
        max_iterations++;
      }
      else
      {
        programs_generated.insert(program_generator.assignment_indices);
        got_new_assignment = true;
      }
    }
    program_generator.output_program(message.status());
    message.status() << messaget::eom;
  }
  return 0;
}

void assemble_program(const cmdlinet &cmdline)
{
  register_language(new_ansi_c_language);
  config.ansi_c.set_32();

  console_message_handlert message_handler;
  messaget message(message_handler);

  // this is our default verbosity
  unsigned int v = messaget::M_STATISTICS;

  if(cmdline.isset("verbosity"))
  {
    v = std::stol(cmdline.get_value("verbosity"));
    if(v > 10)
      v = 10;
  }

  int seed;
  if(cmdline.isset("seed"))
    seed = std::stol(cmdline.get_value("seed"));
  else
  {
    message.warning() << "No seed given, seeding mersenne twister "
                      << "with std::random_device. Not guaranteed to produce "
                      << "random numbers on all systems\n";
    std::random_device rd;
    seed = rd();
  }

  std::size_t program_size = 5;
  std::size_t number_of_constants = 0;
  std::size_t number_of_params = 2;
  std::size_t number_of_programs =
    std::stol(cmdline.get_value("generate-N-programs"));

  if(cmdline.isset("number-of-constants"))
  {
    number_of_constants = std::stoul(cmdline.get_value("number-of-constants"));
  }
  else
  {
    message.warning() << "Number of constants to use not specified; "
                      << "using 0 constants" << messaget::eom;
  }

  if(cmdline.isset("program-size"))
  {
    program_size = std::stoul(cmdline.get_value("program-size"));
    if(program_size == 0)
    {
      message.warning() << "program size of zero is impossible"
                        << messaget::eom;
      return;
    }
    if(program_size == 2)
    {
      message.warning()
        << "program size of 2 has no programs, Using program size 3"
        << messaget::eom;
    }
  }
  else
    message.warning() << "No program size given; "
                      << "generating programs for default size 5"
                      << messaget::eom;

  if(cmdline.isset("number-of-params"))
    number_of_params = std::stoul(cmdline.get_value("number-of-params"));
  else
    message.warning() << "Number of program arguments not given; "
                      << "generating programs with 2 arguments"
                      << messaget::eom;

  program_generatort generator(
    seed,
    program_size,
    number_of_constants,
    number_of_params,
    cmdline.isset("use-mult-and-divide"),
    cmdline.isset("use-zero-and-one"));

  generator.bool_return = cmdline.isset("bool-return");
  generator.assemble_programs(number_of_programs);
}

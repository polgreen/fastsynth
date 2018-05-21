/*
 * neural_learn.h
 *
 *  Created on: 13 May 2018
 *      Author: elipol
 */

#ifndef CPROVER_FASTSYNTH_NEURAL_LEARN_H
#define CPROVER_FASTSYNTH_NEURAL_LEARN_H

#include <fastsynth/solver_learn.h>
#include <util/message.h>
#include <solvers/flattening/bv_pointers.h>

#include "output_generator_encoding.h"

///  interface to neural network implemeted in python
class neural_learnt : public solver_learn_baset
{
public:
  /// Creates an interface to the neural network learner.
  /// For every counterexample input, uses a SAT solver to get
  // a corresponding output. Constructs a command of input/output examples
  /// to send to neural network and parses the output
  /// currently hard-coded to use beam size of 1
  /// \param msg \see msg solver_learnt::msg
  /// \param ns \see ns solver_learnt::ns
  /// \param problem \see solver_learnt::problem
  neural_learnt(
    const namespacet &_ns,
    const problemt &_problem,
    message_handlert &_mh);

protected:
  // solver_instance used to generate output examples
  bv_pointerst output_generator;

  std::vector<counterexamplet> counterexamples;
  solutiont last_solution;

  // command to send to neural network, stored in 3 parts
  std::string command;
  std::vector<std::string> input_examples;
  std::string output_examples;

  message_handlert &message_handler;
  std::string tmp_results_filename;

  /// beam size parameter for neural network; corresponds to the number of
  /// programs returned by the neural network
  std::size_t beam_size;

  virtual decision_proceduret::resultt operator()();

  /// returns a dummy program for use when we don't have enough counterexamples
  /// to make it worth firing up the neural network.
  /// The program returned returns the dummy_program_return_constant
  /// \return solutiont dummy program
  solutiont dummy_program();
  int dummy_program_return_constant;

  /// reads the result from the neural network, writes the solution to
  /// last_solution, and returns SAT if a solution is correctly read in.
  /// and ERROR if the parser fails. NOTE: "SAT" does not mean the solution
  /// is correct, just that it has been successfully read
  /// \param in input stream
  // \return \see decision_proceduret::resultt
  decision_proceduret::resultt read_result(std::istream &in);
  void init();

  /// Synthesis learn constraint generator.
  output_generator_encodingt encoding;

  /// Provides the last solution found.
  /// \return \see verify_encodingt::expressions
  virtual solutiont get_solution() const;

  /// adds a counterexample input to the list of counterexamples
  /// and gets the corresponding output. Adds both the input and output
  /// to the input_example and output_example command strings
  /// \param counterexample new counterexample
  virtual void add_ce(const counterexamplet &);

  /// normalise the value of an expr to a double between -1 and 1
  /// and return a string to append to the commands
  /// \param N expr value to be normalised
  /// \return string with value between -1 and 1
  std::string normalise(const exprt &expr);

  /// Does nothing. Program size is not currently used by the neural network
  virtual void set_program_size(size_t program_size)
  {
    // do nothing
  };

  /// Sets up the solver used to generate the output examples for each
  /// input counterexample
  /// \param problem synthesis problem
  void construct_output_generator(
      const problemt &problem);
};

#endif /* CPROVER_FASTSYNTH_NEURAL_LEARN_H */

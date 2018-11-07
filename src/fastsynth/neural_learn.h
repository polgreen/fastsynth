/*
 * neural_learn.h
 *
 *  Created on: 13 May 2018
 *      Author: elipol
 */

#ifndef CPROVER_FASTSYNTH_NEURAL_LEARN_H
#define CPROVER_FASTSYNTH_NEURAL_LEARN_H

#include <memory>

#include "verify.h"
#include <fastsynth/solver_learn.h>
#include <util/message.h>
#include <solvers/flattening/bv_pointers.h>
#include <queue>

#include "output_generator_encoding.h"

///  interface to neural network implemeted in python
class neural_learnt : public solver_learn_baset
{
public:
  /// Creates an interface to the neural network learner.
  /// For every counterexample input, uses a SAT solver to get
  /// a corresponding output. Constructs a command of input/output examples
  /// to send to neural network and parses the output
  /// currently hard-coded to use beam size of 1
  /// \param msg \see msg solver_learnt::msg
  /// \param ns \see ns solver_learnt::ns
  /// \param problem \see solver_learnt::problem
  neural_learnt(
    const namespacet &_ns,
    const problemt &_problem,
    message_handlert &_mh,
    std::size_t &_beam_size,
    bool use_simple_network);

protected:
  /// Solver instance.
  std::unique_ptr<propt> generator_satcheck;

  /// Solver used to generate output examples.
  std::unique_ptr<class bv_pointerst> output_generator;
  /// Solver used to pre-verify batch
  std::unique_ptr<class bv_pointerst> pre_verifier;
  std::queue<solutiont> stock_solutions;

  std::vector<counterexamplet> counterexamples;
  solutiont last_solution;

  bool simple_network;

  // command to send to neural network, stored in 3 parts
  std::string command;
  std::vector<std::vector<std::string>> input_examples;
  std::vector<std::string> output_examples;
  // use the pre verifier to select the best program from a batch to feed to the verifier
  bool pre_verify_batch;
  // max number of i.o to use
  std::size_t max_number_io;
  int seed; // seed for random counterexample generator

  std::string tmp_results_filename;

  /// beam size parameter for neural network; corresponds to the number of
  /// programs returned by the neural network
  std::size_t beam_size;

  virtual decision_proceduret::resultt operator()();
  decision_proceduret::resultt operator()(verifyt &verifier);

  /// returns a dummy program for use when we don't have enough counterexamples
  /// to make it worth firing up the neural network.
  /// The program returned returns the dummy_program_return_constant. Typically
  /// only used once, in order to generate the first counterexample. We then use
  /// add_random_ces to generate more input/output pairs based on that counterexample.
  /// \return solutiont dummy program
  solutiont dummy_program();
  int dummy_program_return_constant;

  /// generates random input/output examples for use when we
  /// don't have enough counterexamples to make it worth firing up the neural
  /// network. Requires a counterexample as input to base the random
  /// input/output examples on (we take the structure of the counterexample
  /// and replace the values with randomly generated values
  /// \param cex counterexample to base input/output pairs on
  /// \param n number of input/output pairs to add
  void add_random_ces(const counterexamplet &cex, std::size_t n);

  /// reads the result from the neural network, writes the solution to
  /// last_solution, and returns SAT if a solution is correctly read in.
  /// and ERROR if the parser fails. NOTE: "SAT" does not mean the solution
  /// is correct, just that it has been successfully read
  /// \param in input stream
  /// \param verifier to verify batch result
  // \return \see decision_proceduret::resultt
  decision_proceduret::resultt read_result(std::istream &in, verifyt &verifier);
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

  void add_ce(const counterexamplet & cex, bool add_random_cex);

  /// if the expression is an integer, normalises the value of the
  /// integer to a double between -1 and 1
  /// and return a string to append to the commands. Else
  /// returns the result of from_expr(ns,"",expr).
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
  void construct_output_generator();

  /// reset the solver used to generate the output examples for each
  /// input counterexample
  void reset_output_generator();
};

#endif /* CPROVER_FASTSYNTH_NEURAL_LEARN_H */

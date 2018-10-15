/*
 * program_generator.h
 *
 *  Created on: 4 Jun 2018
 *      Author: elipol
 */

#ifndef SRC_FASTSYNTH_PROGRAM_GENERATOR_H_
#define SRC_FASTSYNTH_PROGRAM_GENERATOR_H_


#include <util/std_types.h>
#include <util/config.h>
#include <util/bv_arithmetic.h>
#include <random>

class program_generatort
{
public:
  explicit program_generatort(
      int seed,
      std::size_t _program_size,
      std::size_t _num_constants,
      std::size_t _num_params):
      bool_return(false),
      gen(seed),
      program_size(_program_size),
      num_constants(_num_constants),
      num_params(_num_params),
      max_number_of_ifs(5u),
      max_number_of_shifts(5u),
      number_of_ifs(0u),
      number_of_shifts(0u)
  {
    initialise_operations();
  }

  void assemble_programs(std::size_t n);
  bool bool_return;
protected:

  struct operationt
  {
    std::string name;
  };

  struct instructiont
  {
    std::string string;
    std::size_t length=1;
    bool contains_if=false;
  };

  enum class operator_typet { BITVEC_RETURN, BOOL_RETURN };

  std::mt19937 gen;
  std::size_t program_size;
  std::size_t num_constants;
  std::size_t num_params;
  std::size_t max_number_of_ifs;
  std::size_t max_number_of_shifts;
  std::size_t number_of_ifs;
  std::size_t number_of_shifts;
  std::vector<operationt>bitvec_return_operations;
  std::vector<operationt>bool_return_operations;
  std::vector<instructiont> bitvec_operands;
  std::vector<instructiont> prev_programs;

  operationt not_op;

  std::vector<instructiont> program;

  instructiont get_random_instruction(
      operator_typet type, bool not_if, bool not_shift);
  void initialise_operations();

  instructiont get_binary_bool_operands(operationt &op);
  instructiont get_ite_operands(operationt &op);
  instructiont get_binary_bitvec_operands(operationt &op);
  bool discard_operands(instructiont &operand1, std::size_t &op1_idx,
      instructiont &operand2, std::size_t &op2_idx, operationt &operation);
  void reset();

};



#endif /* SRC_FASTSYNTH_PROGRAM_GENERATOR_H_ */

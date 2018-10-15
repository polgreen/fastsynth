/*
 * program_generator2.cpp
 *
 *  Created on: 31 May 2018
 *      Author: elipol
 */

#define LOOPLIMIT 10000

#include <util/std_types.h>
#include <util/config.h>
#include <util/bv_arithmetic.h>
#include "program_generator.h"


#include <iostream>
#include <random>

bool program_generatort::discard_operands(
    instructiont &operand1, std::size_t &op1_idx,
    instructiont &operand2, std::size_t &op2_idx,
    operationt &op)
{
  // we don't allow ifs inside any operator except if
  if(op.name!="ite" &&
      (operand1.contains_if || operand2.contains_if))
    return true;

  // don't use duplicate operands unless with bvadd
  if(op.name != "bvadd")
  {
    if(op1_idx==op2_idx)
      return true;
  }
  if(op.name == "bvadd")
    if(op1_idx>op2_idx)
      return true;

  // the following operands are commutative so we only need to
  // try one order
  // bvult and bvule are not commutative, but can be combined with not
  if(op.name=="bvadd" || op.name=="bvor" || op.name=="bvxor" ||
      op.name=="bvand" || op.name=="="|| op.name=="bvult" || op.name=="bvule")
    if(op1_idx >= op2_idx)
      return true;

  // only restrictions on ite is that the return values
  // for each case are not the same and
  // only appear in one order. We can skip the rest.
  if(op.name=="ite")
    return false;

  // don't do operations on 2 constants, rarely worth it
  if(op1_idx < num_constants && op2_idx < num_constants)
    return true;

  // We only shift by a constant or a parameter
  if(op.name == "bvshl" || op.name == "bvlshr")
  {
    // we don't shift a constant
    if(op1_idx<num_constants)
      return true;

    if(op2_idx >= num_constants+num_params)
    {
      // replace op2
      std::uniform_int_distribution<unsigned int>param_or_const(
          0, num_params + num_constants -1);

      std::size_t index=op1_idx;
      std::size_t loop_iter=0;
      while(index == op1_idx)
      {
        loop_iter++;
        if(loop_iter > LOOPLIMIT)
          return true;

        index = param_or_const(gen);
        if(index != op1_idx)
        {
          if(index < num_params)
            operand2.string = " |synth::parameter" +
                    std::to_string(index) + "|";
          else
          {
            PRECONDITION(index - num_params >= 0 &&
                index - num_params < num_constants);
            operand2.string = " |constant_value"
                + std::to_string(index - num_params) + "|";
          }
          operand2.length = 1;
          operand2.contains_if = false;
        }
      }
    }
  }

  // we don't allow if statements inside the operands
  // of anything except and if statement
  if(operand1.contains_if || operand2.contains_if)
    return true;

  // we don't allow subtraction of a parameter from a statement that contains
  // the same parameter, unless that statement is a nonlinear expression
  if(op.name == "bvsub")
  {
    for(std::size_t i = 0; i < num_params; i++)
    {
      if( (operand1.string.find("|synth::parameter" + std::to_string(i)))
          != std::string::npos
          && (operand1.string.find("bvshl") == std::string::npos
              && operand1.string.find("bvlshr") == std::string::npos
              && operand1.string.find("bvand") == std::string::npos
              && operand1.string.find("bvxor") == std::string::npos
              && operand1.string.find("bvor") == std::string::npos))
      {
        if(operand2.string.find("|synth::parameter" + std::to_string(i))
            != std::string::npos
            && (operand2.string.find("bvshl") == std::string::npos
                && operand2.string.find("bvlshr") == std::string::npos
                && operand2.string.find("bvand") == std::string::npos
                && operand2.string.find("bvxor") == std::string::npos
                && operand2.string.find("bvor") == std::string::npos))
        {
          return true;
        }
      }
    }


    for(std::size_t i = 0; i < num_constants; i++)
    {
      if( (operand1.string.find("|constant_value" + std::to_string(i)))
          != std::string::npos
          && (operand1.string.find("bvshl") == std::string::npos
              || operand1.string.find("bvlshr") == std::string::npos
              || operand1.string.find("bvand") == std::string::npos
              || operand1.string.find("bvxor") == std::string::npos
              || operand1.string.find("bvor") == std::string::npos))
      {
        if(operand2.string.find("|constant_value" + std::to_string(i))
            != std::string::npos
            && (operand2.string.find("bvshl") == std::string::npos
                || operand2.string.find("bvlshr") == std::string::npos
                || operand2.string.find("bvand") == std::string::npos
                || operand2.string.find("bvxor") == std::string::npos
                || operand2.string.find("bvor") == std::string::npos))
          return true;
      }
    }
  }

  // don't and a variable with itself (unless it has been shifted or subtracted or added)
  // don't xor a variable with itself (unless it has been shifted or subtracted or added)
  // don't or a variable with itself (unless it has been shifted or subtracted or added)

  if(op.name == "bvand" || op.name=="bvxor" || op.name == "bvor")
  {
    for(std::size_t i = 0; i < num_params; i++)
    {
      if(operand1.string.find("|synth::parameter" + std::to_string(i))
          != std::string::npos
          && (operand1.string.find("bvshl") == std::string::npos
          && operand1.string.find("bvsub") == std::string::npos
          && operand1.string.find("bvadd") == std::string::npos
              && operand1.string.find("bvlshr") == std::string::npos))
      {
        if(operand2.string.find("|synth::parameter" + std::to_string(i))
            != std::string::npos
            && (operand1.string.find("bvshl") == std::string::npos
            && operand1.string.find("bvsub") == std::string::npos
            && operand1.string.find("bvadd") == std::string::npos
                && operand1.string.find("bvlshr") == std::string::npos))
          return true;
      }
    }

    for(std::size_t i = 0; i < num_constants; i++)
    {
      if(operand1.string.find("|constant_value" + std::to_string(i))
          != std::string::npos
          && (operand1.string.find("bvshl") == std::string::npos
          && operand1.string.find("bvsub") == std::string::npos
          && operand1.string.find("bvadd") == std::string::npos
              && operand1.string.find("bvlshr") == std::string::npos))
      {
        if(operand2.string.find("|constant_value" + std::to_string(i))
            != std::string::npos
            && (operand1.string.find("bvshl") == std::string::npos
            && operand1.string.find("bvsub") == std::string::npos
            && operand1.string.find("bvadd") == std::string::npos
                && operand1.string.find("bvlshr") == std::string::npos))
          return true;
      }
    }
  }
return false;
}

void program_generatort::initialise_operations()
{
  static const std::vector<std::string> bit_vecs=
      { "bvadd", "bvsub", "bvshl", "bvand", "bvor", "bvxor", "bvlshr" };

    static const std::vector<std::string> bool_returns=
      {"bvule", "bvult", "=" };

    for(const auto &o : bit_vecs)
      {
        operationt op;
        op.name=o;
        bitvec_return_operations.push_back(op);
      }

      for(const auto &o : bool_returns)
      {
        operationt op;
        op.name=o;
        bool_return_operations.push_back(op);
      }

  operationt ite_op;
  ite_op.name = "ite";
  bitvec_return_operations.push_back(ite_op);



  for(std::size_t i=0; i<num_constants; i++)
  {
    instructiont ins;
    ins.string="|constant_value"+std::to_string(i)+"|";
    ins.contains_if=false;
    bitvec_operands.push_back(ins);
  }

  for(std::size_t i=0; i<num_params; i++)
  {
    instructiont ins;
    ins.string="|synth::parameter"+std::to_string(i)+"|";
    ins.contains_if=false;
    bitvec_operands.push_back(ins);
  }
}

program_generatort::instructiont program_generatort::get_ite_operands(
    program_generatort::operationt &op)
{
  PRECONDITION(op.name=="ite");

  std::uniform_int_distribution<unsigned int> bitvec_operand_dis(0,
            bitvec_operands.size() - 1);


  instructiont res;
  res.contains_if=true;
  res.string = " (ite ";
  res.length=1;

  // get first operand
  instructiont condition = get_random_instruction(
      program_generatort::operator_typet::BOOL_RETURN, true, false);
  res.string+= condition.string;

  res.string+=" ";
  res.length+=condition.length;

  // first case return
  instructiont tmp=bitvec_operands[bitvec_operand_dis(gen)];
  res.string+=tmp.string;
  res.length+=tmp.length;
  res.string+=" ";

  // second case return;
  bool got_operand=false;
  instructiont tmp2;
  while(!got_operand)
  {
    tmp2=bitvec_operands[bitvec_operand_dis(gen)];
    if(tmp2.string!=tmp.string)
      got_operand=true;
  }
  res.string+=tmp2.string;
  res.contains_if= true;
  res.length+=tmp2.length;
  res.string+=" )";

  return res;
}



program_generatort::instructiont program_generatort::get_binary_bitvec_operands(
    program_generatort::operationt &op)
{
  instructiont res;
  res.contains_if=false;

  res.string = " ("+op.name + " ";

  std::uniform_int_distribution<unsigned int> bitvec_operand_dis(0,
                bitvec_operands.size() - 1);

  bool found_operands=false;
  std::size_t operand_1_index;
  std::size_t operand_2_index;
  instructiont op1;
  instructiont op2;
  std::size_t loop_iter=0;

  // this loop runs until it finds an acceptable pair of operands, or hits
  // the loop limit
  while(!found_operands)
  {
    loop_iter++;
    if(loop_iter > LOOPLIMIT)
      throw std::domain_error("Unable to find operands within the loop limit");

    operand_1_index=bitvec_operand_dis(gen);
    op1 = bitvec_operands[operand_1_index];

    operand_2_index=bitvec_operand_dis(gen);
     op2 = bitvec_operands[operand_2_index];

    found_operands=!discard_operands(
        op1, operand_1_index, op2, operand_2_index, op);
  }

  res.string+=op1.string + " " + op2.string + ")";
  res.length+=op1.length+op2.length;
  return res;
}

program_generatort::instructiont program_generatort::get_binary_bool_operands(
    program_generatort::operationt &op)
{
  instructiont res;
  res.length=1;
  res.contains_if=false;

  std::uniform_int_distribution<unsigned int> bitvec_operand_dis(0,
              bitvec_operands.size() - 1);

  res.string = " ("+op.name + " ";

  bool found_operands = false;
  std::size_t operand_1_index;
  std::size_t operand_2_index;
  instructiont op1;
  instructiont op2;
  std::size_t loop_iter = 0;

  // this loop runs until it finds an acceptable pair of operands, or hits
  // the loop limit
  while(!found_operands)
  {
    loop_iter++;
    if(loop_iter > LOOPLIMIT)
      throw std::domain_error("Unable to find operands within the loop limit");

    operand_1_index = bitvec_operand_dis(gen);
    op1 = bitvec_operands[operand_1_index];

    operand_2_index = bitvec_operand_dis(gen);
    op2 = bitvec_operands[operand_2_index];

    found_operands = !discard_operands(op1, operand_1_index, op2,
        operand_2_index, op);
  }

  res.string += op1.string + " " + op2.string + ")";
  res.length+=op1.length+op2.length;

  std::uniform_int_distribution<> negate_dis(0, 1);

  if(negate_dis(gen) == 1)
  {
    res.string = " ( not " + res.string + ")";
    res.length++;
  }

  return res;
}



program_generatort::instructiont program_generatort::get_random_instruction(
    program_generatort::operator_typet type, bool not_if, bool not_shift)
{
  operationt op;

  if(bitvec_operands.size()<2)
    not_if=true;

  if(type==program_generatort::operator_typet::BITVEC_RETURN)
  {
    std::uniform_int_distribution<unsigned int> bitvec_operators_dis(0,
          bitvec_return_operations.size() - 1);

    op=bitvec_return_operations[bitvec_operators_dis(gen)];
    // discard if
    std::size_t loop_iter=0;
    while((not_if && op.name=="ite") ||
          (not_shift && op.name=="bvlshr") ||
          (not_shift && op.name=="bvshl"))
    {
      loop_iter++;
      if(loop_iter > LOOPLIMIT)
      {
        std::cerr << "UNABLE TO FIND OPERATOR IN "<<LOOPLIMIT<< "iterations \n";
        throw std::domain_error(
            "Unable to find next operator within the loop limit");
      }
        op=bitvec_return_operations[bitvec_operators_dis(gen)];
    }

    instructiont result;
    try
    {
      if(op.name == "ite")
      {
        number_of_ifs++;
        return get_ite_operands(op);
      }
      else
      {
        if(op.name == "bvlshr" || op.name == "bvshl")
          number_of_shifts++;
        return get_binary_bitvec_operands(op);
      }
    } catch(...)
    {
      // bail out, go back to assemble programs loop and try again
      return bitvec_operands.back();
    }
  }
  else
  {
    std::uniform_int_distribution<unsigned int> bool_operators_dis(0,
          bool_return_operations.size() - 1);

    try
    {
    op=bool_return_operations[bool_operators_dis(gen)];
      return get_binary_bool_operands(op);
    }
    catch(...)
    {
      // bail out, go back to assemble programs loop and try again
      return bitvec_operands.back();
    }
  }


}

void program_generatort::assemble_programs(std::size_t number_of_programs)
{
  for(std::size_t p = 0; p < number_of_programs; p++)
  {
    reset();
    std::size_t instruction_length=0u;
    std::cout << "<program.0.>";

    if(program_size == 1)
    {
      std::uniform_int_distribution<unsigned int> param_or_const(0,
          num_params + num_constants - 1);

      instructiont ins = bitvec_operands[param_or_const(gen)];
      program.push_back(ins);
    }
    else
    {
      while(instruction_length < program_size - 1)
      {
        instructiont ins;
        if(bitvec_operands.size() == 1)
        {
          // only possible instruction is bvadd operands.
          ins.contains_if = false;
          ins.length = 3;
          ins.string = "(bvadd " + bitvec_operands[0].string + " "
              + bitvec_operands[0].string + ")";
        }
        else
        {
          ins = get_random_instruction(operator_typet::BITVEC_RETURN,
              (number_of_ifs >= max_number_of_ifs),
              number_of_shifts >= max_number_of_shifts);
        }

        // discard instructions that we have already generated
        for(const auto &i : program)
        {
          if(ins.string == i.string)
          {
            continue;
          }
        }

        program.push_back(ins);
        bitvec_operands.push_back(ins);
        instruction_length = ins.length;
      }
      if(bool_return)
      {
      // add boolean instruction to end
        instructiont final_ins = get_random_instruction(
           operator_typet::BOOL_RETURN, true, true);
        program.push_back(final_ins);
      }

      for(const auto &prog : prev_programs)
      {
        if(program.back().string == prog.string) // discard
          p--;
        else
          prev_programs.push_back(program.back());
      }
    }

    std::cout<<program.back().string<<"</program.0.>"<<std::endl;
  }
}

void program_generatort::reset()
{
  bitvec_return_operations.clear();
  bool_return_operations.clear();
  bitvec_operands.clear();
  program.clear();
  number_of_ifs=0;
  number_of_shifts=0;
  initialise_operations();
}

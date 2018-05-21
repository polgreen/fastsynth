#ifndef CPROVER_FASTSYNTH_ENUMERATIVE_LEARN_H
#define CPROVER_FASTSYNTH_ENUMERATIVE_LEARN_H


#include <util/namespace.h>

#include <fastsynth/solver_learn.h>
#include <fastsynth/synth_encoding.h>

#include <fastsynth/enumerative_assignment_generator.h>

/// Generates a constraint using synth_encodingt and solves it by
/// enumerating through possible programs using
/// enumerative_program_generator
class enumerative_learnt : public solver_learn_baset
{
protected:
  std::size_t program_size;
  std::vector<counterexamplet> counterexamples;
  solutiont last_solution;
  std::size_t program_index;


  bool verify_solution();

public:
  /// Creates a enumerative learner.
  /// \param msg \see msg solver_learnt::msg
  /// \param ns \see ns solver_learnt::ns
  /// \param problem \see solver_learnt::problem
  enumerative_learnt(
    const namespacet &,
    const problemt &,
    message_handlert &);

  /// \see learnt::set_program_size(size_t)
  void set_program_size(size_t program_size) override;

  void output_program(const solutiont &solution, std::ostream &out) const;

  /// \see learnt::operator()()
  decision_proceduret::resultt operator()() override;

  /// \see learnt::get_expressions()
  solutiont get_solution() const override;

  /// \see learnt::add(const verify_encodingt::counterexamplet &counterexample)
  void add_ce(const counterexamplet &) override;

};

/// generates programs using enumerative assignments
/// see program_generator_frontend.cpp
class enumerative_program_generatort
{
public:
  enumerative_program_generatort(
      const namespacet &_ns,
      synth_encodingt &_synth_encoding,
      problemt & _problem):
        ns(_ns),
    solver(_ns, _synth_encoding),
    synth_encoding(_synth_encoding)
    {
     set_up(_problem);
    }

  void set_up(problemt &problem);
  namespacet ns;
  enumerative_assignment_generatort solver;
  std::vector<std::size_t> assignment_indices;
  synth_encodingt synth_encoding;
  solutiont get_nth_program(const std::size_t &n);
  void output_program(std::ostream &out, const std::size_t &n);
  void output_program(std::ostream &out);
};


#endif /* CPROVER_FASTSYNTH_ENUMERATIVE_LEARN_H */

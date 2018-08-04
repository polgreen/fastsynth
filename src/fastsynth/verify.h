#ifndef CPROVER_FASTSYNTH_VERIFY_H_
#define CPROVER_FASTSYNTH_VERIFY_H_

#include "cegis_types.h"
#include "verify_encoding.h"

class decision_proceduret;

/// verify a candidate solution
class verifyt : public messaget
{
public:
  verifyt(
    const namespacet &_ns,
    const problemt &_problem,
    message_handlert &_message_handler):
    messaget(_message_handler),
    use_smt(false),
    ns(_ns), problem(_problem)
  {
  }

  /// Check a new candidate.
  /// \return \see decision_proceduret::resultt
  virtual decision_proceduret::resultt operator()(solutiont &);

  /// Check a new candidate is valid for a subset of counterexamples
  /// \param solution solution to check
  /// \param counterexamples vector of counterexamples to check
  /// the solution is valid for
  /// \return the number of satisfied counterexamples
  std::size_t operator()(
      solutiont &solution, std::vector<counterexamplet> &counterexamples);

  /// Check a new candidate and produce a counterexample that
  /// is definitely different to the previous counterexamples
  /// \param solution solution to check
  /// \param counterexamples vector of counterexamples received previously
  /// \param force_new_cex if set to true, forces verifier to generate new cex,
  /// otherwise behaves like standard verifier
  /// \return \see decision_proceduret::resultt
  decision_proceduret::resultt operator()(
      solutiont &solution, std::vector<counterexamplet> &counterexamples,
      bool force_new_cex);


  const counterexamplet &get_counterexample() const
  {
    return counterexample;
  }

  bool use_smt;
  std::string logic;

protected:
  const namespacet &ns;
  const problemt &problem;
  counterexamplet counterexample;

  void add_problem(verify_encodingt &, decision_proceduret &);

  void output(
    const solutiont::functionst &,
    std::ostream &);
};

#endif /* CPROVER_FASTSYNTH_VERIFY_H_ */

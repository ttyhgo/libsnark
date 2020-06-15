/** @file
 *****************************************************************************

 Declaration of functionality that runs the R1CS GG-ppzkSNARK for
 a given R1CS example.

 *****************************************************************************
 * @author     This file is part of libsnark, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef RUN_R1CS_CONV_PPZKSNARK_HPP_
#define RUN_R1CS_CONV_PPZKSNARK_HPP_

#include <libff/algebra/curves/public_params.hpp>

#include <libsnark/relations/constraint_satisfaction_problems/r1cs/examples/r1cs_examples.hpp>

namespace libsnark {

/**
 * Runs the ppzkSNARK (generator, prover, and verifier) for a given
 * R1CS example (specified by a constraint system, input, and witness).
 *
 * Optionally, also test the serialization routines for keys and proofs.
 * (This takes additional time.)
 */
template<typename ppT>
bool run_r1cs_conv_ppzksnark(const r1cs_example<libff::Fr<ppT> > &example,
                        const bool test_serialization);
template<typename ppT>
bool run_r1cs_conv_ppzksnark(const r1cs_example<libff::Fr<ppT> > &example, const r1cs_example<libff::Fr<ppT> > &example2,
                        const bool test_serialization);

} // libsnark

#include <libsnark/zk_proof_systems/ppzksnark/convol_snark/examples/run_r1cs_conv_ppzksnark.tcc>

#endif // RUN_R1CS_GG_PPZKSNARK_HPP_
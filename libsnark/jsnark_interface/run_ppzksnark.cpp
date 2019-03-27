/*
 * run_ppzksnark.cpp
 *
 *      Author: Ahmed Kosba
 *      Revised by Hyunok Oh at SnP Lab., Hanyang University
 */

#include "CircuitReader.hpp"
#include <libsnark/gadgetlib2/integration.hpp>
#include <libsnark/gadgetlib2/adapters.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/examples/run_r1cs_ppzksnark.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/r1cs_ppzksnark.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_gg_ppzksnark/examples/run_r1cs_gg_ppzksnark.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_gg_ppzksnark/r1cs_gg_ppzksnark.hpp>
#include <libsnark/common/default_types/r1cs_gg_ppzksnark_pp.hpp>
//#include <libsnark/zk_proof_systems/ppzksnark/r1cs_rom_se_ppzksnark/examples/run_r1cs_rom_se_ppzksnark.hpp>
//#include <libsnark/zk_proof_systems/ppzksnark/r1cs_rom_se_ppzksnark/r1cs_rom_se_ppzksnark.hpp>
//#include <libsnark/common/default_types/r1cs_rom_se_ppzksnark_pp.hpp>

typedef enum {Gro16, KLO18} ppzksnark_algorithm_type;

int main(int argc, char **argv) {

	libff::start_profiling();
	gadgetlib2::initPublicParamsFromDefaultPp();
	gadgetlib2::GadgetLibAdapter::resetVariableIndex();
	ProtoboardPtr pb = gadgetlib2::Protoboard::create(gadgetlib2::R1P);
	ppzksnark_algorithm_type ppzksnark_algorithm = KLO18;

	int inputStartIndex = 0;
	if(argc == 4){
		if(strncmp(argv[1], "gg",3) == 0) {
			ppzksnark_algorithm = Gro16;
			cout << "Using ppzksnark in the generic group model [Gro16]." << endl;
		}
		/*else if (strncmp(argv[1], "se",3) ==0){
			ppzksnark_algorithm = KLO18;
			cout << "Using ppzsnark in the generic group model [KLO18]." << endl;
		} 
		*/
		else{
			cout << "Invalid Argument - Terminating.." << endl;
			return -1;
		}
		inputStartIndex = 1;	
	} 	

	// Read the circuit, evaluate, and translate constraints
	CircuitReader reader(argv[1 + inputStartIndex], argv[2 + inputStartIndex], pb);
	r1cs_constraint_system<FieldT> cs =get_constraint_convol_system_from_gadgetlib2(*pb);// get_constraint_system_from_gadgetlib2(*pb);
	const r1cs_variable_assignment<FieldT> full_assignment =
			get_variable_assignment_from_gadgetlib2(*pb);
	cs.primary_input_size = reader.getNumInputs() + reader.getNumOutputs();
	cs.auxiliary_input_size = full_assignment.size() - cs.num_inputs();

	// extract primary and auxiliary input
	const r1cs_primary_input<FieldT> primary_input(full_assignment.begin(),
			full_assignment.begin() + cs.num_inputs());
	const r1cs_auxiliary_input<FieldT> auxiliary_input(
			full_assignment.begin() + cs.num_inputs(), full_assignment.end());

	vector<FieldT>::const_iterator first = full_assignment.begin();
	vector<FieldT>::const_iterator last = full_assignment.begin()
			+ reader.getNumInputs() + reader.getNumOutputs();
	vector<FieldT> newVec(first, last);


	// only print the circuit output values if both flags MONTGOMERY and BINARY outputs are off (see CMakeLists file)
	// In the default case, these flags should be ON for faster performance.

#if !defined(MONTGOMERY_OUTPUT) && !defined(OUTPUT_BINARY)
	cout << endl << "Printing output assignment in readable format:: " << endl;
	std::vector<Wire> outputList = reader.getOutputWireIds();
	int start = reader.getNumInputs();
	int end = reader.getNumInputs() +reader.getNumOutputs();	
	for (int i = start ; i < end; i++) {
		cout << "[output]" << " Value of Wire # " << outputList[i-reader.getNumInputs()] << " :: ";
		cout << primary_input[i];
		cout << endl;
	}
	cout << endl;
#endif

	//assert(cs.is_valid());
	//assert(cs.is_satisfied(primary_input, auxiliary_input));

	// removed cs.is_valid() check due to a suspected (off by 1) issue in a newly added check in their method.
        // A follow-up will be added.
	if(!cs.is_satisfied(primary_input, auxiliary_input)){
		cout << "The constraint system is  not satisifed by the value assignment - Terminating." << endl;
		return -1;
	}


	r1cs_example<FieldT> example(cs, primary_input, auxiliary_input);
	
	const bool test_serialization = false;
	bool successBit = false;
	if(argc == 3) {
		successBit = libsnark::run_r1cs_ppzksnark<libff::default_ec_pp>(example, test_serialization);

	} else if (ppzksnark_algorithm == Gro16) {
		// The following code makes use of the observation that 
		// libsnark::default_r1cs_gg_ppzksnark_pp is the same as libff::default_ec_pp (see r1cs_gg_ppzksnark_pp.hpp)
		// otherwise, the following code won't work properly, as GadgetLib2 is hardcoded to use libff::default_ec_pp.
		successBit = libsnark::run_r1cs_gg_ppzksnark<libsnark::default_r1cs_gg_ppzksnark_pp>(
			example, test_serialization);
	}
	/*
	else if (ppzksnark_algorithm == KLO18) {
		// The following code makes use of the observation that 
		// libsnark::default_r1cs_rom_se_ppzksnark_pp is the same as libff::default_ec_pp (see r1cs_rom_se_ppzksnark_pp.hpp)
		// otherwise, the following code won't work properly, as GadgetLib2 is hardcoded to use libff::default_ec_pp.
		successBit = libsnark::run_r1cs_rom_se_ppzksnark<libsnark::default_r1cs_rom_se_ppzksnark_pp>(
			example, test_serialization);
	}
	*/

	if(!successBit){
		cout << "Problem occurred while running the ppzksnark algorithms .. " << endl;
		return -1;
	}	
	return 0;
}


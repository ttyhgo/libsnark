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
#include <libsnark/zk_proof_systems/ppzksnark/convol_snark/examples/run_r1cs_oc_ppzksnark.hpp>
//#include <libsnark/zk_proof_systems/ppzksnark/r1cs_gg_ppzksnark/examples/run_r1cs_gg_ppzksnark.hpp>
//#include <libsnark/zk_proof_systems/ppzksnark/r1cs_gg_ppzksnark/r1cs_gg_ppzksnark.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/convol_snark/examples/run_r1cs_conv_ppzksnark.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/convol_snark/r1cs_gg_ppzksnark.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/convol_snark/r1cs_legosnark.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/convol_snark/r1cs_conv_ppzksnark.hpp>
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
	std::cout<<argc<<std::endl;

	if(argc == 6){
		std::cout<<"ok"<<std::endl;
		if(strncmp(argv[1],"conv",5) == 0){
			ppzksnark_algorithm = Gro16;
			inputStartIndex = 1;	

		}
	}
	else if(argc == 4){
		if(strncmp(argv[1], "oc", 3) == 0){
			ppzksnark_algorithm = Gro16;
			cout<<"Conv only"<<endl;
			inputStartIndex = 1;	
		}
		else if(strncmp(argv[1], "gg",3) == 0) {
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
	r1cs_constraint_system<FieldT> cs =get_constraint_system_from_gadgetlib2(*pb);
	//r1cs_constraint_system<FieldT> cs2 =get_constraint_convol_system_from_gadgetlib2(*pb);// get_constraint_system_from_gadgetlib2(*pb);
	r1cs_variable_assignment<FieldT> full_assignment = get_variable_assignment_from_gadgetlib2(*pb);
	// std::cout<<"var 1"<<std::endl;
	// for(size_t i =0;i<full_assignment.size();i++){
	// 	std::cout<<i<<", "<<full_assignment[i].as_ulong()<<std::endl;
	// }
	// std::cout<<"var 1 size : "<<full_assignment.size()<<std::endl;

	cs.primary_input_size = reader.getNumInputs() + reader.getNumOutputs();
	cs.auxiliary_input_size = full_assignment.size() - cs.num_inputs();
	cs.commit_input_size = reader.getCmNumInputs();

	r1cs_constraint_system<FieldT> cs2;
	r1cs_variable_assignment<FieldT> full_assignment2;
	// // for two lego r1cs cs3= conv, cs4 = nconv
	// r1cs_constraint_system<FieldT> cs3;
	// r1cs_variable_assignment<FieldT> full_assignment3;
	// r1cs_constraint_system<FieldT> cs4;
	// r1cs_variable_assignment<FieldT> full_assignment4;
	if(argc == 6){

		gadgetlib2::initPublicParamsFromDefaultPp();
		gadgetlib2::GadgetLibAdapter::resetVariableIndex();
		ProtoboardPtr pb2 = gadgetlib2::Protoboard::create(gadgetlib2::R1P);
		CircuitReader reader2(argv[3 + inputStartIndex], argv[4 + inputStartIndex], pb2);
		cs2 =get_constraint_convol_system_from_gadgetlib2(*pb2);
		full_assignment2 = get_variable_assignment_from_gadgetlib2(*pb2);
		cs2.primary_input_size = reader2.getNumInputs() + reader2.getNumOutputs();
		cs2.auxiliary_input_size = full_assignment2.size() - cs2.num_inputs();
		cs2.commit_input_size = reader2.getCmNumInputs();
		// std::cout<<cs2.primary_input_size<<", "<<cs2.auxiliary_input_size<<std::endl;
		// std::cout<<"var 2"<<std::endl;
		// for(size_t i =0;i<full_assignment2.size();i++){
		// 	std::cout<<i<<", "<<full_assignment2[i].as_ulong()<<std::endl;
		// }
		/*
		gadgetlib2::initPublicParamsFromDefaultPp();
		gadgetlib2::GadgetLibAdapter::resetVariableIndex();
		ProtoboardPtr pb3 = gadgetlib2::Protoboard::create(gadgetlib2::R1P);
		CircuitReader reader3(argv[5 + inputStartIndex], argv[6 + inputStartIndex], pb3);
		cs3 =get_constraint_convol_system_from_gadgetlib2(*pb3);
		full_assignment3 = get_variable_assignment_from_gadgetlib2(*pb3);
		cs3.primary_input_size = reader3.getNumInputs() + reader3.getNumOutputs();
		cs3.auxiliary_input_size = full_assignment3.size() - cs3.num_inputs();
		// std::cout<<cs3.primary_input_size<<", "<<cs3.auxiliary_input_size<<std::endl;
		// std::cout<<"var 3"<<std::endl;
		// for(size_t i =0;i<full_assignment3.size();i++){
		// 	std::cout<<i<<", "<<full_assignment3[i].as_ulong()<<std::endl;
		// }

		gadgetlib2::initPublicParamsFromDefaultPp();
		gadgetlib2::GadgetLibAdapter::resetVariableIndex();
		ProtoboardPtr pb4 = gadgetlib2::Protoboard::create(gadgetlib2::R1P);
		CircuitReader reader4(argv[7 + inputStartIndex], argv[8 + inputStartIndex], pb4);
		std::cout<<argv[7 + inputStartIndex]<<", "<<argv[8 + inputStartIndex]<<std::endl;
		cs4 =get_constraint_convol_system_from_gadgetlib2(*pb4);
		full_assignment4 = get_variable_assignment_from_gadgetlib2(*pb4);
		cs4.primary_input_size = reader4.getNumInputs() + reader4.getNumOutputs();
		cs4.auxiliary_input_size = full_assignment4.size() - cs4.num_inputs();
		// std::cout<<cs4.primary_input_size<<", "<<cs4.auxiliary_input_size<<std::endl;
		// std::cout<<"var 4"<<std::endl;
		// for(size_t i =0;i<full_assignment4.size();i++){
		// 	std::cout<<i<<", "<<full_assignment4[i].as_ulong()<<std::endl;
		// }
		*/
	}

	// extract primary and auxiliary input
	const r1cs_primary_input<FieldT> primary_input(full_assignment.begin(),
			full_assignment.begin() + cs.num_inputs());
	const r1cs_auxiliary_input<FieldT> auxiliary_input(
			full_assignment.begin() + cs.num_inputs(), full_assignment.end());
	const r1cs_primary_input<FieldT> primary_input2(full_assignment2.begin() ,
			full_assignment2.begin() + cs2.num_inputs() );
	const r1cs_auxiliary_input<FieldT> auxiliary_input2(
			full_assignment2.begin() + cs2.num_inputs(), full_assignment2.end());

	/*
	const r1cs_primary_input<FieldT> primary_input3(full_assignment3.begin() ,
			full_assignment3.begin() + cs3.num_inputs() );
	const r1cs_auxiliary_input<FieldT> auxiliary_input3(
			full_assignment3.begin() + cs3.num_inputs(), full_assignment3.end());


	const r1cs_primary_input<FieldT> primary_input4(full_assignment4.begin() ,
			full_assignment4.begin() + cs4.num_inputs() );
	const r1cs_auxiliary_input<FieldT> auxiliary_input4(
			full_assignment4.begin() + cs4.num_inputs(), full_assignment4.end());
	*/
	// const r1cs_primary_input<FieldT> primary_input2(full_assignment2.begin() + full_assignment.size(),
	// 		full_assignment2.begin() + cs2.num_inputs() + full_assignment.size());
	// const r1cs_auxiliary_input<FieldT> auxiliary_input2(
	// 		full_assignment2.begin() + cs2.num_inputs() + full_assignment.size(), full_assignment2.end());

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
		successBit = libsnark::run_r1cs_oc_ppzksnark<libff::default_ec_pp>(example, test_serialization);

	}else if(argc == 6){
		r1cs_example<FieldT> example2(cs2, primary_input2, auxiliary_input2);
		//r1cs_example<FieldT> example3(cs3, primary_input3, auxiliary_input3);
		//r1cs_example<FieldT> example4(cs4, primary_input4, auxiliary_input4);

		std::cout<<"lego"<<std::endl;
		successBit = libsnark::run_r1cs_conv_ppzksnark<libsnark::default_r1cs_gg_ppzksnark_pp>(
			example, example2, test_serialization);
	}
	
	
	//  else if (ppzksnark_algorithm == Gro16) {
	// 	// The following code makes use of the observation that 
	// 	// libsnark::default_r1cs_gg_ppzksnark_pp is the same as libff::default_ec_pp (see r1cs_gg_ppzksnark_pp.hpp)
	// 	// otherwise, the following code won't work properly, as GadgetLib2 is hardcoded to use libff::default_ec_pp.
	// 	successBit = libsnark::run_r1cs_gg_ppzksnark<libsnark::default_r1cs_gg_ppzksnark_pp>(
	// 		example, test_serialization);
	// }
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


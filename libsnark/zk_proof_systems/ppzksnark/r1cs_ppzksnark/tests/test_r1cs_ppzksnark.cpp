/** @file
 *****************************************************************************
 Test program that exercises the ppzkSNARK (first generator, then
 prover, then verifier) on a synthetic R1CS instance.

 *****************************************************************************
 * @author     This file is part of libsnark, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/
#include <cassert>
#include <cstdio>
#include<iostream>
#include<algorithm>
#include<set>

#include<libff/algebra/fields/bigint.hpp>
#include <libff/algebra/scalar_multiplication/multiexp.hpp>
#include <libff/common/profiling.hpp>
#include <libff/common/utils.hpp>

#include <libfqfft/evaluation_domain/get_evaluation_domain.hpp>
#include <libfqfft/polynomial_arithmetic/basic_operations.hpp>

#ifdef MULTICORE
#include <omp.h>
#endif

#include <libsnark/knowledge_commitment/kc_multiexp.hpp>


#include <libsnark/common/default_types/r1cs_ppzksnark_pp.hpp>
#include <libsnark/relations/constraint_satisfaction_problems/r1cs/examples/r1cs_examples.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/examples/run_r1cs_ppzksnark.hpp>
//#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/examples/R1CS.hpp>

#include<stdlib.h>


using namespace libsnark;

using namespace std;

template<typename FieldT>
void print_Poly(const vector<FieldT> c){
	for (size_t i = 0; i < c.size(); i++)
	{
		unsigned long coefficient = c[i].as_ulong();

		if (i == 0) std::cout << coefficient << " + ";
		else if (i < (c.size()-1)) std::cout << coefficient << "x^" << i << " + ";
		else std::cout << coefficient << "x^" << i << std::endl;
	}

}

template<typename FieldT>
void print_point(const vector<FieldT> f){
	for (size_t i = 0; i < f.size(); i++)
	{
		printf("%ld: %ld\n", i, f[i].as_ulong());
	}
}

	template<typename FieldT>
vector<FieldT> zxPolycalculate(vector<vector<FieldT>> zxPoly, 
		size_t num_constraints, size_t num_z_order,
		shared_ptr<libfqfft::evaluation_domain<FieldT>> domain
		)
{
	vector<vector<FieldT>> tempPoly(num_constraints, vector<FieldT>(num_constraints, FieldT::zero()));

	bool isEmpty = true;
	for(size_t i=0; i<num_constraints;i++){
		size_t j;
		for(j=0; j<num_z_order+1;j++){
			if(zxPoly[i][j] != FieldT::zero()){
				isEmpty = false;
				break;
			}
		}
		if(j < num_z_order+1) tempPoly[i][i] = FieldT::one();
	}

	if(isEmpty) return vector<FieldT>(1, FieldT::zero());

	for(size_t i=0; i<num_constraints;i++){
		domain->iFFT(tempPoly[i]);
	}

	//v0(x) * z(x^(4d+1))
	vector<FieldT> zExtendPoly((num_z_order*((num_constraints-1)*4+1))+1, FieldT::zero());
	for(size_t i=0;i<num_z_order+1;i++){
		zExtendPoly[i*((num_constraints-1)*4 + 1)] = zxPoly[0][i];
	}

	vector<FieldT> mulResult(1, FieldT::zero());
	libfqfft::_polynomial_multiplication(mulResult, tempPoly[0], zExtendPoly);

	//point add
	for(size_t i=0; i<num_constraints;i++){
		FieldT temp = FieldT::zero();
		for(size_t j=1; j<num_constraints;j++){
			temp+=tempPoly[j][i];
		}
		mulResult[i] += temp;
	}

	return mulResult;
}
	template<typename FieldT>
qap_instance_evaluation<FieldT> mr1cs_to_qap_instance_map_with_evaluation(
		const shared_ptr<libfqfft::evaluation_domain<FieldT> > &domain,
		const shared_ptr<libfqfft::evaluation_domain<FieldT> > &domain2,
		const shared_ptr<libfqfft::evaluation_domain<FieldT> > &domain4,
		const vector<vector<FieldT>> &V,
		const vector<vector<FieldT>> &W,
		const vector<vector<FieldT>> &Y,
		const size_t num_variables,
		const size_t num_inputs,
		const size_t num_constraints,
		const size_t num_z_order,
		const FieldT &t
		)
{
	libff::enter_block("Call to mr1cs_to_qap_instance_map_with_evaluation");


	const FieldT Zt = domain->compute_vanishing_polynomial(t);


	/*
	// check r1cs
	FieldT ckVk = FieldT::zero();
	FieldT ckWk = FieldT::zero();
	FieldT ckYk = FieldT::zero();
	FieldT omega = libff::get_root_of_unity<FieldT>(4);
	//omega = omega^16;
	for(size_t i=0; i<num_variables; i++){

	FieldT temp = FieldT::zero();
	for(size_t j=0;j<V[i].size();j++){
	temp += V[i][j] * (omega^j);
	}
	cout<<temp.as_ulong()<< " * " << wires[i].as_ulong() << " = "<< (temp*wires[i]).as_ulong() <<"\t";
	ckVk += temp;

	temp = FieldT::zero();
	for(size_t j=0;j<W[i].size();j++){
	temp += W[i][j] * (omega^j);
	}
	cout<<temp.as_ulong()<< " * " << wires[i].as_ulong() << " = "<< (temp*wires[i]).as_ulong() <<"\t";
	ckWk +=temp;

	temp = FieldT::zero();
	for(size_t j=0;j<Y[i].size();j++){
	temp += Y[i][j] * (omega^j);
	}
	cout<<temp.as_ulong()<< " * " << wires[i].as_ulong() << " = "<< (temp*wires[i]).as_ulong() <<endl;
	ckYk +=temp;
	}
	if(ckVk * ckWk == ckYk) cout<<"ok"<<endl;
	else cout<<"no"<<endl;
	 */

	vector<FieldT> At(domain2->m, FieldT::zero());
	vector<FieldT> Bt(domain2->m, FieldT::zero());
	vector<FieldT> Ct(domain2->m, FieldT::zero());
	vector<FieldT> Ht;
	Ht.reserve(domain4->m);
	FieldT ti = FieldT::one();
	for(size_t i=0; i<domain4->m;i++){
		Ht.emplace_back(ti);
		ti*= t;
	}

	//At, Bt, Ct = poly(t);
	for(size_t i=0; i<num_variables; i++){
		for(size_t j=0;j<V[i].size();j++){
			At[i] += V[i][j] * Ht[j];
		}

		for(size_t j=0;j<W[i].size();j++){
			Bt[i] += W[i][j] * Ht[j];
		}

		for(size_t j=0;j<Y[i].size();j++){
			Ct[i] += Y[i][j] * Ht[j];
		}
	}
	libff::leave_block("Call to mr1cs_to_qap_instance_map_with_evaluation");

	return qap_instance_evaluation<FieldT>(domain2,
			num_variables,
			domain2->m,
			num_inputs,
			t,
			move(At),
			move(Bt),
			move(Ct),
			move(Ht),
			Zt);



}

	template<typename FieldT>
qap_witness<FieldT> mr1cs_to_qap_witness_map(
		const shared_ptr<libfqfft::evaluation_domain<FieldT> > &domain2,
		vector<vector<FieldT>> &V,
		vector<vector<FieldT>> &W,
		vector<vector<FieldT>> &Y,
		vector<FieldT> &Ht,
		const FieldT Zt,
		const size_t num_variables,
		const size_t num_inputs,
		const size_t num_constraints,
		const size_t num_z_order,
		const vector<FieldT> &wires
		)
{
	const shared_ptr<libfqfft::evaluation_domain<FieldT> > domain = libfqfft::get_evaluation_domain<FieldT>(num_constraints);
	const shared_ptr<libfqfft::evaluation_domain<FieldT> > domain4 = libfqfft::get_evaluation_domain<FieldT>(domain2->m*2);

	///qap witness start

	libff::enter_block("calcuate V, W, Y");
	vector<FieldT> aA(domain2->m, FieldT::zero());
	vector<FieldT> aB(domain2->m, FieldT::zero());
	vector<FieldT> aC(domain2->m, FieldT::zero());

	// r1cs poly * wire
	for(size_t i=0; i<num_variables;i++){
		for(size_t j=0;j< V[i].size();j++){
			aA[j] += V[i][j] * wires[i];
		}
		for(size_t j=0;j< W[i].size();j++){
			aB[j] += W[i][j] * wires[i];
		}
		for(size_t j=0;j< Y[i].size();j++){
			aC[j] += Y[i][j] * wires[i];
		}
	}
	
	/*
	//DEBUG Calculate V(t), W(t), Y(t)
	FieldT resA3 = FieldT::zero();
	FieldT resB3 = FieldT::zero();
	FieldT resC3 = FieldT::zero();
	// res = poly(t) using lagrange
	   for(size_t i=0;i<domain2->m;i++){
	   resA3 += aA[i] * u[i];
	   resB3 += aB[i] * u[i];
	   resC3 += aC[i] * u[i];
	   }
	 */


	//A, B, C  point -> poly
	//domain2->iFFT(aA);
	//domain2->iFFT(aB);
	//domain2->iFFT(aC);
	libff::leave_block("calcuate V, W, Y");

	libff::enter_block("V, W, Y coset FFT");
	aA.resize(domain4->m, FieldT::zero());
	aB.resize(domain4->m, FieldT::zero());
	aC.resize(domain4->m, FieldT::zero());

	//A(x) => A(gx) same B, C
	domain4->cosetFFT(aA, FieldT::multiplicative_generator);
	domain4->cosetFFT(aB, FieldT::multiplicative_generator);
	domain4->cosetFFT(aC, FieldT::multiplicative_generator);
	libff::leave_block("V, W, Y coset FFT");

	libff::enter_block("claculate Ht");
	vector<FieldT> tmpH(domain4->m, FieldT::zero());
	for(size_t i=0;i<domain4->m;i++){
		tmpH[i] = aA[i] * aB[i] - aC[i];
	}

	//domain4->icosetFFT(aA, FieldT::multiplicative_generator);
	//domain4->icosetFFT(aB, FieldT::multiplicative_generator);
	//domain4->icosetFFT(aC, FieldT::multiplicative_generator);

	libff::enter_block("claculate Zt point");

	vector<FieldT> Zpoly(1, FieldT::one());
	vector<FieldT> xw(2, FieldT::one());
	for(size_t i=0;i<domain->m;i++){
		xw[0] = -domain->get_domain_element(i);
		libfqfft::_polynomial_multiplication(Zpoly, Zpoly, xw);
	}
	//Z(gx) points
	Zpoly.resize(domain4->m, FieldT::zero());
	domain4->cosetFFT(Zpoly, FieldT::multiplicative_generator);
	libff::leave_block("claculate Zt point");

	//(A*B-C(gx) point) / (Z(gx) point)
	for(size_t i=0;i<domain4->m;i++){
		tmpH[i] = tmpH[i] * Zpoly[i].inverse();
	}
	domain4->icosetFFT(tmpH, FieldT::multiplicative_generator);
	libff::leave_block("claculate Ht");

	//domain4->icosetFFT(Zpoly, FieldT::multiplicative_generator);

	/*
	//h(gx) -> h(x)
	FieldT resH = FieldT::zero();
	for(size_t i=0;i<domain4->m;i++){
		resH += tmpH[i] * Ht[i];
	}
	cout<<"cal resH = "<<resH.as_ulong()<<endl;

	FieldT resA2 = FieldT::zero();
	FieldT resB2 = FieldT::zero();
	FieldT resC2 = FieldT::zero();
	// res = poly(t)
	for(size_t i=0;i<domain2->m;i++){
		resA2 += aA[i] * Ht[i];
		resB2 += aB[i] * Ht[i];
		resC2 += aC[i] * Ht[i];
	}
	*/
	/*
	aA.resize(domain2->m);
	aB.resize(domain2->m);
	aC.resize(domain2->m);
	domain2->cosetFFT(aA, FieldT::multiplicative_generator);
	domain2->cosetFFT(aB, FieldT::multiplicative_generator);
	domain2->cosetFFT(aC, FieldT::multiplicative_generator);
	*/

	/*
	   FieldT resA = FieldT::zero();
	   FieldT resB = FieldT::zero();
	   FieldT resC = FieldT::zero();
	   for(size_t i=0;i< num_variables;i++){
	   resA += At[i] * wires[i];
	   resB += Bt[i] * wires[i];
	   resC += Ct[i] * wires[i];
	   }
	 */


	//if(((resA2 * resB2) - resC2) == (Zt * resH)) cout<< "ok2"<<endl;
	//else cout <<"no2"<<endl;

	/*
	   cout<<"A = "<<resA.as_ulong() <<", A2 = "<<resA2.as_ulong()<< ", A3 = "<< resA3.as_ulong()<<endl;
	   cout << (resA == resA2) << endl;
	   cout<<"B = "<<resB.as_ulong() <<", B2 = "<<resB2.as_ulong()<< ", B3 = "<<resB3.as_ulong()<<endl;
	   cout << (resA == resA2) << endl;
	   cout<<"C = "<<resC.as_ulong() <<", C2 = "<<resC2.as_ulong()<< ", C3 = "<<resC3.as_ulong()<<endl;
	   cout << (resA == resA2) << endl;

	   cout<<"resA * resB - resC / Zt = "<< ((resA*resB - resC)*Zt.inverse()).as_ulong()<<endl;
	   cout<<"H = "<<resH.as_ulong()<<endl;
	   cout<<"Z = "<<Zt.as_ulong()<<endl;
	   cout<<"Z^-1 = "<<Zt.inverse().as_ulong()<<endl;
	 */
	return qap_witness<FieldT>(num_variables,
			domain2->m,
			num_inputs,
			FieldT::zero(),
			FieldT::zero(),
			FieldT::zero(),
			wires,
			move(tmpH)
			);
}

template<typename ppT>
void evaluate_mqap(size_t num_a, size_t num_x){

	//size_t num_a = 50;
	//size_t num_x = 100;
	size_t num_y = num_a+num_x-1;
	size_t num_variables = num_a+num_x+num_y;
	size_t num_inputs = num_a+num_x;
	size_t num_constraints = 3;
	size_t num_z_order = num_y-1;



	/*
	   1	a0	1	1	0		0	0	0		0	0	0
	   2	a1	s	0	0		0	0	0		0	0	0
	   3	a2	s^2	0	1		0	0	0		0	0	0
	   1	x0	0	0	0		1	1	0		0	0	0
	   2	x1	0	0	0		s	0	0		0	0	0
	   3	x2	0	0	0		s^2	0	0		0	0	0
	   4	x3	0	0	0		s^3	0	0		0	0	0
	   5	x4	0	0	0		s^4	0	1		0	0	0
	   1	y0	0	0	0		0	0	0		1	1	0
	   4	y1	0	0	0		0	0	0		s	0	0
	   10	y2	0	0	0		0	0	0		s^2	0	0
	   16	y3	0	0	0		0	0	0		s^3	0	0
	   22	y4	0	0	0		0	0	0		s^4	0	0
	   22	y5	0	0	0		0	0	0		s^5	0	0
	   15	y6	0	0	0		0	0	0		s^6	0	1

	 */

	const shared_ptr<libfqfft::evaluation_domain<libff::Fr<ppT>> > domain = libfqfft::get_evaluation_domain<libff::Fr<ppT>>(num_constraints);
	const shared_ptr<libfqfft::evaluation_domain<libff::Fr<ppT>> > domain2 = libfqfft::get_evaluation_domain<libff::Fr<ppT>>(num_z_order*((num_constraints-1)*4+1) + (num_constraints-1) +1);
	const shared_ptr<libfqfft::evaluation_domain<libff::Fr<ppT>> > domain4 = libfqfft::get_evaluation_domain<libff::Fr<ppT>>(domain2->m*2);
	cout<<"degree1 : "<<domain->m<<endl;
	cout<<"degree2 : "<<domain2->m<<endl;
	cout<<"degree4 : "<<domain4->m<<endl;

	vector<vector<vector<libff::Fr<ppT>>>> mr1csV (num_variables, vector<vector<libff::Fr<ppT>>>(num_constraints, vector<libff::Fr<ppT>>(num_z_order+1, libff::Fr<ppT>::zero())));
	vector<vector<vector<libff::Fr<ppT>>>> mr1csW (num_variables, vector<vector<libff::Fr<ppT>>>(num_constraints, vector<libff::Fr<ppT>>(num_z_order+1, libff::Fr<ppT>::zero())));
	vector<vector<vector<libff::Fr<ppT>>>> mr1csY (num_variables, vector<vector<libff::Fr<ppT>>>(num_constraints, vector<libff::Fr<ppT>>(num_z_order+1, libff::Fr<ppT>::zero())));

	/*
	vector<libff::Fr<ppT>> wires={
		libff::Fr<ppT>(1), libff::Fr<ppT>(2), libff::Fr<ppT>(3), //a
		libff::Fr<ppT>(1), libff::Fr<ppT>(2), libff::Fr<ppT>(3), libff::Fr<ppT>(4), libff::Fr<ppT>(5), //x
		libff::Fr<ppT>(1), libff::Fr<ppT>(4), libff::Fr<ppT>(10), libff::Fr<ppT>(16), libff::Fr<ppT>(22), libff::Fr<ppT>(22), libff::Fr<ppT>(15) //y
	};
	*/
	vector<libff::Fr<ppT>> wires(num_variables, libff::Fr<ppT>::zero());
	for(size_t i=0; i<num_a;i++){
		wires[i] = libff::Fr<ppT>(i+1);
	}
	for(size_t i=num_a; i<num_a+num_x;i++){
		wires[i] = libff::Fr<ppT>(i-num_a+1);
	}
	for(size_t i=0; i<num_y;i++){
		//cout<<i<<" ";
		for(size_t j=0;j<num_x;j++){
			for(size_t k=0; k<num_a;k++){
				if((j+k) == i) 
				{
					wires[i+num_a+num_x] += wires[k] * wires[num_a + j];
					//cout<< "a"<<k<<" * x"<<j<<" + ";
				}
			}
		}
		//cout<<endl;
	}

	cout<<"output = ";
	for(size_t i=0; i<num_y;i++){
		cout <<wires[i+num_a+num_x].as_ulong()<<" ";
	}
	cout<<endl;
	

	//V points
	for(size_t i=0; i<num_a;i++){
		mr1csV[i][0][i] = libff::Fr<ppT>::one();
	}

	mr1csV[0][1][0] = libff::Fr<ppT>::one();
	mr1csV[num_a-1][2][0] = libff::Fr<ppT>::one();

	//W points
	for(size_t i=0; i<num_x;i++){
		mr1csW[num_a+i][0][i] = libff::Fr<ppT>::one();
	}
	mr1csW[num_a][1][0] = libff::Fr<ppT>::one();

	mr1csW[num_a+num_x-1][2][0] = libff::Fr<ppT>::one();

	//Y points
	for(size_t i=0; i<num_y;i++){
		mr1csY[num_a+num_x+i][0][i] = libff::Fr<ppT>::one();
	}
	mr1csY[num_a+num_x][1][0] = libff::Fr<ppT>::one();

	mr1csY[num_variables-1][2][0] = libff::Fr<ppT>::one();


	libff::enter_block("Compute V, W, Y");
	cout<<"===========Calculate V==========="<<endl;
	vector<vector<libff::Fr<ppT>>> V(num_variables, vector<libff::Fr<ppT>>(1, libff::Fr<ppT>::zero()));
	for(size_t i=0; i<num_variables; i++){
		V[i] = zxPolycalculate(mr1csV[i], num_constraints, num_z_order, domain);	
	}


	cout<<"===========Calculate W==========="<<endl;
	vector<vector<libff::Fr<ppT>>> W(num_variables, vector<libff::Fr<ppT>>(1, libff::Fr<ppT>::zero()));
	for(size_t i=0; i<num_variables; i++){
		W[i] = zxPolycalculate(mr1csW[i], num_constraints, num_z_order, domain);	
	}

	cout<<"===========Calculate Y==========="<<endl;
	vector<vector<libff::Fr<ppT>>> Y(num_variables, vector<libff::Fr<ppT>>(1, libff::Fr<ppT>::zero()));
	for(size_t i=0; i<num_variables; i++){
		Y[i] = zxPolycalculate(mr1csY[i], num_constraints, num_z_order, domain);	
	}
	libff::leave_block("Compute V, W, Y");

	libff::enter_block("Call to r1cs_ppzksnark_generator");
	libff::Fr<ppT> t = libff::Fr<ppT>::random_element();
	
	qap_instance_evaluation<libff::Fr<ppT>> qap_inst =  mr1cs_to_qap_instance_map_with_evaluation(domain, domain2, domain4, V, W, Y, num_variables, num_inputs, num_constraints, num_z_order, t);

	libff::print_indent(); printf("* QAP number of variables: %zu\n", qap_inst.num_variables());
	libff::print_indent(); printf("* QAP pre degree: %zu\n", num_constraints);
	libff::print_indent(); printf("* QAP degree: %zu\n", domain2->m-1);
	libff::print_indent(); printf("* QAP number of input variables: %zu\n", qap_inst.num_inputs());

	libff::enter_block("Compute query densities");
	size_t non_zero_At = 0, non_zero_Bt = 0, non_zero_Ct = 0, non_zero_Ht = 0;
	cout<<"non zero C index : ";
	for (size_t i = 0; i < qap_inst.num_variables()+1; ++i)
	{   
		if (!qap_inst.At[i].is_zero())
		{   
			++non_zero_At;
		}   
		if (!qap_inst.Bt[i].is_zero())
		{   
			++non_zero_Bt;
		}   
		if (!qap_inst.Ct[i].is_zero())
		{   
			cout<<i<<" ";
			++non_zero_Ct;
		}   
	}   
	for (size_t i = 0; i < qap_inst.degree()+1; ++i)
	{   
		if (!qap_inst.Ht[i].is_zero())
		{   
			++non_zero_Ht;
		}   
	}   
	libff::leave_block("Compute query densities");

	libff::Fr_vector<ppT> At = std::move(qap_inst.At); // qap_inst.At is now in unspecified state, but we do not use it later
	libff::Fr_vector<ppT> Bt = std::move(qap_inst.Bt); // qap_inst.Bt is now in unspecified state, but we do not use it later
	libff::Fr_vector<ppT> Ct = std::move(qap_inst.Ct); // qap_inst.Ct is now in unspecified state, but we do not use it later
	libff::Fr_vector<ppT> Ht = std::move(qap_inst.Ht); // qap_inst.Ht is now in unspecified state, but we do not use it later

	/* append Zt to At,Bt,Ct with 
	   At.emplace_back(qap_inst.Zt);
	   Bt.emplace_back(qap_inst.Zt);
	   Ct.emplace_back(qap_inst.Zt);
	 */

	const  libff::Fr<ppT> alphaA = libff::Fr<ppT>::random_element(),
		   alphaB = libff::Fr<ppT>::random_element(),
		   alphaC = libff::Fr<ppT>::random_element(),
		   rA = libff::Fr<ppT>::random_element(),
		   rB = libff::Fr<ppT>::random_element(),
		   beta = libff::Fr<ppT>::random_element(),
		   gamma = libff::Fr<ppT>::random_element();
	const libff::Fr<ppT>      rC = rA * rB;

	// consrtuct the same-coefficient-check query (must happen before zeroing out the prefix of At)
	libff::Fr_vector<ppT> Kt;
	Kt.reserve(qap_inst.num_variables());
	for (size_t i = 0; i < qap_inst.num_variables(); ++i)
	{
		Kt.emplace_back( beta * (rA * At[i] + rB * Bt[i] + rC * Ct[i] ) );
	}
	//Kt.emplace_back(beta * rA * qap_inst.Zt);
	//Kt.emplace_back(beta * rB * qap_inst.Zt);
	//Kt.emplace_back(beta * rC * qap_inst.Zt);

	
	/* zero out prefix of At and stick it into IC coefficients */
	/*
	libff::Fr_vector<ppT> IC_coefficients;
	IC_coefficients.reserve(qap_inst.num_inputs() + 1);
	for (size_t i = 0; i < qap_inst.num_inputs() + 1; ++i)
	{
		IC_coefficients.emplace_back(At[i]);
		assert(!IC_coefficients[i].is_zero());
		At[i] = libff::Fr<ppT>::zero();
	}
	*/

	const size_t g1_exp_count = 2*(non_zero_At - qap_inst.num_inputs() + non_zero_Ct) + non_zero_Bt + non_zero_Ht + Kt.size();
	const size_t g2_exp_count = non_zero_Bt;
	cout<<"g1 exp = :"<<g1_exp_count<<endl;

	size_t g1_window = libff::get_exp_window_size<libff::G1<ppT> >(g1_exp_count);
	size_t g2_window = libff::get_exp_window_size<libff::G2<ppT> >(g2_exp_count);
	libff::print_indent(); printf("* G1 window: %zu\n", g1_window);
	libff::print_indent(); printf("* G2 window: %zu\n", g2_window);

#ifdef MULTICORE
	const size_t chunks = omp_get_max_threads(); // to override, set OMP_NUM_THREADS env var or call omp_set_num_threads()
#else
	const size_t chunks = 1;
#endif

	libff::enter_block("Generating G1 multiexp table");
	libff::window_table<libff::G1<ppT> > g1_table = get_window_table(libff::Fr<ppT>::size_in_bits(), g1_window, libff::G1<ppT>::one());
	libff::leave_block("Generating G1 multiexp table");


	libff::enter_block("Generating G2 multiexp table");
	libff::window_table<libff::G2<ppT> > g2_table = get_window_table(libff::Fr<ppT>::size_in_bits(), g2_window, libff::G2<ppT>::one());
	libff::leave_block("Generating G2 multiexp table");

	libff::enter_block("Generate R1CS proving key");

	libff::enter_block("Generate knowledge commitments");
	libff::enter_block("Compute the A-query", false);
	knowledge_commitment_vector<libff::G1<ppT>, libff::G1<ppT> > A_query = kc_batch_exp(libff::Fr<ppT>::size_in_bits(), g1_window, g1_window, g1_table, g1_table, rA, rA*alphaA, At, chunks);
	libff::leave_block("Compute the A-query", false);

	libff::enter_block("Compute the B-query", false);
	knowledge_commitment_vector<libff::G2<ppT>, libff::G1<ppT> > B_query = kc_batch_exp(libff::Fr<ppT>::size_in_bits(), g2_window, g1_window, g2_table, g1_table, rB, rB*alphaB, Bt, chunks);
	libff::leave_block("Compute the B-query", false);

	libff::enter_block("Compute the C-query", false);
	knowledge_commitment_vector<libff::G1<ppT>, libff::G1<ppT> > C_query = kc_batch_exp(libff::Fr<ppT>::size_in_bits(), g1_window, g1_window, g1_table, g1_table, rC, rC*alphaC, Ct, chunks);
	libff::leave_block("Compute the C-query", false);

	libff::enter_block("Compute the H-query", false);
	libff::G1_vector<ppT> H_query = batch_exp(libff::Fr<ppT>::size_in_bits(), g1_window, g1_table, Ht);
#ifdef USE_MIXED_ADDITION
	libff::batch_to_special<libff::G1<ppT> >(H_query);
#endif
	libff::leave_block("Compute the H-query", false);

	libff::enter_block("Compute the K-query", false);
	libff::G1_vector<ppT> K_query = batch_exp(libff::Fr<ppT>::size_in_bits(), g1_window, g1_table, Kt);
#ifdef USE_MIXED_ADDITION
	libff::batch_to_special<libff::G1<ppT> >(K_query);
#endif
	libff::leave_block("Compute the K-query", false);

	libff::leave_block("Generate knowledge commitments");

	libff::leave_block("Generate R1CS proving key");

	libff::enter_block("Generate R1CS verification key");
	libff::G2<ppT> alphaA_g2 = alphaA * libff::G2<ppT>::one();
	libff::G1<ppT> alphaB_g1 = alphaB * libff::G1<ppT>::one();
	libff::G2<ppT> alphaC_g2 = alphaC * libff::G2<ppT>::one();
	libff::G2<ppT> gamma_g2 = gamma * libff::G2<ppT>::one();
	libff::G1<ppT> gamma_beta_g1 = (gamma * beta) * libff::G1<ppT>::one();
	libff::G2<ppT> gamma_beta_g2 = (gamma * beta) * libff::G2<ppT>::one();
	libff::G2<ppT> rC_Z_g2 = (rC * qap_inst.Zt) * libff::G2<ppT>::one();

	/*
	libff::enter_block("Encode IC query for R1CS verification key");
	libff::G1<ppT> encoded_IC_base = (rA * IC_coefficients[0]) * libff::G1<ppT>::one();
	libff::Fr_vector<ppT> multiplied_IC_coefficients;
	multiplied_IC_coefficients.reserve(qap_inst.num_inputs());
	for (size_t i = 1; i < qap_inst.num_inputs() + 1; ++i)
	{
		multiplied_IC_coefficients.emplace_back(rA * IC_coefficients[i]);
	}
	libff::G1_vector<ppT> encoded_IC_values = batch_exp(libff::Fr<ppT>::size_in_bits(), g1_window, g1_table, multiplied_IC_coefficients);

	libff::leave_block("Encode IC query for R1CS verification key");

	accumulation_vector<libff::G1<ppT> > encoded_IC_query(std::move(encoded_IC_base), std::move(encoded_IC_values));
	*/
	libff::leave_block("Generate R1CS verification key");

	libff::leave_block("Call to r1cs_ppzksnark_generator");

	/*
	   r1cs_ppzksnark_verification_key<ppT> vk = r1cs_ppzksnark_verification_key<ppT>(alphaA_g2,
	   alphaB_g1,
	   alphaC_g2,
	   gamma_g2,
	   gamma_beta_g1,
	   gamma_beta_g2,
	   rC_Z_g2,
	   encoded_IC_query);

	//r1cs_ppzksnark_proving_key<ppT> pk = r1cs_ppzksnark_proving_key<ppT>(std::move(A_query),
	std::move(B_query),
	std::move(C_query),
	std::move(H_query),
	std::move(K_query),
	std::move(cs.copy());

	pk.print_size();
	vk.print_size();
	 */

	/////////////prove generate/////////////////////////////


	cout<<"==========================prove gen================"<<endl;
	libff::enter_block("Call to r1cs_ppzksnark_prover");

	/* zero know
	   const libff::Fr<ppT> d1 = libff::Fr<ppT>::random_element(),
	   d2 = libff::Fr<ppT>::random_element(),
	   d3 = libff::Fr<ppT>::random_element();
	 */

	libff::enter_block("Compute the polynomial H");
	const qap_witness<libff::Fr<ppT>> qap_wit =  mr1cs_to_qap_witness_map(qap_inst.domain, V, W, Y, Ht, qap_inst.Zt, num_variables, num_inputs, num_constraints, num_z_order, wires);
	cout<<"QAP degree dm2 : "<<qap_wit.degree()<<endl;
	libff::leave_block("Compute the polynomial H");



	knowledge_commitment<libff::G1<ppT>, libff::G1<ppT> > g_A;// = A_query[0] + qap_wit.d1*pk.A_query[qap_wit.num_variables()+1];
	knowledge_commitment<libff::G2<ppT>, libff::G1<ppT> > g_B;// = B_query[0] + qap_wit.d2*pk.B_query[qap_wit.num_variables()+1];
	knowledge_commitment<libff::G1<ppT>, libff::G1<ppT> > g_C;// = C_query[0] + qap_wit.d3*pk.C_query[qap_wit.num_variables()+1];


	libff::G1<ppT> g_H = libff::G1<ppT>::zero();
	libff::G1<ppT> g_K;// = (K_query[0]);
									/* +
										 qap_wit.d1*pk.K_query[qap_wit.num_variables()+1] +
										 qap_wit.d2*pk.K_query[qap_wit.num_variables()+2] +
										 qap_wit.d3*pk.K_query[qap_wit.num_variables()+3]);
									   */

#ifdef DEBUG
	for (size_t i = 0; i < qap_wit.num_inputs() + 1; ++i)
	{
		//assert(A_query[i].g == libff::G1<ppT>::zero());
	}
	assert(A_query.domain_size() == qap_wit.num_variables());
	assert(B_query.domain_size() == qap_wit.num_variables());
	assert(C_query.domain_size() == qap_wit.num_variables());
	//assert(H_query.size() == qap_wit.degree()+1);
	assert(K_query.size() == qap_wit.num_variables());
#endif


	libff::enter_block("Compute the proof");

	libff::enter_block("Compute answer to A-query", false);
	// erase g0
	g_A = kc_multi_exp_with_mixed_addition<libff::G1<ppT>,
		libff::G1<ppT>,
		libff::Fr<ppT>,
		libff::multi_exp_method_bos_coster>(
				A_query,
				0, qap_wit.num_variables(),
				qap_wit.coefficients_for_ABCs.begin(), qap_wit.coefficients_for_ABCs.begin()+qap_wit.num_variables(),
				chunks);
	libff::leave_block("Compute answer to A-query", false);

	libff::enter_block("Compute answer to B-query", false);
	g_B = kc_multi_exp_with_mixed_addition<libff::G2<ppT>,
		libff::G1<ppT>,
		libff::Fr<ppT>,
		libff::multi_exp_method_bos_coster>(
				B_query,
				0, qap_wit.num_variables(),
				qap_wit.coefficients_for_ABCs.begin(), qap_wit.coefficients_for_ABCs.begin()+qap_wit.num_variables(),
				chunks);
	libff::leave_block("Compute answer to B-query", false);

	libff::enter_block("Compute answer to C-query", false);
	g_C = kc_multi_exp_with_mixed_addition<libff::G1<ppT>,
		libff::G1<ppT>,
		libff::Fr<ppT>,
		libff::multi_exp_method_bos_coster>(
				C_query,
				0, qap_wit.num_variables(),
				qap_wit.coefficients_for_ABCs.begin(), qap_wit.coefficients_for_ABCs.begin()+qap_wit.num_variables(),
				chunks);
	libff::leave_block("Compute answer to C-query", false);

	libff::enter_block("Compute answer to H-query", false);
	g_H = g_H + libff::multi_exp<libff::G1<ppT>,
		libff::Fr<ppT>,
		libff::multi_exp_method_BDLO12>(
				H_query.begin(), H_query.begin()+qap_wit.degree()+1,
				qap_wit.coefficients_for_H.begin(), qap_wit.coefficients_for_H.begin()+qap_wit.degree()+1,
				chunks);
	libff::leave_block("Compute answer to H-query", false);

	libff::enter_block("Compute answer to K-query", false);
	g_K = libff::multi_exp_with_mixed_addition<libff::G1<ppT>,
		libff::Fr<ppT>,
		libff::multi_exp_method_bos_coster>(
				K_query.begin(), K_query.begin()+qap_wit.num_variables(),
				qap_wit.coefficients_for_ABCs.begin(), qap_wit.coefficients_for_ABCs.begin()+qap_wit.num_variables(),
				chunks);
	libff::leave_block("Compute answer to K-query", false);

	libff::leave_block("Compute the proof");

	libff::leave_block("Call to r1cs_ppzksnark_prover");


	cout<<"===============start verify==============="<<endl;
	libff::G2_precomp<ppT> pp_G2_one_precomp        = ppT::precompute_G2(libff::G2<ppT>::one());
	libff::G2_precomp<ppT> vk_alphaA_g2_precomp     = ppT::precompute_G2(alphaA_g2);
	libff::G1_precomp<ppT> vk_alphaB_g1_precomp     = ppT::precompute_G1(alphaB_g1);
	libff::G2_precomp<ppT> vk_alphaC_g2_precomp     = ppT::precompute_G2(alphaC_g2);
	libff::G2_precomp<ppT> vk_rC_Z_g2_precomp       = ppT::precompute_G2(rC_Z_g2);
	libff::G2_precomp<ppT> vk_gamma_g2_precomp      = ppT::precompute_G2(gamma_g2);
	libff::G1_precomp<ppT> vk_gamma_beta_g1_precomp = ppT::precompute_G1(gamma_beta_g1);
	libff::G2_precomp<ppT> vk_gamma_beta_g2_precomp = ppT::precompute_G2(gamma_beta_g2);

	vector<libff::Fr<ppT>> primary_input(num_inputs, libff::Fr<ppT>::zero());
	for(size_t i=0;i<num_inputs;i++){
		primary_input[i] = wires[i];
	}
	libff::enter_block("Call to r1cs_ppzksnark_online_verifier_weak_IC");
	//assert(encoded_IC_query.domain_size() >= primary_input.size());

	libff::enter_block("Compute input-dependent part of A");
	//const accumulation_vector<libff::G1<ppT> > accumulated_IC = encoded_IC_query.template accumulate_chunk<libff::Fr<ppT> >(primary_input.begin(), primary_input.end(), 0);
	//const libff::G1<ppT> &acc = accumulated_IC.first;
	libff::leave_block("Compute input-dependent part of A");

	bool result = true;

	/*
	libff::enter_block("Check if the proof is well-formed");
	if (!proof.is_well_formed())
	{
		if (!libff::inhibit_profiling_info)
		{   
			libff::print_indent(); printf("At least one of the proof elements does not lie on the curve.\n");
		}
		result = false;
	}
	libff::leave_block("Check if the proof is well-formed");
	*/

	libff::enter_block("Online pairing computations");
	libff::enter_block("Check knowledge commitment for A is valid");
	libff::G1_precomp<ppT> proof_g_A_g_precomp      = ppT::precompute_G1(g_A.g);
	libff::G1_precomp<ppT> proof_g_A_h_precomp = ppT::precompute_G1(g_A.h);
	libff::Fqk<ppT> kc_A_1 = ppT::miller_loop(proof_g_A_g_precomp,      vk_alphaA_g2_precomp);
	libff::Fqk<ppT> kc_A_2 = ppT::miller_loop(proof_g_A_h_precomp, pp_G2_one_precomp);
	libff::GT<ppT> kc_A = ppT::final_exponentiation(kc_A_1 * kc_A_2.unitary_inverse());

	if (kc_A != libff::GT<ppT>::one())
	{
		if (!libff::inhibit_profiling_info)
		{
			libff::print_indent(); printf("Knowledge commitment for A query incorrect.\n");
		}
		result = false;
	}
	libff::leave_block("Check knowledge commitment for A is valid");

	libff::enter_block("Check knowledge commitment for B is valid");
	libff::G2_precomp<ppT> proof_g_B_g_precomp      = ppT::precompute_G2(g_B.g);
	libff::G1_precomp<ppT> proof_g_B_h_precomp = ppT::precompute_G1(g_B.h);
	libff::Fqk<ppT> kc_B_1 = ppT::miller_loop(vk_alphaB_g1_precomp, proof_g_B_g_precomp);
	libff::Fqk<ppT> kc_B_2 = ppT::miller_loop(proof_g_B_h_precomp,    pp_G2_one_precomp);
	libff::GT<ppT> kc_B = ppT::final_exponentiation(kc_B_1 * kc_B_2.unitary_inverse());
	if (kc_B != libff::GT<ppT>::one())
	{
		if (!libff::inhibit_profiling_info)
		{
			libff::print_indent(); printf("Knowledge commitment for B query incorrect.\n");
		}
		result = false;
	}
	libff::leave_block("Check knowledge commitment for B is valid");

	libff::enter_block("Check knowledge commitment for C is valid");
	libff::G1_precomp<ppT> proof_g_C_g_precomp      = ppT::precompute_G1(g_C.g);
	libff::G1_precomp<ppT> proof_g_C_h_precomp = ppT::precompute_G1(g_C.h);
	libff::Fqk<ppT> kc_C_1 = ppT::miller_loop(proof_g_C_g_precomp,      vk_alphaC_g2_precomp);
	libff::Fqk<ppT> kc_C_2 = ppT::miller_loop(proof_g_C_h_precomp, pp_G2_one_precomp);
	libff::GT<ppT> kc_C = ppT::final_exponentiation(kc_C_1 * kc_C_2.unitary_inverse());
	if (kc_C != libff::GT<ppT>::one())
	{
		if (!libff::inhibit_profiling_info)
		{
			libff::print_indent(); printf("Knowledge commitment for C query incorrect.\n");
		}
		result = false;
	}
	libff::leave_block("Check knowledge commitment for C is valid");

	libff::enter_block("Check QAP divisibility");
	// check that g^((A+acc)*B)=g^(H*\Prod(t-\sigma)+C)
	// equivalently, via pairings, that e(g^(A+acc), g^B) = e(g^H, g^Z) + e(g^C, g^1)
	libff::G1_precomp<ppT> proof_g_A_g_acc_precomp = ppT::precompute_G1(g_A.g);// + acc);
	libff::G1_precomp<ppT> proof_g_H_precomp       = ppT::precompute_G1(g_H);
	libff::Fqk<ppT> QAP_1  = ppT::miller_loop(proof_g_A_g_acc_precomp,  proof_g_B_g_precomp);
	libff::Fqk<ppT> QAP_23  = ppT::double_miller_loop(proof_g_H_precomp, vk_rC_Z_g2_precomp, proof_g_C_g_precomp, pp_G2_one_precomp);
	libff::GT<ppT> QAP = ppT::final_exponentiation(QAP_1 * QAP_23.unitary_inverse());

	if (QAP != libff::GT<ppT>::one())
	{
		if (!libff::inhibit_profiling_info)
		{
			libff::print_indent(); printf("QAP divisibility check failed.\n");
		}
		result = false;
	}
	libff::leave_block("Check QAP divisibility");

	libff::enter_block("Check same coefficients were used");
	libff::G1_precomp<ppT> proof_g_K_precomp = ppT::precompute_G1(g_K);
	libff::G1_precomp<ppT> proof_g_A_g_acc_C_precomp = ppT::precompute_G1((g_A.g)+ g_C.g);// + acc) + g_C.g);
	libff::Fqk<ppT> K_1 = ppT::miller_loop(proof_g_K_precomp, vk_gamma_g2_precomp);
	libff::Fqk<ppT> K_23 = ppT::double_miller_loop(proof_g_A_g_acc_C_precomp, vk_gamma_beta_g2_precomp, vk_gamma_beta_g1_precomp, proof_g_B_g_precomp);
	libff::GT<ppT> K = ppT::final_exponentiation(K_1 * K_23.unitary_inverse());
	if (K != libff::GT<ppT>::one())
	{
		if (!libff::inhibit_profiling_info)
		{
			libff::print_indent(); printf("Same-coefficient check failed.\n");
		}
		result = false;
	}
	libff::leave_block("Check same coefficients were used");
	libff::leave_block("Online pairing computations");
	libff::leave_block("Call to r1cs_ppzksnark_online_verifier_weak_IC");

	cout<<"result : "<< result<<endl;


}

template<typename ppT>
void testPoly(int num_a, int num_x){
	evaluate_mqap<ppT>(num_a, num_x);//libff::Fr<ppT>>();
}

	template<typename ppT>
void test_r1cs_ppzksnark(size_t num_constraints,
		size_t input_size)
{
	libff::print_header("(enter) Test R1CS ppzkSNARK");

	const bool test_serialization = true;
	r1cs_example<libff::Fr<ppT> > example = generate_r1cs_example_with_binary_input<libff::Fr<ppT> >(num_constraints, input_size);
	const bool bit = run_r1cs_ppzksnark<ppT>(example, test_serialization);
	assert(bit);

	libff::print_header("(leave) Test R1CS ppzkSNARK");
}

int main(int argc, char* argv[])
{
	default_r1cs_ppzksnark_pp::init_public_params();
	libff::start_profiling();
	if(argc <2) cout<<"wrong"<<endl;

	testPoly<default_r1cs_ppzksnark_pp>(stoi(argv[1]), stoi(argv[2]));
	//test_r1cs_ppzksnark<default_r1cs_ppzksnark_pp>(10000, 100);
}


// PenalizedSpline312.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <vector>
#include "../Alglib/cpp/src/interpolation.h"
using namespace alglib;
using namespace std;
float multiSinusoidal(float x)
{
	const double pi = std::acos(-1);
	float retval = 1 / (1 + 5 * pow(x, 2));
	retval += 0.1 * sin(10 * pi * x);
	retval += 0.1 * cos(100 * pi * x);
	return retval;
}


void Out(std::ofstream& of, alglib::real_1d_array& x, spline1dinterpolant s)
{
	for (int i = 0; i < x.length(); i++) {
		auto r = alglib::spline1dcalc(s, x[i]);
		of << r << ",";
	}
	of << std::endl;

}

void OutDeriv(alglib::real_1d_array& x, vector<double>& vs, vector<double>& vds,vector<double>& vd2s)
{
	string fn = "D:/MyProjects/SplineDeriv.txt";
	ofstream of(fn, ofstream::out);
	for (int i = 0; i < x.length(); i++) {
		of << x[i] << ",";
	}
	of << std::endl;
	for (int i = 0; i < vs.size(); i++) {
		of << vs[i] << ",";
	}
	of << std::endl;
	for (int i = 0; i < vds.size(); i++) {
		of << vds[i] << ",";
	}
	of << std::endl;
	for (int i = 0; i < vd2s.size(); i++) {
		of << vd2s[i] << ",";
	}
	of << std::endl;

}

int splineIT(alglib::real_1d_array x, alglib::real_1d_array y)
{

	double v;
	spline1dinterpolant s;
	spline1dfitreport rep;
	double rho;
	ae_int_t info;

	int m = 400;
	// http://www.alglib.net/translator/man/manual.cpp.html#sub_spline1dcalc

	std::string fn = "D:\\MyProjects\\spline312.txt";
	std::ofstream of(fn, std::ofstream::out);

	for (int i = 0; i < x.length(); i++) {
		of << x[i] << ",";
	}
	of << std::endl;


	//rho = -5.0;
	rho= 0;
	spline1dfitpenalized(x, y, m, rho, info, s, rep);
	Out(of, x, s);



	// rho = +10.0;  why was this a straight line
	rho = 2;
	spline1dfitpenalized(x, y, m, rho, info, s, rep);
	Out(of, x, s);

	// output derivative
	vector<double> vs;
	vector<double> vds;
	vector<double> vd2s;

	for (int i = 0; i < x.length(); i++) {
		// auto r = alglib::spline1dcalc(c, x[i]);
		double sp, dsp, d2sp;
		spline1ddiff(s, x[i], sp, dsp, d2sp);
		vs.push_back(sp);
		vds.push_back(dsp);
		vd2s.push_back(d2sp);
	}

	OutDeriv(x, vs, vds, vd2s);

	//rho = +3.0;
	rho = +6.0;
	spline1dfitpenalized(x, y, m, rho, info, s, rep);
	Out(of, x, s);


	return 0;
}


int main()
{

	const int N = 201;
	std::vector<double> X(N), Y(N);
	for (int i = 0; i < N; i++)
	{
		X[i] = (2 * (double)i) / (N - 1) - 1;
		auto y = multiSinusoidal(X[i]);
		Y[i] = y;

	}
	alglib::real_1d_array AX, AY;
	AX.setcontent(X.size(), &(X[0]));
	AY.setcontent(Y.size(), &(Y[0]));
	splineIT(AX, AY);
}
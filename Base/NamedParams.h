#pragma once
/*
* NamedParams.h

*/

#ifndef NamedParams_H_
#define NamedParams_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include "boost/date_time/posix_time/time_serialize.hpp"
#include <boost/serialization/vector.hpp>
#include<vector>

using namespace std;
using namespace boost::posix_time;

class NamedParams {

private:
	friend class boost::serialization::access;
	
	template<class Archive> void serialize(Archive & ar,
		const unsigned int version) {
		try {
			ar & BOOST_SERIALIZATION_NVP(Key);
			ar & BOOST_SERIALIZATION_NVP(Symbol);
			ar & BOOST_SERIALIZATION_NVP(Frequency);
			ar & BOOST_SERIALIZATION_NVP(Start);
			ar & BOOST_SERIALIZATION_NVP(End);
			ar & BOOST_SERIALIZATION_NVP(Scale);
			ar & BOOST_SERIALIZATION_NVP(Intraday);
			ar & BOOST_SERIALIZATION_NVP(HoldingPeriod);
			ar & BOOST_SERIALIZATION_NVP(OpenPassive);
			ar & BOOST_SERIALIZATION_NVP(ClosePassive);
			ar & BOOST_SERIALIZATION_NVP(MinLengthA);
			ar & BOOST_SERIALIZATION_NVP(RhoA);
			ar & BOOST_SERIALIZATION_NVP(CushionA);
			ar & BOOST_SERIALIZATION_NVP(ModeA);
			ar & BOOST_SERIALIZATION_NVP(RiskFactorA);
			ar & BOOST_SERIALIZATION_NVP(ClearHistoryA);
			ar & BOOST_SERIALIZATION_NVP(MultiplicityB);
			ar & BOOST_SERIALIZATION_NVP(MultiplierB);
			ar & BOOST_SERIALIZATION_NVP(StopMultiplierB);
			ar & BOOST_SERIALIZATION_NVP(TargetMultiplierB);
			ar & BOOST_SERIALIZATION_NVP(RebalanceB);
			ar & BOOST_SERIALIZATION_NVP(ReplaceModeB);
			ar & BOOST_SERIALIZATION_NVP(MaxPositionsB);
			ar & BOOST_SERIALIZATION_NVP(HedgedSeries2B);
			ar & BOOST_SERIALIZATION_NVP(Contracts);
		}
		catch (exception &e) {

			throw e;
		}
	}

public:
	// General
	string Key;
	string Symbol;
	string Frequency;
	float Start;
	float End;
	int Scale;
	bool Intraday;
	int HoldingPeriod;
	bool OpenPassive;
	bool ClosePassive;

	int MinLengthA;
	float RhoA;
	float CushionA;
	int ModeA;
	float RiskFactorA;
	bool ClearHistoryA;

	int MultiplicityB;
	float MultiplierB;
	float StopMultiplierB;
	float TargetMultiplierB;
	int RebalanceB;
	int ReplaceModeB;
	int MaxPositionsB;
	int Contracts;
	bool HedgedSeries2B;
};

class NamedParamsVector
{

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar,
		const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(m_npVector);
	}

public:
	vector<NamedParams>  m_npVector;

};

#endif /* NamedParams_H_ */



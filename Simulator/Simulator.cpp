
#include "Simulator.h"
#include "../Trading/PriceBook.h"
#include "../Base/UtilFunc.h"
#include "../Base/MacroTrader.h"

ptime GetAsofDate()
{
	return MacroTrader::Instance()->Asofdate();
}

namespace Anon {
	int Simulator::ReadData()
	{
		try {


			string Source = MacroTrader::configMap["ConfigParams.Source"];
			string Mode = MacroTrader::configMap["ConfigParams.Mode"];
			string product = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Product"];
			string filename = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.ProductFile"];
			string ticksize = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Ticksize"];
			string exchange = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Exchange"];
			string factor = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Factor"];
			string freq = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Frequency"];

			if (Mode == "0")
			{
				MacroTrader::sMacroTrader->AddProduct(product, exchange, freq, Decimal(::atof(ticksize.c_str())));
				_externalKeyVector.push_back(product + ":" + exchange);
				if (MacroTrader::sMacroTrader->Products().size() > 0)
					MacroTrader::sMacroTrader->Products()[0]->PriceBookPtr->Updated = &PriceBookUpdated;
			}


			if (Mode == "1") {
				vector<string> products;
				vector<string> exchanges;
				vector<Decimal> ticksizes;
				products.push_back(product);
				exchanges.push_back(exchange);
				ticksizes.push_back(Decimal(::atof(ticksize.c_str())));
				_externalKeyVector.push_back(product + ":" + exchange);
				product = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.Product"];
				filename = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.File"];
				ticksize = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.Ticksize"];
				exchange = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.Exchange"];
				factor = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.Factor"];
				products.push_back(product);
				exchanges.push_back(exchange);
				ticksizes.push_back(Decimal(::atof(ticksize.c_str())));
				_externalKeyVector.push_back(product + ":" + exchange);

				MacroTrader::sMacroTrader->AddProducts(products, exchanges, "1m", ticksizes);
				if (MacroTrader::sMacroTrader->Products().size() > 0) {
					MacroTrader::sMacroTrader->Products()[0]->PriceBookPtr->Updated = &PriceBookUpdated;
					MacroTrader::sMacroTrader->Products()[1]->PriceBookPtr->Updated = &PriceBookUpdated;
				}
			}




			bool tickData = to_bool(Source);
			if (Source == "Tick")
				ProcessMultipleTick();
			else
				ProcessMultipleCandles();
		}
		catch (exception & e)
		{
			cout << "Exception in Data Reader: " << e.what() << endl;

		}
		return 0;
	}

	Simulator::Simulator()
	{
	}

	void Simulator::ProcessMultipleTick()
	{

		try {
			int counter = 0;
			string quotefile1 = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.File"];
			string tradefile1 = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.File"] + ".trd";
			string quotefile2 = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.File"];
			string tradefile2 = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.File"] + ".trd";;
			std::ifstream isq(quotefile1, ios::in | ios::binary);
			std::ifstream ist(tradefile1, ios::in | ios::binary);
			std::ifstream isq2(quotefile2, ios::in | ios::binary);
			std::ifstream ist2(tradefile2, ios::in | ios::binary);


			if (!isq.is_open()) {
				cout << "Problem opening input file: " << quotefile1 << endl;

			}

			if (!ist.is_open()) {
				cout << "Problem opening input file: " << tradefile1 << endl;

			}

			if (MacroTrader::configMap["ConfigParams.Mode"] == "1" && !isq2.is_open()) {
				cout << "Problem opening input file: " << quotefile2 << endl;


			}

			if (MacroTrader::configMap["ConfigParams.Mode"] == "1" && !ist2.is_open()) {
				cout << "Problem opening input file: " << tradefile2 << endl;
				// throw exception("Problem opening input file 2");

			}

			vector<ifstream*> streamVector;

			int64_t ticksQ;
			unsigned char sideQ;
			int64_t rateQ;
			int64_t sizeQ;


			streamVector.push_back(&isq); //0
			streamVector.push_back(&ist); // 1
			if (MacroTrader::configMap["ConfigParams.Mode"] == "1") {
				streamVector.push_back(&isq2);// 2
				streamVector.push_back(&ist2); // 3
			}

			int vectorSize = streamVector.size();
			vector<bool> quoteProcessed;
			vector<int64_t> ticksQVec(vectorSize);
			vector<unsigned char> sideQVec(vectorSize);
			vector<int64_t> rateQVec(vectorSize);
			vector<int64_t> sizeQVec(vectorSize);

			for (int i = 0; i < streamVector.size(); i++) {
				quoteProcessed.push_back(true);
			}

			//For skipping header row
			bool firstProcess = true;
			while (eof(streamVector))
			{
				counter++;
				/*if (counter == 63339)
					cout << "there";*/
				Quote quote;
				Quote trade;
				for (int i = 0; i < streamVector.size(); i++) {
					if (quoteProcessed[i] == true && !streamVector[i]->eof()) {
						streamVector[i]->read((char*)&ticksQ, sizeof(ticksQ));
						streamVector[i]->read((char*)&sideQ, sizeof(sideQ));
						streamVector[i]->read((char*)&rateQ, sizeof(rateQ));
						streamVector[i]->read((char*)&sizeQ, sizeof(sizeQ));
						/*if (firstProcess) {
						ticksQVec.push_back(ticksQ);
						sideQVec.push_back(sideQ);
						rateQVec.push_back(rateQ);
						sizeQVec.push_back(sideQ);
						}
						else {*/
						ticksQVec[i] = ticksQ;
						sideQVec[i] = (sideQ);
						rateQVec[i] = (rateQ);
						sizeQVec[i] = (sizeQ);
						//std::cout << "ticks:" << ticksQ << ", sideQ:" << sideQ << " ,rateQ:" << rateQ << " ,size:" << sizeQ << endl;
						//	}
						quoteProcessed[i] = false;
					}
				}
				firstProcess = false;

				//  identify earliest timestamp  index
				std::vector<int64_t>::iterator result = std::min_element(std::begin(ticksQVec), std::end(ticksQVec));
				int t = std::distance(std::begin(ticksQVec), result);
				//std::cout << "min element at: " << std::distance(std::begin(ticksQVec), result);

				//if (streamVector[t]->eof())
				//{

				//	ticksQVec[t] = std::numeric_limits<int64_t>::max();// std::numeric_limits<int>::max();
				//}
				//Logging - Testing 
				ptime uxTime;

				time_t  unixSecs = getUnixTime(ticksQVec[t]);
				uxTime = from_time_t(unixSecs);
				//cout << "counter: " << counter << endl;
				//boost::posix_time::ptime pt = boost::posix_time::from_time_t(uxTime);
				//cout << "Time Logged " << boost::posix_time::to_simple_string(uxTime) << endl;
//				multiTickFile << to_simple_string(uxTime) << endl;
				//Testing end
				quoteProcessed[t] = true;


				int factor;
				string productString;

				// if t < 2 then get factor and Product for first instrument
				if (t < 2) {
					factor = stoi(MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Factor"]);
					productString = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Product"];
					productString.erase(std::remove(productString.begin(), productString.end(), '/'), productString.end());
				}
				else if (MacroTrader::configMap["ConfigParams.Mode"] == "1") {
					factor = stoi(MacroTrader::configMap["ConfigParams.Instruments.Instrument2.Factor"]);
					productString = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.Product"];
					productString.erase(std::remove(productString.begin(), productString.end(), '/'), productString.end());
				}

				if (t % 2 == 0) //even indices hence quote
				{
					MakeQuote(ticksQVec[t], sideQVec[t], Decimal(rateQVec[t]), sizeQVec[t], factor, quote);
					//RunApp(quote, productString, true);
					if (MacroTrader::sMacroTrader->Products().size() > 0)
					{
						string symbol = MacroTrader::sMacroTrader->Products()[t / 2]->ExternalSymbol;
						RaiseQuote(MacroTrader::sMacroTrader->Products()[t / 2], quote);
						Candle * c;
						if ((c = _candleMaker.HandleQuote(FromCDull(ticksQVec[t]), quote, symbol)) != NULL)
							MacroTrader::sMacroTrader->HandleCandle(MacroTrader::sMacroTrader->Products()[t / 2]->ExternalKey, *c);
					}
				}
				else {

					MakeTrade(ticksQVec[t], sideQVec[t], Decimal(rateQVec[t]), sizeQVec[t], factor, trade);
					if (MacroTrader::sMacroTrader->Products().size() > 0)
					{
						string symbol = MacroTrader::sMacroTrader->Products()[t / 2]->ExternalSymbol;
						RaiseTrade(MacroTrader::sMacroTrader->Products()[t / 2], trade);
						Candle * c;
						if ((c = _candleMaker.HandleTrade(FromCDull(ticksQVec[t]), trade, symbol)) != NULL)
							MacroTrader::sMacroTrader->HandleCandle(MacroTrader::sMacroTrader->Products()[t / 2]->ExternalKey, *c);
					}
				}
				if (streamVector[t]->eof())
				{

					ticksQVec[t] = std::numeric_limits<int64_t>::max();// std::numeric_limits<int>::max();
				}

			}

			for (int i = 0; i < streamVector.size(); i++)
				streamVector[i]->close();
			string candleDir = GetOutDir();
			_candleMaker.WriteCandlesToFile(candleDir + "CandleMakerOut.csv", false);
		}
		catch (exception & e) {

			cout << "Simulator Exception -ProcessMultipleTick" << endl;
			throw e;
		}
		//multiTickFile.close();
	}


	void Simulator::RaiseQuote(Product * p, Quote q, bool tickData)
	{
		PriceInfo pi((Decimal)q.rate, q.size, 1);
		ptime uxTime;
		if (tickData) {
			time_t unixSecs = getUnixTime(q.date);
			uxTime = from_time_t(unixSecs);
		}
		else
			uxTime = from_time_t(q.date);

		p->PriceBookPtr->MarketStatePtr->GVTimestamp = uxTime;

		p->MarketStatePtr->ProductPtr = p;

		p->PriceBookPtr->Add(pi, q.side == 0 ? true : false);

	}

	void Simulator::RaiseTrade(Product * p, Quote q, bool tickData)
	{
		PriceInfo pi(q.rate, q.size, 1);

		ptime uxTime;
		if (tickData) {
			time_t  unixSecs = getUnixTime(q.date);
			uxTime = from_time_t(unixSecs);
		}
		else
			uxTime = from_time_t(q.date);



		p->MarketStatePtr->GVTimestamp = uxTime;
		MarketDepthChangeEventArgs *e = new MarketDepthChangeEventArgs();

		e->State = *(p->MarketStatePtr);
		MarketStateDelta *d = new MarketStateDelta();
		d->TradeExecuted = true;
		e->Delta = *d;

		e->State.LastSignedQuantity = q.side == 0x1 ? q.size : -q.size;
		e->State.LastQuantity = q.size;
		e->State.LastPrice = q.rate;
		// called directly for trade  
		PriceBookUpdated(e->State.ProductPtr->ExternalKey, e);

	}

	void Simulator::ProcessMultipleCandles()
	{
		auto candleDuration = time_duration(0, 0, 0);
		try
		{
			string freq = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Frequency"];

			if (freq == "1m")
				candleDuration = time_duration(0, 1, 0);
			else if (freq == "5m")
				candleDuration = time_duration(0, 5, 0);
			else if (freq == "5m")
				candleDuration = time_duration(0, 5, 0);
			else if (freq == "15m")
				candleDuration = time_duration(0, 15, 0);
			else if (freq == "1H")
				candleDuration = time_duration(1, 0, 0);
			else if (freq == "4H")
				candleDuration = time_duration(4, 0, 0);
			else if (freq == "1D")
				candleDuration = time_duration(24, 0, 0);
			else
				cout << "invalid duration in config file?" << endl;
		}
		catch (exception ex)
		{
			cout << "frequency missing: older version of config file?" << endl;
		}


		try
		{
			Quote quote;
			Quote trade;
			string candlefile1 = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.File"];
			string candlefile2 = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.File"];

			std::ifstream isq(candlefile1, ios::in);
			std::ifstream isq2(candlefile2, ios::in);

			// check if files were actually opened?
			if (!isq.is_open()) {
				cout << "Problem opening input file: " << candlefile1 << endl;
				// doesnt work on gcc  with CENTOS7.2
				// throw exception("Problem opening input file 1");

			}
			else
			{
				cout << "opened candlefile1" << candlefile1 << endl;
			}

			if (MacroTrader::configMap["ConfigParams.Mode"] == "1" && !isq2.is_open()) {
				cout << "Problem opening input file: " << candlefile2 << endl;
				// throw exception("Problem opening input file 2" );

			}
			else
			{
				cout << "opened candlefile2 " << candlefile2 << endl;
			}

			vector<ifstream*> streamVector;

			streamVector.push_back(&isq); //0
			if (MacroTrader::configMap["ConfigParams.Mode"] == "1") {
				streamVector.push_back(&isq2); // 1
			}

			int vectorSize = streamVector.size();
			vector<bool> quoteProcessed;
			vector<time_t> timeStamp(vectorSize);
			vector<Decimal> LSTB(vectorSize);
			vector<int64_t> LSBS(vectorSize);
			vector<Decimal> LSTA(vectorSize);
			vector<int64_t> LSAS(vectorSize);
			vector<string> interval(vectorSize);
			vector<Decimal> LBID(vectorSize);
			vector<Decimal> HASK(vectorSize);

			vector<Decimal> INTF(vectorSize);  // interval first trade price
			//(intv -ivam)
			vector<int64_t> INTV(vectorSize);  // interval volume i.e. sum(shares))
			vector<int64_t> IVAM(vectorSize);  // interval volume over mid quote(bid + ask) / 2
			vector<int64_t> IVAMpaid(vectorSize);
			int fieldSize = 0;

			for (int i = 0; i < streamVector.size(); i++) {
				quoteProcessed.push_back(true);
				string line;
				//Skip first header line
				getline(*streamVector[i], line);
			}
			while (eof(streamVector))
			{
				//  NOTE: lots of conversion so please put this in a try catch  block with enough checks
				for (int i = 0; i < streamVector.size(); i++) {
					if (quoteProcessed[i] == true && !streamVector[i]->eof()) {
						string line;
						getline(*streamVector[i], line);
						if (teststrategy)
						{
							cout << line << endl;
						}
						if (line == "")
							continue;
						vector<string> fields;
						fields = split(line, ',');
						if (fields.size() == 24)
							fieldSize = 24;
						else if (fields.size() == 23)  // last field is blank: TODO correct the logic, failing with resampled files
						{
							fieldSize = 23; //IVAM is field[23] -last field
						}
						else if (fields.size() == 2)
							fieldSize = 2;
						else
						{
							cout << "Incorrect number of fields in data file:" << fields.size() << endl;
							throw new exception();
						}

						string ts = "";
						if (fieldSize == 24 || fieldSize == 23)
							ts = fields[0];
						else
							ts = fields[0] + " 00:00:00";

						ValidateTimestamp(ts, fieldSize);

						if (fieldSize == 24 || fieldSize == 23) {

							LSTB[i] = (fields[10].length() > 0) ? Decimal(fields[10]) : Decimal(0);
							LSBS[i] = (fields[11].length() > 0) ? atoi(fields[11].c_str()) : 0;
							LSTA[i] = (fields[12].length() > 0) ? Decimal(fields[12]) : Decimal(0);
							LSAS[i] = (fields[13].length() > 0) ? atoi(fields[13].c_str()) : 0;
							LBID[i] = (fields[15].length() > 0) ? Decimal(fields[15]) : Decimal(0);
							HASK[i] = (fields[16].length() > 0) ? Decimal(fields[16]) : Decimal(0);
							interval[i] = fields[1];

                            if(LSTB[i] > LSTA[i])
                            {
                                cout << "crossed quotes" << endl;
                            }
						}
						else
						{
							LSTB[i] = (fields[1].length() > 0) ? Decimal(fields[1]) : Decimal(0);
							LSBS[i] = 0;
							LSTA[i] = (fields[1].length() > 0) ? Decimal(fields[1]) : Decimal(0);
							LSAS[i] = 0;

						}

						try {
							if (ts.length() == 10) // format  2014-03-07 for  1 day resamples
							{
								ptime periodStart = time_from_string(ts + " 00:00:00");
								time_t myT = boost::posix_time::to_time_t(periodStart);
								timeStamp[i] = myT;
							}
							else if (ts.length() == 19)// format  2014-03-07 00:00:00  for others
							{
								ptime periodStart = time_from_string(ts);
								time_t myT = boost::posix_time::to_time_t(periodStart);
								timeStamp[i] = myT;
							}
							else
							{
								cout << "invalid date / time format" << endl;
							}

						}
						catch (exception &e)
						{
							cout << "not able to convert to time. using 1 Day resamples?" << endl;
						}

						if (fieldSize == 24) {
							// NOTE: maybe do similar ternaries if necessary
							INTF[i] = (fields[5].length() > 0) ? Decimal(stof(fields[5])) : Decimal(0);
							INTV[i] = (fields[8].length() > 0) ? atoi(fields[8].c_str()) : 0;
							IVAM[i] = (fields[23].length() > 0) ? atoi(fields[23].c_str()) : 0;
						}
						else if (fieldSize == 23) {
							INTF[i] = (fields[5].length() > 0) ? Decimal(stof(fields[5])) : Decimal(0);
							INTV[i] = (fields[8].length() > 0) ? atoi(fields[8].c_str()) : 0;
							IVAM[i] = 0;
						}
						else {
							INTF[i] = Decimal(0);
							INTV[i] = 0;
							IVAM[i] = 0;

						}


						IVAMpaid[i] = INTV[i] - IVAM[i];

						quoteProcessed[i] = false;
					}
				}

				//  identify earliest timestamp  index
				std::vector<int64_t>::iterator result = std::min_element(std::begin(timeStamp), std::end(timeStamp));
				int t = std::distance(std::begin(timeStamp), result);
				//std::cout << "min element at: " << std::distance(std::begin(timeStamp), result);
				quoteProcessed[t] = true;


				//Logging - Testing 
				boost::posix_time::ptime pt = boost::posix_time::from_time_t(timeStamp[t]);
				//Testing end
				if (streamVector[t]->eof())
				{

					timeStamp[t] = std::numeric_limits<int64_t>::max();// std::numeric_limits<int>::max();
				}

				if (!streamVector[t]->eof())
				{
					int factor;
					string productString;

					if (t < 1) {
						factor = stoi(MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Factor"]);
						productString = MacroTrader::configMap["ConfigParams.Instruments.Instrument1.Product"];
						productString.erase(std::remove(productString.begin(), productString.end(), '/'), productString.end());
					}
					else {
						factor = stoi(MacroTrader::configMap["ConfigParams.Instruments.Instrument2.Factor"]);
						productString = MacroTrader::configMap["ConfigParams.Instruments.Instrument2.Product"];
						productString.erase(std::remove(productString.begin(), productString.end(), '/'), productString.end());
					}

					if (false) 
					{

						MakeQuote(timeStamp[t], 0x1, LSTB[t], LSBS[t], factor, quote);
						if (MacroTrader::sMacroTrader->Products().size() > 0)
						{
							RaiseQuote(MacroTrader::sMacroTrader->Products()[t], quote, false);
						}


						MakeQuote(timeStamp[t], 0x0, LSTA[t], LSAS[t], factor, quote);
						if (MacroTrader::sMacroTrader->Products().size() > 0)
						{
							RaiseQuote(MacroTrader::sMacroTrader->Products()[t], quote, false);
						}

						if (INTF[t] != Decimal(0)) {
							MakeTrade(timeStamp[t], 0x0, INTF[t], IVAMpaid[t], factor, trade);
							if (MacroTrader::sMacroTrader->Products().size() > 0)
								RaiseTrade(MacroTrader::sMacroTrader->Products()[t], trade, false);

							MakeTrade(timeStamp[t], 0x1, INTF[t], IVAM[t], factor, trade);
							if (MacroTrader::sMacroTrader->Products().size() > 0)
								RaiseTrade(MacroTrader::sMacroTrader->Products()[t], trade, false);
						}
					}

					//skip raising candle if LSTA or LSTB are missing
					if (LSTA[t] != Decimal(0) || LSTB[t] != Decimal(0)) {
						//  NOTE:  check time math
						auto c = new Candle();
						c->interval = candleDuration;
						c->timeStamp = boost::posix_time::from_time_t(timeStamp[t]);
						c->INTF = INTF[t];
						c->INTV = INTV[t];
						c->IVAM = IVAM[t];
						c->LSAS = LSAS[t];
						c->LSBS = LSBS[t];
						c->LSTA = LSTA[t];
						c->LSTB = LSTB[t];
						c->LBID = LBID[t];
						c->HASK = HASK[t];
						if (MacroTrader::sMacroTrader->Products().size() > 0)
							MacroTrader::sMacroTrader->HandleCandle(MacroTrader::sMacroTrader->Products()[t]->ExternalKey, *c);
					}
				}

			}
			for (int i = 0; i < streamVector.size(); i++)
				streamVector[i]->close();

			MacroTrader::sMacroTrader->PrintPositions();
		}
		catch (exception & e) {

			cout << "Simulator Exception -ProcessMultipleCandle" << endl;
			throw e;
		}
	}

	void Simulator::MakeQuote(int64_t ticks, unsigned char side, Decimal rate, int64_t size, int factor, Quote& q)
	{
		string s;
		if (side == (unsigned char)0x0)
		{
			s = "BUY";
		}
		else if (side == (unsigned char)0x1)
		{
			s = "SELL";
		}
		else
		{
			cout << "what the hell is this?" << endl;
		}

		Decimal dRate = rate;
		dRate = dRate / factor;

		q.side = side;
		q.rate = dRate;
		q.size = size;
		q.date = ticks;

		// cout << "Quote - side, rate, size ticks: " << s << "," << q.rate << "," << q.size << "," << q.date << endl;
	}
	void Simulator::MakeTrade(int64_t ticks, unsigned char side, Decimal rate, int64_t size, int factor, Quote& trade)
	{
		string s;
		if (side == (unsigned char)0x1)
		{
			s = "PAID";
		}
		else if (side == (unsigned char)0x2)
		{
			s = "GIVEN";
		}
		else
		{
			cout << "what the hell is this?" << endl;
		}

		Decimal dRate = rate;
		dRate = dRate / factor;

		trade.side = side;
		trade.rate = dRate;
		trade.size = size;
		trade.date = ticks;
	}

	void Simulator::PriceBookUpdated(string, MarketDepthChangeEventArgs* e)
	{
		//cout << "Pb_Updated: " << to_simple_string(e->State.GVTimestamp) << endl;

		MacroTrader::sMacroTrader->HandleEvent(e->State.ProductPtr->ExternalKey, *e);
	}

	void Simulator::ValidateTimestamp(string ts, int numFields)
	{

		if (numFields == 2) {
		}
		else {
			//make sure its yyyy-mm-dd format
			if (ts[4] != '-' &&  ts[7] != '-') {
				cout << "Wrong date format. Expected YYYY-MM-DD" << endl;
				throw new exception();
			}
		}
	}

}

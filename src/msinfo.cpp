#include <iostream>
#include <cstdlib>
#include <sstream>
#include <vector>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include "structures/antennainfo.h"
#include "structures/msmetadata.h"

#include "strategy/algorithms/thresholdtools.h"

using namespace std;

std::string BoolToStr(bool val)
{
	if(val)
		return "true";
	else
		return "false";
}

int main(int argc, char *argv[])
{
	if(argc < 2) {
		cout << "This program will provide you general information about a measurement set.\nUsage:\n\t" << argv[0] << " [options] <measurement set>\n"
		     << endl;
		exit(-1);
	}

#ifdef HAS_LOFARSTMAN
	register_lofarstman();
#endif // HAS_LOFARSTMAN

	//bool saveFlagLength = false;
	//string flagLengthFile;
	bool antennaeList = false;
	int pindex = 1;
	while(pindex < argc && argv[pindex][0] == '-') {
		string parameter = argv[pindex]+1;
		if(parameter == "antennae")
		{
			antennaeList = true;
		} else
		{
			cerr << "Bad parameter: -" << parameter << endl;
			exit(-1);
		}
		++pindex;
	}
	
	string measurementFile = argv[pindex];
	if(antennaeList)
	{
		MSMetaData set(measurementFile);
		const size_t antennaCount = set.AntennaCount();
		for(size_t i=0;i<antennaCount;++i)
		{
			cout.width(16);
			cout.precision(16);
			const AntennaInfo antenna = set.GetAntennaInfo(i);
			cout << antenna.id << '\t' << antenna.position.Latitude()*180.0/M_PI << '\t' << antenna.position.Longitude()*180.0/M_PI << '\t' << antenna.position.Altitude() << '\n';
		}
	}
	else {
		cout << "Opening measurementset " << measurementFile << endl; 
		MSMetaData set(measurementFile);
		size_t antennaCount = set.AntennaCount();
		cout
			<< "Telescope name: " << set.TelescopeName() << '\n'
			<< "Number of antennea: " << antennaCount << '\n'
			<< "Number of time scans: " << set.TimestepCount() << '\n'
			<< "Number of channels/band: " << set.FrequencyCount(0) << '\n'
			<< "Number of fields: " << set.FieldCount() << '\n'
			<< "Number of bands: " << set.BandCount() << '\n';
		{
			casacore::MeasurementSet table(set.Path());
			cout << "Has DATA column: " << BoolToStr(table.tableDesc().isColumn("DATA")) << "\n";
			cout << "Has CORRECTED_DATA column: " << BoolToStr(table.tableDesc().isColumn("CORRECTED_DATA")) << "\n";
			cout << "Has MODEL_DATA column: " << BoolToStr(table.tableDesc().isColumn("MODEL_DATA")) << "\n";
		}
		std::vector<long double> baselines;
		for(size_t i=0;i<antennaCount;++i)
		{
			cout << "==Antenna " << i << "==\n";
			AntennaInfo info = set.GetAntennaInfo(i);
			cout <<
				"Diameter: " << info.diameter << "\n"
				"Name: " << info.name << "\n"
				"Position: " << info.position.ToString() << "\n"
				"Mount: " << info.mount << "\n"
				"Station: " << info.station << "\n"
				"Providing baselines: ";
			for(size_t j=0;j<antennaCount;++j) {
				AntennaInfo info2 = set.GetAntennaInfo(j);
				Baseline b(info.position, info2.position);
				long double dist = b.Distance();
				long double angle = b.Angle() * 180.0 / M_PIn;
				cout << dist << "m/" << angle << "` ";
				baselines.push_back(dist);
			}
			cout << "\n" << endl;
		}
		sort(baselines.begin(), baselines.end());
		cout << "All provided baselines: ";
		unsigned i=0;
		while(i<baselines.size()-1)
		{
			if(baselines[i+1]-baselines[i] < 1.0)
				baselines.erase(baselines.begin() + i);
			else
				++i;
		}
		for(long double v : baselines)
			cout << (v) << ' ';
		cout << '\n';

		for(unsigned i=0;i!=set.BandCount();++i) {
			cout << "== Spectral band index " << i << " ==" << endl;
			BandInfo bandInfo = set.GetBandInfo(i);
			cout << "Channel count: " << bandInfo.channels.size() << std::endl;
			cout << "Channels: ";
			for(unsigned j=0;j<bandInfo.channels.size();++j) {
				if(j > 0) cout << ", ";
				cout << round(bandInfo.channels[j].frequencyHz/1000000) << "MHz";
			}
			cout << endl;
		}
		
		for(unsigned i=0;i<set.FieldCount();++i) {
			FieldInfo fieldInfo = set.GetFieldInfo(i);
			cout << "Field " << i << ":\n\tdelay direction=" << fieldInfo.delayDirectionDec << " dec, " << fieldInfo.delayDirectionRA << "ra.\n\tdelay direction (in degrees)=" << (fieldInfo.delayDirectionDec/M_PIn*180.0L) << " dec," << (fieldInfo.delayDirectionRA/M_PIn*180.0L) << " ra." << endl;
		}

		return EXIT_SUCCESS;
	}
}

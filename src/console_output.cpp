/*
 * Copyright (c) 2012 Adrien Guinet <adrien@guinet.me>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 	Redistributions of source code must retain the above copyright notice, this
 * 	list of conditions and the following disclaimer.
 * 
 * 	Redistributions in binary form must reproduce the above copyright notice,
 * 	this list of conditions and the following disclaimer in the documentation
 * 	and/or other materials provided with the distribution.
 * 
 * 	Neither the name of Adrien Guinet nor the names of its
 * 	contributors may be used to endorse or promote products derived from this
 * 	software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <usbtop/buses.h>
#include <usbtop/usb_bus.h>
#include <usbtop/usb_device.h>
#include <usbtop/usb_stats.h>
#include <usbtop/console_output.h>
#include <usbtop/should_stop.h>
#include <usbtop/tools.h>

#include <iostream>
#include <iomanip>
#include <sstream>

#include <unistd.h>

double usbtop::ConsoleOutput::_t0 = 0;

void usbtop::ConsoleOutput::main()
{
	std::cout.precision(2);
	std::cout.setf(std::ios::fixed);
	if(csv){
		// Wait 1 second to be sure that all of the USB devices has been 
		// recognized before initialization of the CSV file.
		usleep(1000*1000);
		initialize_csv();
		while (true) {
			usleep(250*1000);
			if (usbtop::ShouldStop::value()) {
				break;
			}
			print_csv();
		}
	}
	else{
		while (true) {
			usleep(250*1000);
			if (usbtop::ShouldStop::value()) {
				break;
			}
			clear_screen();
			print_stats();
		}
	}
}

void usbtop::ConsoleOutput::initialize_csv()
{
	std::cout << "Time" << ",";
	UsbBuses::list_pop([](UsbBus const* bus)
		{ initialize_csv_bus(*bus); });
	std::cout << std::endl;
}

void usbtop::ConsoleOutput::initialize_csv_bus(UsbBus const& bus)
{
	UsbBus::list_devices_t const& devs = bus.devices();
	UsbBus::list_devices_t::const_iterator it;
	for (it = devs.begin(); it != devs.end(); it++) {
	    std::cout << "Bus_" << bus.id() << "_Device_" << it->first << "_BW_to" << ",";
	    std::cout << "Bus_" << bus.id() << "_Device_" << it->first << "_BW_from" << ",";
	    std::cout << "Bus_" << bus.id() << "_Device_" << it->first << "_Nb_to" << ",";
	    std::cout << "Bus_" << bus.id() << "_Device_" << it->first << "_Nb_from" << ",";
	}
	std::cout << "Bus_" << bus.id() << "_Total_" << "BW_to" << ",";
    std::cout << "Bus_" << bus.id() << "_Total_" << "BW_from" << ",";
    std::cout << "Bus_" << bus.id() << "_Total_" << "Nb_to" << ",";
    std::cout << "Bus_" << bus.id() << "_Total_" << "Nb_from" << ",";
	
	_t0 = usbtop::tools::get_current_timestamp();
}

void usbtop::ConsoleOutput::print_csv()
{
	// Write time
	double t = usbtop::tools::get_current_timestamp();
	std::cout << t - _t0 << ",";
	// Write stats of buses
	UsbBuses::list_pop([](UsbBus const* bus)
		{ print_csv_bus(*bus); });
	std::cout << std::endl;
}

void usbtop::ConsoleOutput::print_csv_bus(UsbBus const& bus)
{
	UsbBus::list_devices_t const& devs = bus.devices();
	UsbBus::list_devices_t::const_iterator it;
    double cumulative_bw_to(0);
    double cumulative_bw_from(0);
	long cumulative_npackets_to(0);
	long cumulative_npackets_from(0);

	for (it = devs.begin(); it != devs.end(); it++) {
		UsbDevice const& dev(*it->second);
		UsbStats const& stats(dev.stats());
        double bw_to = stats.stats_to_device().bw_instant();
        double bw_from = stats.stats_from_device().bw_instant();
		long npackets_to = stats.stats_to_device().sample_rate();
		long npackets_from = stats.stats_from_device().sample_rate();

        cumulative_bw_to += bw_to;
        cumulative_bw_from += bw_from;
		cumulative_npackets_to += npackets_to;
		cumulative_npackets_from += npackets_from;

        std::cout << bw_to << "," << bw_from << "," << npackets_to << "," << npackets_from << ",";
	}
	std::cout << cumulative_bw_to << "," << cumulative_bw_from << "," << cumulative_npackets_to << "," << cumulative_npackets_from << ",";
}

void usbtop::ConsoleOutput::clear_screen()
{
	std::cout << "\033[2J\033[1;1H";
}

void usbtop::ConsoleOutput::print_stats()
{
	UsbBuses::list_pop([](UsbBus const* bus)
		{ print_stats_bus(*bus); });
}

std::string usbtop::ConsoleOutput::bytes_to_string(double bytes)
{
    std::stringstream returnVal;
    returnVal.precision(2);
    returnVal << std::fixed;
    if (bytes < 500.0)
    {
        returnVal << bytes << " b/s";
    }
    else if (bytes < 500000.0)
    {
        returnVal << bytes/1000.0 << " kb/s";
    }
    else
    {
        returnVal << bytes/1000000.0 << " Mb/s";
    }

    return returnVal.str();
}

void usbtop::ConsoleOutput::print_stats_bus(UsbBus const& bus)
{
	std::cout << "Bus ID " << bus.id() << " (" << bus.desc() << ")"; 
    std::cout << "\tBW to device\tBW from device\tNb to device\tNb from device" << std::endl;
	UsbBus::list_devices_t const& devs = bus.devices();
	UsbBus::list_devices_t::const_iterator it;
    double cumulative_bw_to(0);
    double cumulative_bw_from(0);
	long cumulative_npackets_to(0);
	long cumulative_npackets_from(0);

	for (it = devs.begin(); it != devs.end(); it++) {
		UsbDevice const& dev(*it->second);
		UsbStats const& stats(dev.stats());
        std::cout << "  Device ID " << std::setw(3) << it->first << ":\t";
        double bw_to = stats.stats_to_device().bw_instant();
        double bw_from = stats.stats_from_device().bw_instant();
		long npackets_to = stats.stats_to_device().sample_rate();
		long npackets_from = stats.stats_from_device().sample_rate();

        cumulative_bw_to += bw_to;
        cumulative_bw_from += bw_from;
		cumulative_npackets_to += npackets_to;
		cumulative_npackets_from += npackets_from;

        std::cout << "\t" << bytes_to_string(bw_to) << "\t" << bytes_to_string(bw_from) << "\t";
		std::cout << npackets_to << " pkts/s\t" << npackets_from << " pkts/s" << std::endl;
	}
    std::cout << "  ---------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "  Total:\t\t\t" << bytes_to_string(cumulative_bw_to) << "\t" << bytes_to_string(cumulative_bw_from) << "\t";
	std::cout << cumulative_npackets_to << " pkts/s\t" << cumulative_npackets_from << " pkts/s" << std::endl;
    std::cout << std::endl;
}

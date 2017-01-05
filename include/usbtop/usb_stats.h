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

#ifndef USBTOP_USB_STATS_H
#define USBTOP_USB_STATS_H

#include <cassert>
#include <cstddef>

#include <boost/circular_buffer.hpp>
#include <boost/thread/shared_mutex.hpp>

#define INITIAL_BUFFER_SIZE 128
#define WINDOW_SECOND 		1.0		// Time in seconds for this type of window
#define WINDOW_MINUTE		60.0	// Time in seconds for this type of window

namespace usbtop {

class Stats: boost::noncopyable
{
	typedef std::pair<double, size_t> sample_t;
public:
	Stats();
	Stats(Stats&& o):
		_total_samples(o._total_samples),
		_total_tsize(o._total_tsize),
		_current_tsize(o._current_tsize),
		_tN(o._tN),		
		_inst_data(std::move(o._inst_data))
	{ }

public:
	static void init(double window_time);
	void push(double timestamp, size_t spacket);
	static double get_window_time();

public:
	double bw_instant() const;
	double bw_mean() const;
	long sample_rate() const;

private:
	// Global stats
	size_t _total_samples;
	size_t _total_tsize;
	size_t _current_tsize;
	double _tN;

	// "Instantaneous" stats
	std::vector<sample_t> _inst_data;
	mutable boost::shared_mutex _access;

	// Timestamp when the application is launched. Used as t0
	static double _t0;

	// Last "instantaneous" stats
	double _last_inst_bw;

	// Last "instantaneous" number of packets per seconds
	size_t _last_inst_sample_rate;

	// Time window for statistics in seconds
	static double _stats_window;
};

class UsbStats: boost::noncopyable
{
public:
	// The order of this enum is important !! (cf. UsbBus::push)
	typedef enum {
		from_device = 0,
		to_device
	} direction_type;

public:
	UsbStats() { }

	UsbStats(UsbStats&& o):
		_from_device(std::move(o._from_device)),
		_to_device(std::move(o._to_device))
	{ }

public:
	inline void push(const double ts, const size_t size, const direction_type dir)
	{
		assert((dir == direction_type::from_device) || (dir == direction_type::to_device));
		if (dir == direction_type::from_device) {
			_from_device.push(ts, size);
		}
		else {
			_to_device.push(ts, size);
		}
	}

public:
	Stats const& stats_to_device() const { return _to_device; }
	Stats const& stats_from_device() const { return _from_device; }

private:
	Stats _from_device;
	Stats _to_device;
};

}

#endif

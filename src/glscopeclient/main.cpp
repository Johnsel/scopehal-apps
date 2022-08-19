/***********************************************************************************************************************
*                                                                                                                      *
* glscopeclient                                                                                                        *
*                                                                                                                      *
* Copyright (c) 2012-2022 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Andrew D. Zonenberg
	@brief Program entry point
 */

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#endif

#include "glscopeclient.h"
#include "../scopeprotocols/scopeprotocols.h"
#include "../scopeexports/scopeexports.h"
#include <libgen.h>
#include <omp.h>
#include <chrono>
#include <iostream>
#include <thread>
#include "pthread_compat.h"

#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#include "PreferenceManager.h"
using namespace std;

//for color selection
int g_numDecodes = 0;

//Feature disable flags for debug
bool g_noglint64 = false;

ScopeApp* g_app = NULL;

//Default locale for printing numbers
char* g_defaultNumLocale;

void help();

void help()
{
	LogDebug("help 1\n");

	fprintf(stderr,
			"glscopeclient [general options] [logger options] [dev options] [filename|scope]\n"
			"\n"
			"  [general options]:\n"
			"    --help      : this message...\n"
			"    --nodata    : when loading a .scopesession from the command line, only load instrument/UI settings\n"
			"                  (default is to load waveform data too)\n"
			"    --reconnect : when loading a .scopesession from the command line, reconnect to the instrument\n"
			"                  (default is to do offline analysis)\n"
			"    --retrigger : when loading a .scopesession from the command line, start triggering immediately\n"
			"                  (default is to be paused)\n"
			"    --version   : print version number. (not yet implemented)\n"
			"\n"
			"  [logger options]:\n"
			"    levels: ERROR, WARNING, NOTICE, VERBOSE, DEBUG\n"
			"    --quiet|-q                    : reduce logging level by one step\n"
			"    --verbose                     : set logging level to VERBOSE\n"
			"    --debug                       : set logging level to DEBUG\n"
			"    --trace <classname>|          : name of class with tracing messages. (Only relevant when logging level is DEBUG.)\n"
			"            <classname::function>\n"
			"    --logfile|-l <filename>       : output log messages to file\n"
			"    --logfile-lines|-L <filename> : output log messages to file, with line buffering\n"
			"    --stdout-only                 : writes errors/warnings to stdout instead of stderr\n"
			"\n"
			"  [dev options]:\n"
			"    --noavx2                      : Do not use AVX2, even if supported on the current system\n"
			"    --noavx512f                   : Do not use AVX512F, even if supported on the current system\n"
			"    --noglint64                   : Act as if GL_ARB_gpu_shader_int64 is not present, even if it is\n"
			"    --noopencl                    : Do not use OpenCL, even if supported on the current system\n"
			"\n"
			"  [filename|scope]:\n"
			"    filename : path to a .scopesession to load on startup\n"
			"               May also be a CSV or other supported file to be imported.\n"
			"               Some file formats (like CSV) allow multiple files to be specified, separated by spaces\n"
			"    scope    : <scope name>:<scope driver>:<transport protocol>[:<transport arguments]\n"
			"\n"
			"  Examples:\n"
			"    glscopeclient --debug myscope:siglent:lxi:192.166.1.123\n"
			"    glscopeclient --debug --trace SCPITMCTransport myscope:siglent:usbtmc:/dev/usbtmc0\n"
			"    glscopeclient --reconnect --retrigger foobar.scopesession\n"
			"\n"
	);
}

#ifndef _WIN32
void Relaunch(int argc, char* argv[]);
#endif

int main(int argc, char* argv[])
{
	LogDebug("main1\n");
	//Global settings
	Severity console_verbosity = Severity::NOTICE;

	//Parse command-line arguments
	vector<string> scopes;
	vector<string> filesToLoad;
	bool reconnect = false;
	bool nodata = false;
	bool retrigger = false;
	bool noavx2 = false;
	bool noavx512f = false;
	for(int i=1; i<argc; i++)
	{
		LogDebug("forloop 1\n");

		string s(argv[i]);

		//Let the logger eat its args first
		if(ParseLoggerArguments(i, argc, argv, console_verbosity))
			continue;

			LogDebug("forloop 2\n");

		if(s == "--help")
		{
			LogDebug("help 1\n");

			help();
			return 0;
		}
		else if(s == "--version")
		{
			//not implemented
			//ShowVersion();
			return 0;
		}
		else if(s == "--reconnect")
			reconnect = true;
		else if(s == "--nodata")
			nodata = true;
		else if(s == "--retrigger")
			retrigger = true;
		else if(s == "--noglint64")
			g_noglint64 = true;
		else if(s == "--noopencl")
			g_disableOpenCL = true;
		else if(s == "--noavx2")
			noavx2 = true;
		else if(s == "--noavx512f")
			noavx512f = true;
		else if(s[0] == '-')
		{
			fprintf(stderr, "Unrecognized command-line argument \"%s\", use --help\n", s.c_str());
			return 1;
		}

		//Not a flag. Either a connection string or a file name.
		else
		{
			//If there's a colon after the first few characters, it's a connection string
			//(Windows may have drive letter colons early on)
			auto colon = s.rfind(":");
			if( (colon != string::npos) && (colon > 1) )
				scopes.push_back(s);

			//Otherwise assume it's a file
			else
				filesToLoad.push_back(s);
		}
	}

	//Set up logging
	g_log_sinks.emplace(g_log_sinks.begin(), new ColoredSTDLogSink(console_verbosity));

	//Complain if the OpenMP wait policy isn't set right
	const char* policy = getenv("OMP_WAIT_POLICY");
	#ifndef _WIN32
		bool need_relaunch = false;
	#endif
	if((policy == NULL) || (strcmp(policy, "PASSIVE") != 0) )
	{
		#ifdef _WIN32
			LogWarning("glscopeclient works best with the OMP_WAIT_POLICY environment variable set to PASSIVE\n");
		#else
			LogDebug("OMP_WAIT_POLICY not set to PASSIVE\n");
			setenv("OMP_WAIT_POLICY", "PASSIVE", true);

			need_relaunch = true;
		#endif
	}

	//Complain if asan options are not set right
	#ifdef __SANITIZE_ADDRESS__
		LogDebug("Compiled with AddressSanitizer\n");

		#ifdef HAVE_OPENCL
			const char* asan_options = getenv("ASAN_OPTIONS");
			if( (asan_options == nullptr) || (strstr(asan_options, "protect_shadow_gap=0") == nullptr) )
			{
				#ifndef _WIN32
					LogDebug("glscopeclient requires protect_shadow_gap=0 for OpenCL support to work under asan\n");

					if(asan_options == nullptr)
						setenv("ASAN_OPTIONS", "protect_shadow_gap=0", true);

					else
					{
						string tmp = asan_options;
						tmp += ",protect_shadow_gap=0";
						setenv("ASAN_OPTIONS", tmp.c_str(), true);
					}
					need_relaunch = true;
				#endif
			}
		#endif
	#endif

	#ifndef _WIN32
		if(need_relaunch)
		{
			LogDebug("Re-exec'ing with correct environment\n");
			Relaunch(argc, argv);
		}
	#endif

	g_app = new ScopeApp;

	//Initialize object creation tables for predefined libraries
	if(!VulkanInit())
		return 1;
	TransportStaticInit();
	DriverStaticInit();
	ScopeProtocolStaticInit();
	ScopeExportStaticInit();

	//Disable CPU features we don't want to use
	if(noavx2 && g_hasAvx2)
	{
		g_hasAvx2 = false;
		LogDebug("Disabling AVX2 because --noavx2 argument was passed\n");
	}
	if(noavx512f && g_hasAvx512F)
	{
		g_hasAvx512F = false;
		LogDebug("Disabling AVX512F because --noavx512f argument was passed\n");
	}

	//Initialize object creation tables for plugins
	InitializePlugins();

	//Connect to the scope(s)
	g_app->run(
		g_app->ConnectToScopes(scopes),
		filesToLoad,
		reconnect,
		nodata,
		retrigger);

	//Global cleanup
	ScopehalStaticCleanup();
	delete g_app;

	return 0;
}

#ifndef _WIN32
void Relaunch(int argc, char* argv[])
{
		LogDebug("Relaunching\n");

	//make a copy of arguments since argv[] does not have to be null terminated, but execvp requires that
	vector<char*> args;
	for(int i=0; i<argc; i++)
		args.push_back(argv[i]);
	args.push_back(NULL);

	//Launch ourself with the new environment
	execvp(argv[0], &args[0]);
}
#endif

double GetTime()
{
#ifdef _WIN32
	uint64_t tm;
	static uint64_t freq = 0;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&tm));
	double ret = tm;
	if(freq == 0)
		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
	return ret / freq;
#else
	timespec t;
	clock_gettime(CLOCK_REALTIME,&t);
	double d = static_cast<double>(t.tv_nsec) / 1E9f;
	d += t.tv_sec;
	return d;
#endif
}

void ScopeThread(Oscilloscope* scope)
{
		LogDebug("ScopeThread()\n");

	pthread_setname_np_compat("ScopeThread");

	auto sscope = dynamic_cast<SCPIOscilloscope*>(scope);

	//Assume hyperthreading is enabled and only use one thread per physical core
	omp_set_num_threads(omp_get_num_procs() / 2);

	double tlast = GetTime();
	size_t npolls = 0;
	double dt = 0;
	while(!g_app->IsTerminating())
	{
		//Push any pending queued commands
		if(sscope)
			sscope->GetTransport()->FlushCommandQueue();

		//If the queue is too big, stop grabbing data
		size_t npending = scope->GetPendingWaveformCount();
		if(npending > 20)
		{
			LogTrace("Queue is too big, sleeping\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			tlast = GetTime();

			/*
			if(!sscope)
				continue;
			auto dtransport = dynamic_cast<SCPITwinLanTransport*>(sscope->GetTransport());
			if(!dtransport)
				continue;
			int hsock = dtransport->GetSecondarySocket();
			int bytesReady = 0;
			if(ioctl(hsock, FIONREAD, &bytesReady) >= 0)
				LogDebug("socket has %d bytes ready\n", bytesReady);
			*/

			continue;
		}

		//If the queue is more than 5 sec long, wait for a while before polling any more.
		//We've gotten ahead of the UI!
		if(npending > 1 && npending*dt > 5)
		{
			LogTrace("Capture thread got 5 sec ahead of UI, sleeping\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			tlast = GetTime();
			continue;
		}

		//If trigger isn't armed, don't even bother polling for a while.
		if(!scope->IsTriggerArmed())
		{
			LogTrace("Scope isn't armed, sleeping\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			tlast = GetTime();
			continue;
		}

		auto stat = scope->PollTrigger();

		if(stat == Oscilloscope::TRIGGER_MODE_TRIGGERED)
		{
			//Collect the data, fail if that doesn't work
			if(!scope->AcquireData())
			{
				tlast = GetTime();
				continue;
			}

			//Measure how long the acquisition took
			double now = GetTime();
			dt = now - tlast;
			tlast = now;
			//LogDebug("Triggered, dt = %.3f ms (npolls = %zu)\n",
			//	dt*1000, npolls);

			//If this is a really slow connection (VPN etc), wait a while to let the UI thread do stuff.
			if(dt > 1000)
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

			npolls = 0;

			continue;
		}

		//Wait 1ms before polling again, so the UI thread has a chance to grab the mutex
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

		npolls ++;
	}
}

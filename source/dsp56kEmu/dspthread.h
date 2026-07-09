#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <memory>
#include <string>
#include <thread>

#include "debuggerinterface.h"

namespace dsp56k
{
	class DSP;

	class DSPThread final
	{
	public:
		using Guard = std::lock_guard<std::mutex>;
		using Callback = std::function<void(uint32_t)>;

		explicit DSPThread(DSP& _dsp, const char* _name = nullptr, std::shared_ptr<DebuggerInterface> _debugger = {});
		~DSPThread();
		void join();
		void terminate();

		std::mutex& mutex() { return m_mutex; }

		void setCallback(const Callback& _callback);

		void setLogToDebug(const bool _log) { m_logToDebug = _log; }
		void setLogToStdout(const bool _log) { m_logToStdout = _log; }

		const char* getMipsString() const { return m_mipsString; }
		double getCurrentMips() const { return m_currentMips; }
		double getAverageMips() const { return m_averageMips; }

		void setDebugger(DebuggerInterface* _debugger);
		void detachDebugger(const DebuggerInterface* _debugger);

		DSP& dsp() { return m_dsp; }

		bool runThread() const { return m_runThread; }

		// DIAGNOSTIC ONLY - not a fix. Tests whether voluntarily suspending this
		// realtime-scheduled thread at all causes audio artifacts, independent of any pacing
		// decision (see doc/dsp_threading_and_peripherals.md, "What both attempts had in
		// common"). Remove once the question is answered either way - do not ship.
		void setDiagnosticSleepEnabled(bool _enabled) { m_diagnosticSleepEnabled = _enabled; }
		bool getDiagnosticSleepEnabled() const { return m_diagnosticSleepEnabled; }

		// DIAGNOSTIC ONLY. Callable from any thread (e.g. the host audio thread right before
		// a preset/sysex load). Suppresses the diagnostic sleep for a short cooldown window,
		// to test whether the crackle is caused by the sleep landing during bursty
		// HDI08/preset-load activity specifically, rather than by suspending this thread at
		// all. See doc/dsp_threading_and_peripherals.md, Case study #3.
		void notifyBurstActivity();

	private:
		void threadFunc();
		void diagnosticSleep();

		DSP& m_dsp;
		const std::string m_name;

		std::mutex m_mutex;
		std::unique_ptr<std::thread> m_thread;

		bool m_runThread;

		bool m_diagnosticSleepEnabled = false;
		uint32_t m_diagnosticSleepCounter = 0;
		std::atomic<int64_t> m_diagnosticSleepSuppressUntilUs{0};

		Callback m_callback;

		std::recursive_mutex m_debuggerMutex;
		DebuggerInterface* m_nextDebugger = nullptr;

		std::shared_ptr<DebuggerInterface> m_debugger;

		double m_currentMips = 0.0;
		double m_averageMips = 0.0;

		double m_currentMcps = 0.0;
		double m_averageMcps = 0.0;

		char m_mipsString[128]{0};

		bool m_logToDebug = true;
		bool m_logToStdout = false;
	};
}

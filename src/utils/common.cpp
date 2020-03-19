/**
 * Generate IPV4 and IPV6 5-tuples to standard output
 * gen at the best effort.
 * Need the VIP:port list
 */
#include "common.h"
#include <csignal>
#include <cstdarg>
//#include <gperftools/profiler.h>
#include "input_types.h"

int Clocker::currentLevel = 0;
list<Counter> Counter::counters;
TeeOstream tos;
Clocker global("root", &tos);

//int stick_this_thread_to_core(int core_id) {
//  long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
//  if (core_id < 0 || core_id >= num_cores) return EINVAL;
//
//  cpu_set_t cpuset;
//  CPU_ZERO(&cpuset);
//  CPU_SET(core_id, &cpuset);
//
//  pthread_t current_thread = pthread_self();
//  return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
//}

static pthread_mutex_t printf_mutex;

void sync_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  pthread_mutex_lock(&printf_mutex);
  vprintf(format, args);
  pthread_mutex_unlock(&printf_mutex);
  
  va_end(args);
}

void printStackTrace(int sig) {
  void *array[10];
  size_t size;
  
  // get void*'s for all entries on the stack
  size = backtrace(array, 10);
  
  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

void sigHandler(int sig) {
//  ProfilerStop();
  for (auto &c:Counter::counters) {
    c.lap();
  }
  
  printStackTrace(sig);
  exit(0);
}

void registerSigHandler() {
  signal(SIGINT, sigHandler);
  signal(SIGSEGV, sigHandler);
  signal(SIGABRT, sigHandler);
  signal(SIGFPE, sigHandler);
  signal(SIGILL, sigHandler);
  signal(SIGTERM, sigHandler);
}

void commonInit() {
  system("mkdir -p ../dist/logs/");
  
  registerSigHandler();
  pthread_mutex_init(&printf_mutex, NULL);
}

//! convert a 64-bit Integer to human-readable format in K/M/G. e.g, 102400 is converted to "100K".
std::string human(uint64_t word) {
  std::stringstream ss;
  if (word <= 1024) ss << word;
  else if (word <= 10240) ss << std::setprecision(2) << word * 1.0 / 1024 << "K";
  else if (word <= 1048576) ss << word / 1024 << "K";
  else if (word <= 10485760) ss << word * 1.0 / 1048576 << "M";
  else if (word <= (1048576 << 10)) ss << word / 1048576 << "M";
  else ss << word * 1.0 / (1 << 30) << "G";
  std::string s;
  ss >> s;
  return s;
}

//! split a c-style string with delimineter chara.
std::vector<std::string> split(const char *str, char deli) {
  std::istringstream ss(str);
  std::string token;
  std::vector<std::string> ret;
  while (std::getline(ss, token, deli)) {
    if (token.size() >= 1) ret.push_back(token);
  }
  return ret;
}

string Counter::pad() const {
  if (!Clocker::currentLevel) return "";
  
  ostringstream oss;
  for (int i = 0; i < Clocker::currentLevel; ++i) oss << "| ";
  oss << "  ";
  return oss.str();
}

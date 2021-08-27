//
// Created by maxwe on 2021-08-26.
//

#include "internal.hpp"

#include <string>
#include <iostream>

#include <format>

using namespace std::string_literals;
using namespace std::string_view_literals;

std::istream& skipWhitespace(std::istream& in) noexcept {
  while ( std::isspace(in.peek()))
    in.get();
  return in;
}

std::istream& nextLine(std::istream& in) noexcept {
  std::string line;
  std::getline(in, line);
  return in;
}

enum class action{
  invalid,
  push,
  pop,
  exit,
  debug,
  default_priority
};

struct state_t{
  action      m_action;
  std::string m_string;
  uint32_t    m_priority;
  uint32_t    m_default_priority;
  bool        m_print_debug;
};

void clearState(state_t& state) noexcept {
  state.m_action   = action::invalid;
  state.m_string   = "";
  state.m_priority = state.m_default_priority;

  std::cin.clear();
  std::cin >> std::boolalpha;
  std::cin.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  std::cout << std::flush;
}

void parseState(state_t& state) noexcept {
  std::string cmd;

  clearState(state);


  std::cout << "> ";

  try {
    std::cin >> cmd;
  } catch (std::exception& e) {
    state.m_action = action::invalid;
    state.m_string = "Scan Error: [["s + e.what() + "]]";
    return;
  }

  if ( cmd == "exit"sv) {
    state.m_action = action::exit;
    std::cin >> nextLine;
  }
  else if ( cmd == "push"sv) {

    state.m_action = action::push;

    std::cin >> skipWhitespace;
    getline(std::cin, state.m_string);
    if (isdigit(state.m_string.front())) {
      std::stringstream string_stream{std::move(state.m_string)};
      string_stream >> state.m_priority >> skipWhitespace;
      state.m_string = "";
      std::getline(string_stream, state.m_string);
    }
  }
  else if ( cmd == "pop"sv) {
    state.m_action = action::pop;
  }
  else if ( cmd == "debug"sv) {
    state.m_action = action::debug;
    try {
      std::cin >> state.m_print_debug;

    } catch(std::exception& e) {
      state.m_action = action::invalid;
      state.m_string = R"(Invalid Boolean Value: Must be either "true" or "false" (sysmsg: [[)"s + e.what() + "]]";
    }
    std::cin >> nextLine;
  }
  else if ( cmd == "default_priority"sv) {
    state.m_action = action::default_priority;
    try {
      std::cin >> state.m_default_priority;
    } catch(std::exception& e) {
      state.m_action = action::invalid;
      state.m_string = R"(Invalid Priority Value: Must be a positive integer (sysmsg: [[)"s + e.what() + "]]";
    }
    std::cin >> nextLine;
  }
  else {
    state.m_action = action::invalid;
    state.m_string = "Unrecognized Command: [["s + cmd + "]]";
    std::cin >> nextLine;
  }
}

qtz_request_t makeNewRequest(const state_t& state) noexcept {
  auto req = new qtz_request;
  req->queuePriority = state.m_priority;
  std::memcpy(req->payload, state.m_string.data(), state.m_string.size() + 1);
  return req;
}
void processRequest(qtz_request_t request) noexcept {
  std::cout << std::format("Message: \"{}\"\n", request->payload);
  delete request;
}


struct debug_log_t {
  std::ostream& os;
  const volatile bool& enabled;

  template <typename ...T>
  void log(std::string_view fmt, const T& ...args) {
    if ( enabled ) {
      os << std::format("[Debug] :: "s.append(fmt), args...) << "\n";
    }
  }
};

struct prefix_log_t {
  std::ostream& os;
  std::string   prefix;

  template <typename ...T>
  void log(std::string_view fmt, const T& ...args) {
    os << prefix << std::format(fmt, args...) << "\n";
  }
};


int main() {

  std::string inputString;
  state_t     state{
    .m_default_priority = 20,
    .m_print_debug = false,
  };
  request_priority_queue queue{24};

  debug_log_t debug{std::cout, state.m_print_debug};
  prefix_log_t error{std::cout, "[Error] :: "};
  prefix_log_t info{std::cout, "[Info] :: "};


  do {
    parseState(state);
    switch ( state.m_action ) {
      case action::invalid:
        error.log("{}", state.m_string);
        break;
      case action::push: {
        qtz_request_t newReq = makeNewRequest(state);
        debug.log(R"(Create Request: {{ "address": {}, "priority": {}, "message": "{}" }})", (void*)newReq, newReq->queuePriority, newReq->payload);
        queue.insert(newReq);
        if ( state.m_print_debug ) {
          debug.log(R"(Post Insertion: heap_is_valid = {})", queue.is_valid_heap());
        }
        break;
      }
      case action::pop: {
        if ( queue.empty() ) {
          error.log("No Requests in Queue");
        }
        else {
          qtz_request_t req = queue.pop();
          debug.log(R"(Acquire Request: {{ "address": {}, "priority": {}, "message": "{}" }})", (void*)req, req->queuePriority, req->payload);
          processRequest(req);
        }

        break;
      }

      case action::exit:
        std::cout << "\n\nGoodbye!\n";
        while ( !queue.empty() ) {
          delete queue.pop();
        }
        return 0;
      case action::debug:
        info.log("Set Debug Logging: {}", state.m_print_debug ? "enabled" : "disabled");
        break;
      case action::default_priority:
        info.log("Set Default Priority: {}", state.m_default_priority);
        break;
    }
  } while(true);
}
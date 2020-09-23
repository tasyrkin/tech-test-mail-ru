#include <iostream>
#include <string>
#include <optional>
#include <vector>
#include <fstream>
#include <memory>

/**
 * =============================================================================
 * Commands
 * =============================================================================
 */
class Command {
 public:
  virtual std::optional<std::string> apply(int field, std::string& str) = 0;
  virtual ~Command() = default;
};

/**
 * Makes a string upper case for a specific string
 */
class LowerCaseCommand : public Command {
 public:
  explicit LowerCaseCommand(int n) : field_(n) {}
  ~LowerCaseCommand() override = default;
  std::optional<std::string> apply(int field, std::string& str) override;
 private:
  int field_;
};
std::optional<std::string> LowerCaseCommand::apply(int field, std::string& str) {
  if (field != this->field_) {
    return {};
  }
  std::string result;
  for(auto& c : str) {
     result.push_back(toupper(c));
  }
  // a string copy is made
  return result;
}

/**
 * Makes a string upper case for a specific string
 */
class UpperCaseCommand : public Command {
 public:
  explicit UpperCaseCommand(int n) : field_(n) {}
  std::optional<std::string> apply(int field, std::string& str) override;
  ~UpperCaseCommand() override = default;
 private:
  int field_;
};
std::optional<std::string> UpperCaseCommand::apply(int field, std::string& str) {
  if (field != this->field_) {
    return {};
  }
  std::string result;
  for(auto& c : str) {
     result.push_back(tolower(c));
  }
  // a string copy is made
  return result;
}

/**
 * Makes a string upper case for a specific string
 */
class ReplaceCommand : public Command {
 public:
  explicit ReplaceCommand(int n, char from, char to)
      : field_(n), from_(from), to_(to) {}
  std::optional<std::string> apply(int field, std::string& str) override;
  ~ReplaceCommand() override = default;
 private:
  int field_;
  char from_;
  char to_;
};
std::optional<std::string> ReplaceCommand::apply(int field, std::string& str) {
  if (field != this->field_) {
    return {};
  }
  std::string result;
  for(auto& c : str) {
     if(c == this->from_) {
       result.push_back(this->from_);
     } else {
       result.push_back(c);
     }
  }
  // a string copy is made
  return result;
}

/**
 * =============================================================================
 * End Commands
 * =============================================================================
 */


void print_help_and_exit() {
  std::string help_line = R"(
  FileManipulator modifies line fields in the file
  <file_path>     - path to the file for manipulation
  [N:u]           - change every line's field N to lower case letters
  [N:U]           - change every line's field N to upper case letters
  [N:RAB]         - replace a character A to B in every line's field N

  Note: if N does not represent a valid field, the command is not applied
)";

  std::cout << help_line;
  std::exit(1);
}

/**
 * Splits a string by a delim character
 * @param str
 * @param delim
 * @param out
 */
void tokenize(std::string const &str, const char delim, std::vector<std::string> &out)
{
	size_t start;
	size_t end = 0;

	while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
	{
		end = str.find(delim, start);
		out.push_back(str.substr(start, end - start));
	}
}

/**
 * Parses commands as specified in the requirements. Exits the program if finds a wrong command
 * @param argc
 * @param argv
 * @param commands
 */
void parse_commands(int argc, char* const* argv, std::vector<std::unique_ptr<Command>>& commands) {
  for(int idx = 2; idx < argc; ++idx) {
    std::string cmd(argv[idx]);
    std::vector<std::string> parts;
    tokenize(cmd, ':', parts);

    if (parts.size() != 2) {
      std::cerr << "Warning: unable to parse argument [" << cmd << "]" << std::endl;
      print_help_and_exit();
    }
    int field = std::stoi(parts[0]);
    if (parts[1] == "u") {
      std::unique_ptr<Command> lower_case_command (new LowerCaseCommand(field));
      commands.push_back(std::move(lower_case_command));
    } else if (parts[1] == "U") {
      std::unique_ptr<Command> upper_case_command (new UpperCaseCommand(field));
      commands.push_back(std::move(upper_case_command));
    } else {
      // replace parsing
      if (parts[1].size() != 3) {
        std::cerr << "Warning: unable to parse argument [" << cmd << "]" << std::endl;
        print_help_and_exit();
      }
      // must start with 'R'
      if (parts[1][0] != 'R') {
        std::cerr << "Warning: unable to parse argument [" << cmd << "]" << std::endl;
        print_help_and_exit();
      }
      std::__1::unique_ptr<Command> replace_command (
          new ReplaceCommand(field, parts[1][1], parts[1][2])
      );
      commands.push_back(std::move(replace_command));
    }
  }
}

/**
 * Applies commands as specified in the requirements.
 * Returns changed flag together with the modified fileds
 * @param line
 * @param commands
 * @param changed
 * @param modified
 */
void apply_commands(
    std::string& line,
    std::vector<std::unique_ptr<Command>>& commands,
    bool& changed,
    std::vector<std::string>& modified
    ) {
  std::vector<std::string> fields;
  tokenize(line, '\t', fields);
  for(std::vector<std::string>::size_type idx = 0; idx != fields.size(); idx++) {
    std::string& str = fields[idx];
    for(auto& command : commands) {
      std::optional<std::string> modified_str = *command->apply(idx, str);
      if (modified_str.has_value()) {
        str = modified_str.value();
        changed = true;
      }
    }
    modified.push_back(str);
  }

}

int main(int argc, char**argv) {
  if (argc < 2) {
    print_help_and_exit();
  }
  std::string file_path(argv[1]);

  std::vector<std::unique_ptr<Command>> commands;
  parse_commands(argc, argv, commands);

  std::ifstream infile(file_path);
  std::string line;
  while (std::getline(infile, line)) {
    std::vector<std::string> modified;
    bool changed;
    apply_commands(line, commands, changed, modified);

    // at least one field had changed, thus print out the full string
    if (changed) {
      for(std::vector<std::string>::size_type idx = 0; idx != modified.size(); idx++) {
        std::cout << modified[idx];
        // print tab only when not the last field
        if (idx < modified.size() - 1) {
          std::cout << '\t';
        }
      }
      std::cout << std::endl;
    }
  }

  return 0;
}

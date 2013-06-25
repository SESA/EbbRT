/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <fstream>
#include <regex>

#include <boost/program_options.hpp>
#include <boost/xpressive/xpressive.hpp>

namespace po = boost::program_options;
namespace xp = boost::xpressive;

void
process_stream(std::istream& is, xp::sregex& re)
{
  std::string line;
  xp::smatch match;
  while (std::getline(is, line)) {
    auto res = xp::regex_search(line, match, re);
    if (res) {
      std::cout << line << std::endl;
    }
  }
}

void
process_file(const std::string& name, xp::sregex& re)
{
  std::ifstream is(name.c_str());
  if (is.bad()) {
    throw std::runtime_error("Unable to open file");
  }
  process_stream(is, re);
}

int main(int argc, char** argv)
{
  po::options_description desc("Options");
  desc.add_options()
    ("help", "Print help messages")
    ("pattern", po::value<std::string>()->required(), "Pattern to search for")
    ("files", po::value<std::vector<std::string> >(), "List of files to search");

  po::positional_options_description opts;
  opts.add("pattern", 1);
  opts.add("files", -1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc)
              .positional(opts).run(),
              vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return EXIT_SUCCESS;
    }
    po::notify(vm);
  } catch (po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    std::cerr << desc << std::endl;
    return EXIT_FAILURE;
  }

  std::string pattern = vm["pattern"].as<std::string>();
  xp::sregex re = xp::sregex::compile(pattern);
  const std::vector<std::string>& files =
    vm["files"].as<std::vector<std::string> >();
  for (const auto& fname : files) {
    process_file(fname, re);
  }

  return EXIT_SUCCESS;
}

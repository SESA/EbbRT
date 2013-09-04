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
#include <sstream>

#include "app/HiddenRegister/Hidden.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Console/Console.hpp"

ebbrt::EbbRoot*
ebbrt::Hidden::ConstructRoot()
{
  return new SharedRoot<Hidden>;
}

ebbrt::Hidden::Hidden(EbbId id) : EbbRep{id}
{
  std::ostringstream stream;
  stream << "In " << __PRETTY_FUNCTION__ << " this pointer = " << this <<
    std::endl;
  auto str = stream.str();
  auto buf = console->Alloc(str.size());
  std::memcpy(buf.data(), str.c_str(), str.size());
  console->Write(std::move(buf));
}

void
ebbrt::Hidden::NoReturn()
{
  std::ostringstream stream;
  stream << "In " << __PRETTY_FUNCTION__ << " this pointer = " << this <<
    std::endl;
  auto str = stream.str();
  auto buf = console->Alloc(str.size());
  std::memcpy(buf.data(), str.c_str(), str.size());
  console->Write(std::move(buf));
}

ebbrt::Hidden::SmallStruct
ebbrt::Hidden::SmallPODReturn()
{
  SmallStruct s;
  std::ostringstream stream;
  stream << "In " << __PRETTY_FUNCTION__ << " this pointer = " << this <<
    std::endl;
  auto str = stream.str();
  auto buf = console->Alloc(str.size());
  std::memcpy(buf.data(), str.c_str(), str.size());
  console->Write(std::move(buf));
  return s;
}

ebbrt::Hidden::BigStruct
ebbrt::Hidden::BigPODReturn()
{
  BigStruct b;
  std::ostringstream stream;
  stream << "In " << __PRETTY_FUNCTION__ << " this pointer = " << this <<
    std::endl;
  auto str = stream.str();
  auto buf = console->Alloc(str.size());
  std::memcpy(buf.data(), str.c_str(), str.size());
  console->Write(std::move(buf));
  return b;
}

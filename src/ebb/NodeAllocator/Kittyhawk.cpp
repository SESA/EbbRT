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

#include <stdio.h>
#include "ebb/SharedRoot.hpp"
#include "ebb/NodeAllocator/Kittyhawk.hpp"
#include "ebb/EventManager/EventManager.hpp"


ebbrt::EbbRoot*
ebbrt::Kittyhawk::ConstructRoot()
{
  return new SharedRoot<Kittyhawk>();
}

ebbrt::Kittyhawk::Kittyhawk(EbbId id) : NodeAllocator{id}
{
  // assert if kh commands are unspecified 
  FILE *fp;
  fp = popen("khget", "r"); 
  assert( fp );
  pclose(fp);
}

void
ebbrt::Kittyhawk::Allocate(std::string ip, std::string app, std::string config)
{
  //TODO: allocate by NetworkID, translate ip into string 

  std::string loadcmd = "slload "+app+" "+config+" "+ip;

  FILE *fp;
  char out[1024];

  fp = popen(loadcmd.c_str(), "r");
  while (fgets(out, 1024, fp) != NULL)
    printf("%s \n", out);
  pclose(fp);

//  fp = popen("khget -i InApp ~/EbbRT/build-tst/bare/src/app/PingPong/PingPong.iso 1", "r"); 
//
//  while (fgets(out, 1024, fp) != NULL)
//    printf("%s \n", out);
//
//  pclose(fp);
}

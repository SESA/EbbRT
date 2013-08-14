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
#include <iostream>

extern "C" {
#include "app/LibFox/photon.h"
}

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "app/LibFox/MCPhotonExecutor.hpp"

ebbrt::EbbRoot*
ebbrt::MCPhotonExecutor::ConstructRoot()
{
  return new SharedRoot<MCPhotonExecutor>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol("MCPhotonExecutor",
                        ebbrt::MCPhotonExecutor::ConstructRoot);
}

ebbrt::MCPhotonExecutor::MCPhotonExecutor(EbbId id)
  : Executor{id}
{
}

ebbrt::Future<std::vector<ebbrt::Buffer> >
ebbrt::MCPhotonExecutor::Execute(const std::string& task,
                                 std::vector<Buffer> inputs)
{
  int task_num;
  if (sscanf(task.data(), "photon%d", &task_num) == 1) {
    READONLY ro;
    init_readonly(&ro);
    auto buf = std::malloc(sizeof(WRITEABLE));
    if (buf == nullptr) {
      throw std::bad_alloc();
    }
    auto wr = reinterpret_cast<WRITEABLE*>(buf);
    init_writeable(wr);
    taskcode(task_num, 5000, &ro, wr);
    return make_future<std::vector<Buffer> >(1, Buffer(buf, sizeof(WRITEABLE)));
  } else if (memcmp(task.data(), "print", 5) == 0) {
    auto wr = reinterpret_cast<WRITEABLE*>(inputs[0].data());
    char header[] = " *** final results ***";
    print_writeable(stdout, header, wr);
    return make_future<std::vector<Buffer> >();
   } else {
    auto buf = std::malloc(sizeof(WRITEABLE));
    if (buf == nullptr) {
      throw std::bad_alloc();
    }
    auto wr = reinterpret_cast<WRITEABLE*>(buf);
    init_writeable(wr);

    //FIXME: this is an unnecessary copy due to the way
    //collect_writeable is written
    for (int i = 0; i < inputs.size(); ++i) {
      collect_writeable(wr, reinterpret_cast<WRITEABLE*>(inputs[i].data()));
    }

    return make_future<std::vector<Buffer> >(1, Buffer(buf, sizeof(WRITEABLE)));
  }
}

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

#include <mpi.h>

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/FailureDetector/PhiAccrualFailureDetector.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebb/Timer/Timer.hpp"

ebbrt::EbbRoot*
ebbrt::PhiAccrualFailureDetector::ConstructRoot()
{
  return new SharedRoot<PhiAccrualFailureDetector>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol("PhiAccrualFailureDetector",
                        ebbrt::PhiAccrualFailureDetector::ConstructRoot);
}

namespace {
  //bool failed = false;
  int rank;
  std::chrono::steady_clock::time_point start;
}

ebbrt::PhiAccrualFailureDetector::PhiAccrualFailureDetector(EbbId id)
  : AccrualFailureDetector{id}
{
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  start = std::chrono::steady_clock::now();
  timer_func_ = [=]() {
    auto me = static_cast<EbbRef<PhiAccrualFailureDetector> >(ebbid_);
    me->Periodic();
  };
  timer->Wait(std::chrono::seconds{1}, timer_func_);
}

namespace {
  enum MessageType {
    MONITOR,
    HEARTBEAT
  };
}

void
ebbrt::PhiAccrualFailureDetector::
Register(NetworkId who,
         std::function<void(NetworkId,bool)> func,
         double threshold)
{
  auto& desc = map_[who];
  if (desc.listeners.empty()) {
    //start monitoring
    auto buf = message_manager->Alloc(sizeof(MessageType));
    *reinterpret_cast<MessageType*>(buf.data()) = MONITOR;
    message_manager->Send(who, ebbid_, std::move(buf));
  }
  desc.listeners.emplace(threshold, std::move(func));
}

void
ebbrt::PhiAccrualFailureDetector::HandleMessage(NetworkId from, Buffer buf)
{
  assert(buf.length() >= sizeof(MessageType));

  const auto& mtype = *reinterpret_cast<MessageType*>(buf.data());
  switch(mtype) {
  case MONITOR:
    {
      monitors_.insert(from);
      break;
    }
  case HEARTBEAT:
    {
      auto now = std::chrono::steady_clock::now();
      auto& desc = map_[from];

      // add to sums
      double new_interval = std::chrono::duration_cast<std::chrono::nanoseconds>
        (now - desc.last_heard_from).count();
      desc.last_heard_from = now;
      if (desc.intervals.size() < 1000) {
        desc.simplemovingaverage +=
          (new_interval - desc.simplemovingaverage) /
          (desc.intervals.size() + 1);
        desc.powersumavg +=
          (new_interval * new_interval - desc.powersumavg) /
          (desc.intervals.size() + 1);
      } else {
        const auto& old_interval = desc.intervals.front();
        desc.simplemovingaverage +=
          (new_interval - old_interval) / 1000;
        desc.powersumavg +=
          (((new_interval * new_interval) -
            (old_interval * old_interval)) / 1000);
        desc.intervals.pop_front();
      }
      desc.intervals.emplace_back(std::move(new_interval));

      //check if we need to report that a previous suspicion was false
      if (now > (desc.started + std::chrono::seconds{10})) {
        //find all listeners with a threshold lower than the old phi value
        for (auto it = desc.listeners.begin();
             it != desc.listeners.upper_bound(RegisterInfo(desc.phi));
             it++) {
          //report that it is alive
          it->func(from, false);
        }
      }
      desc.phi = 0;
      break;
    }
  }
}

namespace {
  //from http://www.johndcook.com/cpp_phi.html (public domain)
  // returns the probability of getting a value smaller than x from a
  // normal distribution with mean = 0 and stddev = 1
  double normalCDF(double x)
  {
    // constants
    double a1 =  0.254829592;
    double a2 = -0.284496736;
    double a3 =  1.421413741;
    double a4 = -1.453152027;
    double a5 =  1.061405429;
    double p  =  0.3275911;

    // Save the sign of x
    int sign = 1;
    if (x < 0)
      sign = -1;
    x = fabs(x)/sqrt(2.0);

    // A&S formula 7.1.26
    double t = 1.0/(1.0 + p*x);
    double y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);

    return 0.5*(1.0 + sign*y);
  }
}

void
ebbrt::PhiAccrualFailureDetector::Periodic()
{
  auto now = std::chrono::steady_clock::now();
  // if (((now - start) > std::chrono::seconds{60}) &&
  //     rank == 3 && !failed) {
  //   std::cout << rank << " failing" << std::endl;
  //   monitors_.clear();
  //   failed = true;
  // }
  //send out our heartbeats
  for (const auto& who : monitors_) {
    auto buf = message_manager->Alloc(sizeof(MessageType));
    *reinterpret_cast<MessageType*>(buf.data()) = HEARTBEAT;
    message_manager->Send(who, ebbid_, std::move(buf));
  }

  // if (rank == 0) {
  //   if (map_.empty()) {
  //     std::cout << "map is empty" << std::endl;
  //   } else {
  //     std::cout << "map is not empty" << std::endl;
  //   }
  // }
  for (auto& kv : map_) {
    auto& desc = kv.second;
    // std::cout << "started monitoring " << kv.first.rank <<
    //   std::chrono::duration_cast<std::chrono::seconds>
    //   (now - desc.started).count() << " seconds ago" << std::endl;
    if ((now - desc.started) > std::chrono::seconds{10} &&
        (now - desc.last_heard_from) > std::chrono::seconds{5}) {
      double newphi;
      if (desc.intervals.empty()) {
        //If we haven't gotten any heartbeats, lets assume it is dead
        newphi = std::numeric_limits<double>::max();
      } else {
        double mean = desc.simplemovingaverage;
        double variance =
          (desc.powersumavg * desc.intervals.size() -
           desc.intervals.size() * mean * mean)
          / (desc.intervals.size() - 1);
        double stddev = std::sqrt(variance);

        double sample = std::chrono::duration_cast<std::chrono::nanoseconds>
          (now - desc.last_heard_from).count();
        //Normalize the sample to N(0,1)
        sample -= mean;
        sample /= stddev;
        //The probability of getting a heartbeat later than the current
        //time (the probability that the node is still alive)
        double p_later = 1 - normalCDF(sample);
        newphi = -log10(p_later);
      }

      //Loop over all thresholds that were crossed by this change
      for (auto it = desc.listeners.upper_bound(RegisterInfo(desc.phi));
           it != desc.listeners.upper_bound(RegisterInfo(newphi));
           ++it) {
        double mean = desc.simplemovingaverage;
        double variance =
          (desc.powersumavg * desc.intervals.size() -
           desc.intervals.size() * mean * mean)
          / (desc.intervals.size() - 1);
        double stddev = std::sqrt(variance);
        std::cout << "Haven't heard from " << kv.first.rank << " in " <<
          std::chrono::duration_cast<std::chrono::duration<double> >
          (now - desc.last_heard_from).count() << " seconds. Mean = " <<
          mean << " Stddev = " << stddev << std::endl;
        it->func(kv.first, true);
      }
      desc.phi = newphi;
    }
  }
  timer->Wait(std::chrono::seconds{1}, timer_func_);
}

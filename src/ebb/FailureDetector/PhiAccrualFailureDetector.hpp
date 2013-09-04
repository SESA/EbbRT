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
#ifndef EBBRT_EBB_FAILUREDETECTOR_PHIACCRUALFAILUREDETECTOR_HPP
#define EBBRT_EBB_FAILUREDETECTOR_PHIACCRUALFAILUREDETECTOR_HPP

#include <set>
#include <unordered_map>
#include <unordered_set>

#include "ebb/FailureDetector/AccrualFailureDetector.hpp"

namespace ebbrt {
  class PhiAccrualFailureDetector : public AccrualFailureDetector {
  public:
    static EbbRoot* ConstructRoot();

    PhiAccrualFailureDetector(EbbId id);

    virtual void Register(NetworkId who,
                          std::function<void(NetworkId,bool)> func,
                          double threshold = 5.0) override;

    virtual void HandleMessage(NetworkId from, Buffer buf) override;
  private:
    virtual void Periodic();
    struct RegisterInfo {
      RegisterInfo(double thresh, std::function<void(NetworkId,bool)> mf) :
        threshold{thresh}, func{std::move(mf)} {}
      RegisterInfo(double thresh) :
        threshold{thresh} {}
      bool operator<(const RegisterInfo& rhs) const {
        return threshold < rhs.threshold;
      }

      double threshold;
      std::function<void(NetworkId,bool)> func;
    };
    struct Descriptor {
      Descriptor() : started{std::chrono::steady_clock::now()},
        last_heard_from{started}, powersumavg{0}, simplemovingaverage{0},
        //        interval_sum{0}, squared_interval_sum{0},
        phi{0}
      {
      }
      std::chrono::steady_clock::time_point started;
      std::chrono::steady_clock::time_point last_heard_from;
      std::list<double> intervals;
      double powersumavg;
      double simplemovingaverage;
      // double interval_sum;
      // double squared_interval_sum;
      double phi;
      std::set<RegisterInfo> listeners;
    };
    std::unordered_map<NetworkId, Descriptor> map_;
    std::unordered_set<NetworkId> monitors_;
    std::function<void()> timer_func_;
  };
}

#endif

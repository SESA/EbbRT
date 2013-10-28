#pragma once

#include "ebb/EventManager/Future.hpp"

void activate_context();

void deactivate_context();

void wait_for_future(ebbrt::Future<void>* fut);

#include "app/AppEbb.hpp"

char ebb_id_resv __attribute__ ((section ("static_ebb_ids")));
extern char static_ebb_ids_start[];
ebbrt::Ebb<ebbrt::App> ebbrt::app_ebb __attribute__((init_priority (101))) =
  Ebb<App>(static_cast<EbbId>(&ebb_id_resv - static_ebb_ids_start));

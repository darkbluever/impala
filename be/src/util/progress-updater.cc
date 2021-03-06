// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#include "util/progress-updater.h"

#include "common/logging.h"

using namespace impala;
using namespace std;

ProgressUpdater::ProgressUpdater(const string& label, int64_t total, int period) :
  label_(label), logging_level_(2), total_(total), update_period_(period), 
  num_complete_(0), last_output_percentage_(0) {
}

ProgressUpdater::ProgressUpdater() :
  logging_level_(2), total_(0), update_period_(0), 
  num_complete_(0), last_output_percentage_(0) {
}

void ProgressUpdater::Update(int64_t delta) {
  DCHECK_GE(delta, 0);
  if (delta == 0) return;

  __sync_fetch_and_add(&num_complete_, delta);

  // Cache some shared variables to avoid locking.  It's possible the progress
  // update is out of order (e.g. prints 1 out of 10 after 2 out of 10) 
  double old_percentage = last_output_percentage_;
  int64_t num_complete = num_complete_;
  
  if (num_complete >= total_) {
    // Always print the final 100% complete
    VLOG(logging_level_) << label_ << " 100\% Complete (" 
                         << num_complete << " out of " << total_ << ")";
    return;
  }

  // Convert to percentage as int
  int new_percentage = (static_cast<double>(num_complete) / total_) * 100;
  if (new_percentage - old_percentage > update_period_) {
    // Only update shared variable if this guy was the latest.
    __sync_val_compare_and_swap(&last_output_percentage_, old_percentage, new_percentage);
    VLOG(logging_level_) << label_ << ": " << new_percentage << "\% Complete ("
                         << num_complete << " out of " << total_ << ")";
  }
}

#pragma once

#include "common.h"
#include "msg.h"
#include <stop_token>

#include <boost/outcome.hpp>
#include <boost/outcome/outcome.hpp>

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

outcome::result<void> run_server(common::shared_memory* shm, std::stop_token st);
    

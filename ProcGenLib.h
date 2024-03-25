#pragma once
#ifndef hPGL
#define hPGL

/* For Planet Generation */
#include <thread>
#include <random>
#include <functional>

/* Proceedural Generation Library */
namespace pgl {
	static std::random_device random;
	inline thread_local std::mt19937 global_rng{ random() };
}

#endif